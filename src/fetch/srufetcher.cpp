/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#include "srufetcher.h"
#include "../fieldformat.h"
#include "../collection.h"
#include "../translators/tellico_xml.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../utils/guiproxy.h"
#include "../gui/lineedit.h"
#include "../gui/combobox.h"
#include "../gui/stringmapwidget.h"
#include "../utils/string_utils.h"
#include "../utils/lccnvalidator.h"
#include "../utils/isbnvalidator.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/Job>
#include <KJobUiDelegate>
#include <KJobWidgets/KJobWidgets>
#include <KConfigGroup>
#include <KComboBox>
#include <KAcceleratorManager>

#include <QSpinBox>
#include <QLabel>
#include <QGridLayout>
#include <QFile>
#include <QUrlQuery>
#include <QDomDocument>

namespace {
  // 7090 was the old default port, but that was just because LoC used it
  // let's use default HTTP port of 80 now
  static const int SRU_DEFAULT_PORT = 80;
  static const int SRU_MAX_RECORDS = 25;
}

using namespace Tellico;
using Tellico::Fetch::SRUFetcher;

SRUFetcher::SRUFetcher(QObject* parent_)
    : Fetcher(parent_), m_port(SRU_DEFAULT_PORT), m_job(nullptr), m_MARCXMLHandler(nullptr), m_MODSHandler(nullptr), m_SRWHandler(nullptr), m_started(false) {
}

SRUFetcher::SRUFetcher(const QString& name_, const QString& host_, uint port_, const QString& path_,
                       const QString& format_, QObject* parent_) : Fetcher(parent_),
      m_host(host_), m_port(port_), m_path(path_), m_format(format_),
      m_job(nullptr), m_MARCXMLHandler(nullptr), m_MODSHandler(nullptr), m_SRWHandler(nullptr), m_started(false) {
  m_name = name_; // m_name is protected in super class
  if(!m_path.startsWith(QLatin1Char('/'))) {
    m_path.prepend(QLatin1Char('/'));
  }
}

SRUFetcher::~SRUFetcher() {
  delete m_MARCXMLHandler;
  m_MARCXMLHandler = nullptr;
  delete m_MODSHandler;
  m_MODSHandler = nullptr;
  delete m_SRWHandler;
  m_SRWHandler = nullptr;
}

QString SRUFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

// No Raw for now.
bool SRUFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == Person || k == ISBN || k == Keyword || k == LCCN;
}

bool SRUFetcher::canFetch(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::Bibtex;
}

void SRUFetcher::readConfigHook(const KConfigGroup& config_) {
  m_host = config_.readEntry("Host");
  int p = config_.readEntry("Port", SRU_DEFAULT_PORT);
  if(p > 0) {
    m_port = p;
  }
  m_path = config_.readEntry("Path");
  // used to be called Database
  if(m_path.isEmpty()) {
    m_path = config_.readEntry("Database");
  }
  if(!m_path.startsWith(QLatin1Char('/'))) {
    m_path.prepend(QLatin1Char('/'));
  }
  m_format = config_.readEntry("Format", "mods");
  const QStringList queryFields = config_.readEntry("QueryFields", QStringList());
  const QStringList queryValues = config_.readEntry("QueryValues", QStringList());
  Q_ASSERT(queryFields.count() == queryValues.count());
  for(int i = 0; i < qMin(queryFields.count(), queryValues.count()); ++i) {
    m_queryMap.insert(queryFields.at(i), queryValues.at(i));
  }
}

void SRUFetcher::search() {
  m_started = true;
  if(m_host.isEmpty() || m_path.isEmpty() || m_format.isEmpty()) {
    myDebug() << "settings are not set!";
    stop();
    return;
  }

  QUrl u;
  u.setScheme(QStringLiteral("http"));
  u.setHost(m_host);
  u.setPort(m_port);
  u = QUrl::fromUserInput(u.url() + m_path);

  QUrlQuery query;
  for(StringMap::ConstIterator it = m_queryMap.constBegin(); it != m_queryMap.constEnd(); ++it) {
    query.addQueryItem(it.key(), it.value());
  }
  // allow user to override these so check for existing item first
  if(!query.hasQueryItem(QStringLiteral("operation"))) {
    query.addQueryItem(QStringLiteral("operation"), QStringLiteral("searchRetrieve"));
  }
  if(!query.hasQueryItem(QStringLiteral("version"))) {
    query.addQueryItem(QStringLiteral("version"), QStringLiteral("1.1"));
  }
  if(!query.hasQueryItem(QStringLiteral("maximumRecords"))) {
    query.addQueryItem(QStringLiteral("maximumRecords"), QString::number(SRU_MAX_RECORDS));
  }
  if(!m_format.isEmpty() && m_format != QLatin1String("none")
     && !query.hasQueryItem(QStringLiteral("recordSchema"))) {
    query.addQueryItem(QStringLiteral("recordSchema"), m_format);
  }

  const int type = collectionType();
  QString str = QLatin1Char('"') + request().value() + QLatin1Char('"');
  switch(request().key()) {
    case Title:
      query.addQueryItem(QStringLiteral("query"), QLatin1String("dc.title=") + str);
      break;

    case Person:
      {
        QString s;
        if(type == Data::Collection::Book || type == Data::Collection::Bibtex) {
          s = QLatin1String("author=") + str + QLatin1String(" or dc.author=") + str;
        } else {
          s = QLatin1String("dc.creator=") + str + QLatin1String(" or dc.editor=") + str;
        }
        query.addQueryItem(QStringLiteral("query"), s);
      }
      break;

    case ISBN:
      {
        QString s = request().value();
        s.remove(QLatin1Char('-'));
        QStringList isbnList = FieldFormat::splitValue(s);
        // also search for isbn10 values
        for(QStringList::Iterator it = isbnList.begin(); it != isbnList.end(); ++it) {
          if((*it).startsWith(QLatin1String("978"))) {
            QString isbn10 = ISBNValidator::isbn10(*it);
            isbn10.remove(QLatin1Char('-'));
            it = isbnList.insert(it, isbn10);
            ++it;
          }
        }
        QString q;
        for(int i = 0; i < isbnList.count(); ++i) {
          // make an assumption that DC output uses the dc profile and everything else uses Bath for ISBN
          // no idea if this holds true universally, but matches LOC, COPAC, and KB
          if(m_format == QLatin1String("dc")) {
            q += QLatin1String("dc.identifier=") + isbnList.at(i);
          } else {
            q += QLatin1String("bath.isbn=") + isbnList.at(i);
          }
          if(i < isbnList.count()-1) {
            q += QLatin1String(" or ");
          }
        }
        query.addQueryItem(QStringLiteral("query"), q);
      }
      break;

    case LCCN:
      {
        QString s = request().value();
        QStringList lccnList = FieldFormat::splitValue(s);
        QString q;
        for(int i = 0; i < lccnList.count(); ++i) {
          q += QLatin1String("bath.lccn=") + lccnList.at(i);
          q += QLatin1String(" or bath.lccn=") + LCCNValidator::formalize(lccnList.at(i));
          if(i < lccnList.count()-1) {
            q += QLatin1String(" or ");
          }
        }
        query.addQueryItem(QStringLiteral("query"), q);
      }
      break;

    case Keyword:
      query.addQueryItem(QStringLiteral("query"), str);
      break;

    case Raw:
      {
        QString key = request().value().section(QLatin1Char('='), 0, 0).trimmed();
        QString str = request().value().section(QLatin1Char('='), 1).trimmed();
        query.addQueryItem(key, str);
      }
      break;

    default:
      myWarning() << "key not recognized: " << request().key();
      stop();
      break;
  }
  u.setQuery(query);
//  myDebug() << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result,
          this, &SRUFetcher::slotComplete);
}

void SRUFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
    m_job = nullptr;
  }

  m_started = false;
  emit signalDone(this);
}

void SRUFetcher::slotComplete(KJob*) {
  if(m_job->error()) {
    m_job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = m_job->data();
  if(data.isEmpty()) {
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = nullptr;

#if 0
  myWarning() << "Remove debug from srufetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  Data::CollPtr coll;
  QString msg;

  const QString result = QString::fromUtf8(data.constData(), data.size());

  // first check for SRU errors
  QDomDocument dom;
  if(!dom.setContent(result, true /*namespace*/)) {
    myWarning() << "server did not return valid XML.";
    stop();
    return;
  }

  const QString& diag = XML::nsZingDiag;
  QDomNodeList diagList = dom.elementsByTagNameNS(diag, QStringLiteral("diagnostic"));
  for(int i = 0; i < diagList.count(); ++i) {
    QDomElement elem = diagList.item(i).toElement();
    QDomNodeList nodeList1 = elem.elementsByTagNameNS(diag, QStringLiteral("message"));
    QDomNodeList nodeList2 = elem.elementsByTagNameNS(diag, QStringLiteral("details"));
    for(int j = 0; j < nodeList1.count(); ++j) {
      QString d = nodeList1.item(j).toElement().text();
      if(!d.isEmpty()) {
        QString d2 = nodeList2.item(j).toElement().text();
        if(!d2.isEmpty()) {
          d += QLatin1String(" (") + d2 + QLatin1Char(')');
        }
        myDebug() << "[" << m_host << "/" << m_path << "]" << d;
        if(!msg.isEmpty()) {
          msg += QLatin1Char('\n');
        }
        msg += d;
      }
    }
  }

  QString modsResult;
  if(m_format == QLatin1String("mods")) {
    modsResult = result;
//  } else if(m_format == QLatin1String("marcxml") && initMARCXMLHandler()) {
// some SRU data sources call it MARC21-xml or something other than marcxml
  } else if(m_format.startsWith(QLatin1String("marc"), Qt::CaseInsensitive) && initMARCXMLHandler()) {
    // brute force marcxchange conversion. This is probably wrong at some level
    QString newResult = result;
    if(m_format.startsWith(QLatin1String("marcxchange"), Qt::CaseInsensitive)) {
      newResult.replace(QRegularExpression(QLatin1String("xmlns:marc=\"info:lc/xmlns/marcxchange-v[12]\"")),
                        QStringLiteral("xmlns:marc=\"http://www.loc.gov/MARC21/slim\""));
    }
    modsResult = m_MARCXMLHandler->applyStylesheet(newResult);
  }
  if(!modsResult.isEmpty() && initMODSHandler()) {
    Import::TellicoImporter imp(m_MODSHandler->applyStylesheet(modsResult));
    coll = imp.collection();
    if(!msg.isEmpty()) {
      msg += QLatin1Char('\n');
    }
    msg += imp.statusMessage();
  } else if((m_format == QLatin1String("pam") ||
             m_format == QLatin1String("dc") ||
             m_format == QLatin1String("none")) &&
            initSRWHandler()) {
    Import::TellicoImporter imp(m_SRWHandler->applyStylesheet(result));
    coll = imp.collection();
    if(!msg.isEmpty()) {
      msg += QLatin1Char('\n');
    }
    msg += imp.statusMessage();
  } else {
    myDebug() << "unrecognized format:" << m_format;
    stop();
    return;
  }

  if(coll && !msg.isEmpty()) {
    message(msg, coll->entryCount() == 0 ? MessageHandler::Warning : MessageHandler::Status);
  }

  if(!coll) {
    myDebug() << "no collection pointer";
    if(!msg.isEmpty()) {
      message(msg, MessageHandler::Error);
    }
    stop();
    return;
  }

  // since the Dewey and LoC field titles have a context in their i18n call here
  // but not in the stylesheet where the field is actually created
  // update the field titles here
  QHashIterator<QString, QString> i(allOptionalFields());
  while(i.hasNext()) {
    i.next();
    Data::FieldPtr field = coll->fieldByName(i.key());
    if(field) {
      field->setTitle(i.value());
      coll->modifyField(field);
    }
  }

  foreach(Data::EntryPtr entry, coll->entries()) {
    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }
  stop();
}

Tellico::Data::EntryPtr SRUFetcher::fetchEntryHook(uint uid_) {
  return m_entries[uid_];
}

Tellico::Fetch::FetchRequest SRUFetcher::updateRequest(Data::EntryPtr entry_) {
//  myDebug() << source() << ": " << entry_->title();
  QString isbn = entry_->field(QStringLiteral("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(Fetch::ISBN, isbn);
  }

  QString lccn = entry_->field(QStringLiteral("lccn"));
  if(!lccn.isEmpty()) {
    return FetchRequest(Fetch::LCCN, lccn);
  }

  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  QString t = entry_->field(QStringLiteral("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }
  return FetchRequest();
}

bool SRUFetcher::initMARCXMLHandler() {
  if(m_MARCXMLHandler) {
    return true;
  }

  QString xsltfile = DataFileRegistry::self()->locate(QStringLiteral("MARC21slim2MODS3.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate MARC21slim2MODS3.xsl.";
    return false;
  }

  QUrl u = QUrl::fromLocalFile(xsltfile);

  m_MARCXMLHandler = new XSLTHandler(u);
  if(!m_MARCXMLHandler->isValid()) {
    myWarning() << "error in MARC21slim2MODS3.xsl.";
    delete m_MARCXMLHandler;
    m_MARCXMLHandler = nullptr;
    return false;
  }
  return true;
}

bool SRUFetcher::initMODSHandler() {
  if(m_MODSHandler) {
    return true;
  }

  QString xsltfile = DataFileRegistry::self()->locate(QStringLiteral("mods2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate mods2tellico.xsl.";
    return false;
  }

  QUrl u = QUrl::fromLocalFile(xsltfile);

  m_MODSHandler = new XSLTHandler(u);
  if(!m_MODSHandler->isValid()) {
    myWarning() << "error in mods2tellico.xsl.";
    delete m_MODSHandler;
    m_MODSHandler = nullptr;
    return false;
  }
  return true;
}

bool SRUFetcher::initSRWHandler() {
  if(m_SRWHandler) {
    return true;
  }

  QString xsltfile = DataFileRegistry::self()->locate(QStringLiteral("srw2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate srw2tellico.xsl.";
    return false;
  }

  QUrl u = QUrl::fromLocalFile(xsltfile);

  m_SRWHandler = new XSLTHandler(u);
  if(!m_SRWHandler->isValid()) {
    myWarning() << "error in srw2tellico.xsl.";
    delete m_SRWHandler;
    m_SRWHandler = nullptr;
    return false;
  }
  return true;
}

Tellico::Fetch::Fetcher::Ptr SRUFetcher::libraryOfCongress(QObject* parent_) {
  return Fetcher::Ptr(new SRUFetcher(i18n("Library of Congress (US)"), QStringLiteral("z3950.loc.gov"), 7090,
                                     QStringLiteral("voyager"), QStringLiteral("mods"), parent_));
}

QString SRUFetcher::defaultName() {
  return i18n("SRU Server");
}

QString SRUFetcher::defaultIcon() {
//  return QLatin1String("network-workgroup"); // just to be different than z3950
  return QStringLiteral(":/icons/sru");
}

// static
Tellico::StringHash SRUFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("address")]  = i18n("Address");
  hash[QStringLiteral("abstract")] = i18n("Abstract");
  hash[QStringLiteral("dewey")]    = i18nc("Dewey Decimal classification system", "Dewey Decimal");
  hash[QStringLiteral("lcc")]      = i18nc("Library of Congress classification system", "LoC Classification");
  return hash;
}

Tellico::Fetch::ConfigWidget* SRUFetcher::configWidget(QWidget* parent_) const {
  return new ConfigWidget(parent_, this);
}

SRUFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const SRUFetcher* fetcher_ /*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;
  QLabel* label = new QLabel(i18n("Hos&t: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_hostEdit = new GUI::LineEdit(optionsWidget());
  connect(m_hostEdit, &QLineEdit::textChanged, this, &ConfigWidget::slotSetModified);
  connect(m_hostEdit, &QLineEdit::textChanged, this, &ConfigWidget::signalName);
  connect(m_hostEdit, &QLineEdit::textChanged, this, &ConfigWidget::slotCheckHost);
  l->addWidget(m_hostEdit, row, 1);
  QString w = i18n("Enter the host name of the server.");
  label->setWhatsThis(w);
  m_hostEdit->setWhatsThis(w);
  label->setBuddy(m_hostEdit);

  label = new QLabel(i18n("&Port: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_portSpinBox = new QSpinBox(optionsWidget());
  m_portSpinBox->setMaximum(999999);
  m_portSpinBox->setMinimum(0);
  m_portSpinBox->setValue(SRU_DEFAULT_PORT);
  void (QSpinBox::* valueChanged)(int) = &QSpinBox::valueChanged;
  connect(m_portSpinBox, valueChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_portSpinBox, row, 1);
  w = i18n("Enter the port number of the server. The default is %1.", SRU_DEFAULT_PORT);
  label->setWhatsThis(w);
  m_portSpinBox->setWhatsThis(w);
  label->setBuddy(m_portSpinBox);

  label = new QLabel(i18n("Path: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_pathEdit = new GUI::LineEdit(optionsWidget());
  connect(m_pathEdit, &QLineEdit::textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_pathEdit, row, 1);
  w = i18n("Enter the path to the database used by the server.");
  label->setWhatsThis(w);
  m_pathEdit->setWhatsThis(w);
  label->setBuddy(m_pathEdit);

  label = new QLabel(i18n("Format: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_formatCombo = new GUI::ComboBox(optionsWidget());
  m_formatCombo->addItem(QStringLiteral("MODS"), QLatin1String("mods"));
  m_formatCombo->addItem(QStringLiteral("MARCXML"), QLatin1String("marcxml"));
  m_formatCombo->addItem(QStringLiteral("PAM"), QLatin1String("pam"));
  m_formatCombo->addItem(QStringLiteral("Dublin Core"), QLatin1String("dc"));
  m_formatCombo->setEditable(true);
  void (GUI::ComboBox::* activatedInt)(int) = &GUI::ComboBox::activated;
  connect(m_formatCombo, activatedInt, this, &ConfigWidget::slotSetModified);
  connect(m_formatCombo, &QComboBox::editTextChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_formatCombo, row, 1);
  w = i18n("Enter the result format used by the server.");
  label->setWhatsThis(w);
  m_formatCombo->setWhatsThis(w);
  label->setBuddy(m_formatCombo);

  l->setRowStretch(++row, 1);

  m_queryTree = new GUI::StringMapWidget(StringMap(), optionsWidget());
  l->addWidget(m_queryTree, row, 0, 1, 2);
  m_queryTree->setLabels(i18n("Field"), i18n("Value"));

  // now add additional fields widget
  addFieldsWidget(SRUFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    m_hostEdit->setText(fetcher_->m_host);
    m_portSpinBox->setValue(fetcher_->m_port);
    m_pathEdit->setText(fetcher_->m_path);
    if(m_formatCombo->findData(fetcher_->m_format) == -1) {
      m_formatCombo->addItem(fetcher_->m_format, fetcher_->m_format);
    }
    m_formatCombo->setCurrentData(fetcher_->m_format);
    m_queryTree->setStringMap(fetcher_->m_queryMap);
  }
  KAcceleratorManager::manage(optionsWidget());
}

void SRUFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  QString s = m_hostEdit->text().trimmed();
  if(!s.isEmpty()) {
    config_.writeEntry("Host", s);
  }
  int port = m_portSpinBox->value();
  if(port > 0) {
    config_.writeEntry("Port", port);
  }
  s = m_pathEdit->text().trimmed();
  if(!s.isEmpty()) {
    config_.writeEntry("Path", s);
  }
  s = m_formatCombo->currentData().toString().trimmed();
  if(s.isEmpty()) {
    // user-entered format will not have data set for the item. Just use the text itself
    s = m_formatCombo->currentText().trimmed();
  }
  if(!s.isEmpty()) {
    config_.writeEntry("Format", s);
  }
  StringMap queryMap = m_queryTree->stringMap();
  if(!queryMap.isEmpty()) {
    config_.writeEntry("QueryFields", queryMap.keys());
    config_.writeEntry("QueryValues", queryMap.values());
  }
}

QString SRUFetcher::ConfigWidget::preferredName() const {
  QString s = m_hostEdit->text();
  return s.isEmpty() ? SRUFetcher::defaultName() : s;
}

void SRUFetcher::ConfigWidget::slotCheckHost() {
  QString s = m_hostEdit->text();
  // someone might be pasting a full URL, check that
  if(s.indexOf(QLatin1Char(':')) > -1 || s.indexOf(QLatin1Char('/')) > -1) {
    QUrl u(s);
    if(u.isValid()) {
      m_hostEdit->setText(u.host());
      if(u.port() > 0) {
        m_portSpinBox->setValue(u.port());
      }
      if(!u.path().isEmpty()) {
        m_pathEdit->setText(u.path().trimmed());
      }
    }
  }
}
