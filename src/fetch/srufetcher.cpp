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
#include "../translators/xmlimporter.h"
#include "../utils/guiproxy.h"
#include "../gui/lineedit.h"
#include "../gui/combobox.h"
#include "../utils/string_utils.h"
#include "../utils/lccnvalidator.h"
#include "../utils/isbnvalidator.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <kio/job.h>
#include <KJobUiDelegate>
#include <KJobWidgets/KJobWidgets>
#include <KConfigGroup>
#include <KComboBox>
#include <KAcceleratorManager>
#include <KIntSpinBox>

#include <QLabel>
#include <QGridLayout>
#include <QFile>

namespace {
  // 7090 was the old default port, but that was just because LoC used it
  // let's use default HTTP port of 80 now
  static const int SRU_DEFAULT_PORT = 80;
  static const int SRU_MAX_RECORDS = 25;
}

using namespace Tellico;
using Tellico::Fetch::SRUFetcher;

SRUFetcher::SRUFetcher(QObject* parent_)
    : Fetcher(parent_), m_job(0), m_MARCXMLHandler(0), m_MODSHandler(0), m_SRWHandler(0), m_started(false) {
}

SRUFetcher::SRUFetcher(const QString& name_, const QString& host_, uint port_, const QString& path_,
                       QObject* parent_) : Fetcher(parent_),
      m_host(host_), m_port(port_), m_path(path_), m_format(QLatin1String("mods")),
      m_job(0), m_MARCXMLHandler(0), m_MODSHandler(0), m_SRWHandler(0), m_started(false) {
  m_name = name_; // m_name is protected in super class
}

SRUFetcher::~SRUFetcher() {
  delete m_MARCXMLHandler;
  m_MARCXMLHandler = 0;
  delete m_MODSHandler;
  m_MODSHandler = 0;
  delete m_SRWHandler;
  m_SRWHandler = 0;
}

QString SRUFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
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
}

void SRUFetcher::search() {
  if(m_host.isEmpty() || m_path.isEmpty() || m_format.isEmpty()) {
    myDebug() << "settings are not set!";
    stop();
    return;
  }

  m_started = true;

  QUrl u;
  u.setScheme(QLatin1String("http"));
  u.setHost(m_host);
  u.setPort(m_port);
  u.setPath(QLatin1Char('/') + m_path);

  u.addQueryItem(QLatin1String("operation"), QLatin1String("searchRetrieve"));
  u.addQueryItem(QLatin1String("version"), QLatin1String("1.1"));
  u.addQueryItem(QLatin1String("maximumRecords"), QString::number(SRU_MAX_RECORDS));
  if(!m_format.isEmpty() && m_format != QLatin1String("none")) {
    u.addQueryItem(QLatin1String("recordSchema"), m_format);
  }
  // added for the nature.com openSearch, what's the best way to make this an option?
  if(m_host.endsWith(QLatin1String("nature.com"))) {
    u.addQueryItem(QLatin1String("httpAccept"), QLatin1String("application/sru+xml"));
  }

  const int type = collectionType();
  QString str = QLatin1Char('"') + request().value + QLatin1Char('"');
  switch(request().key) {
    case Title:
      u.addQueryItem(QLatin1String("query"), QLatin1String("dc.title=") + str);
      break;

    case Person:
      {
        QString s;
        if(type == Data::Collection::Book || type == Data::Collection::Bibtex) {
          s = QLatin1String("author=") + str + QLatin1String(" or dc.author=") + str;
        } else {
          s = QLatin1String("dc.creator=") + str + QLatin1String(" or dc.editor=") + str;
        }
        u.addQueryItem(QLatin1String("query"), s);
      }
      break;

    case ISBN:
      {
        QString s = request().value;
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
          q += QLatin1String("bath.isbn=") + isbnList.at(i);
          if(i < isbnList.count()-1) {
            q += QLatin1String(" or ");
          }
        }
        u.addQueryItem(QLatin1String("query"), q);
      }
      break;

    case LCCN:
      {
        QString s = request().value;
        QStringList lccnList = FieldFormat::splitValue(s);
        QString q;
        for(int i = 0; i < lccnList.count(); ++i) {
          q += QLatin1String("bath.lccn=") + lccnList.at(i);
          q += QLatin1String(" or bath.lccn=") + LCCNValidator::formalize(lccnList.at(i));
          if(i < lccnList.count()-1) {
            q += QLatin1String(" or ");
          }
        }
        u.addQueryItem(QLatin1String("query"), q);
      }
      break;

    case Keyword:
      u.addQueryItem(QLatin1String("query"), str);
      break;

    case Raw:
      {
        QString key = request().value.section(QLatin1Char('='), 0, 0).trimmed();
        QString str = request().value.section(QLatin1Char('='), 1).trimmed();
        u.addQueryItem(key, str);
      }
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      stop();
      break;
  }
//  myDebug() << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
}

void SRUFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }

  m_started = false;
  emit signalDone(this);
}

void SRUFetcher::slotComplete(KJob*) {
  if(m_job->error()) {
    m_job->ui()->showErrorMessage();
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
  m_job = 0;

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

  const QString result = QString::fromUtf8(data, data.size());

  // first check for SRU errors
  const QString& diag = XML::nsZingDiag;
  Import::XMLImporter xmlImporter(result);
  QDomDocument dom = xmlImporter.domDocument();

  QDomNodeList diagList = dom.elementsByTagNameNS(diag, QLatin1String("diagnostic"));
  for(int i = 0; i < diagList.count(); ++i) {
    QDomElement elem = diagList.item(i).toElement();
    QDomNodeList nodeList1 = elem.elementsByTagNameNS(diag, QLatin1String("message"));
    QDomNodeList nodeList2 = elem.elementsByTagNameNS(diag, QLatin1String("details"));
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
  } else if(m_format == QLatin1String("marcxml") && initMARCXMLHandler()) {
    modsResult = m_MARCXMLHandler->applyStylesheet(result);
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
    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
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
  QString isbn = entry_->field(QLatin1String("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(Fetch::ISBN, isbn);
  }

  QString lccn = entry_->field(QLatin1String("lccn"));
  if(!lccn.isEmpty()) {
    return FetchRequest(Fetch::LCCN, lccn);
  }

  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  QString t = entry_->field(QLatin1String("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }
  return FetchRequest();
}

bool SRUFetcher::initMARCXMLHandler() {
  if(m_MARCXMLHandler) {
    return true;
  }

  QString xsltfile = DataFileRegistry::self()->locate(QLatin1String("MARC21slim2MODS3.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate MARC21slim2MODS3.xsl.";
    return false;
  }

  QUrl u = QUrl::fromLocalFile(xsltfile);

  m_MARCXMLHandler = new XSLTHandler(u);
  if(!m_MARCXMLHandler->isValid()) {
    myWarning() << "error in MARC21slim2MODS3.xsl.";
    delete m_MARCXMLHandler;
    m_MARCXMLHandler = 0;
    return false;
  }
  return true;
}

bool SRUFetcher::initMODSHandler() {
  if(m_MODSHandler) {
    return true;
  }

  QString xsltfile = DataFileRegistry::self()->locate(QLatin1String("mods2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate mods2tellico.xsl.";
    return false;
  }

  QUrl u = QUrl::fromLocalFile(xsltfile);

  m_MODSHandler = new XSLTHandler(u);
  if(!m_MODSHandler->isValid()) {
    myWarning() << "error in mods2tellico.xsl.";
    delete m_MODSHandler;
    m_MODSHandler = 0;
    return false;
  }
  return true;
}

bool SRUFetcher::initSRWHandler() {
  if(m_SRWHandler) {
    return true;
  }

  QString xsltfile = DataFileRegistry::self()->locate(QLatin1String("srw2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate srw2tellico.xsl.";
    return false;
  }

  QUrl u = QUrl::fromLocalFile(xsltfile);

  m_SRWHandler = new XSLTHandler(u);
  if(!m_SRWHandler->isValid()) {
    myWarning() << "error in srw2tellico.xsl.";
    delete m_SRWHandler;
    m_SRWHandler = 0;
    return false;
  }
  return true;
}

Tellico::Fetch::Fetcher::Ptr SRUFetcher::libraryOfCongress(QObject* parent_) {
  return Fetcher::Ptr(new SRUFetcher(i18n("Library of Congress (US)"), QLatin1String("z3950.loc.gov"), 7090,
                                     QLatin1String("voyager"), parent_));
}

QString SRUFetcher::defaultName() {
  return i18n("SRU Server");
}

QString SRUFetcher::defaultIcon() {
  return QLatin1String("network-workgroup"); // just to be different than z3950
}

// static
Tellico::StringHash SRUFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("address")]  = i18n("Address");
  hash[QLatin1String("abstract")] = i18n("Abstract");
  hash[QLatin1String("dewey")]    = i18nc("Dewey Decimal classification system", "Dewey Decimal");
  hash[QLatin1String("lcc")]      = i18nc("Library of Congress classification system", "LoC Classification");
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
  connect(m_hostEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  connect(m_hostEdit, SIGNAL(textChanged(const QString&)), SIGNAL(signalName(const QString&)));
  connect(m_hostEdit, SIGNAL(textChanged(const QString&)), SLOT(slotCheckHost()));
  l->addWidget(m_hostEdit, row, 1);
  QString w = i18n("Enter the host name of the server.");
  label->setWhatsThis(w);
  m_hostEdit->setWhatsThis(w);
  label->setBuddy(m_hostEdit);

  label = new QLabel(i18n("&Port: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_portSpinBox = new KIntSpinBox(0, 999999, 1, SRU_DEFAULT_PORT, optionsWidget());
  connect(m_portSpinBox, SIGNAL(valueChanged(int)), SLOT(slotSetModified()));
  l->addWidget(m_portSpinBox, row, 1);
  w = i18n("Enter the port number of the server. The default is %1.", SRU_DEFAULT_PORT);
  label->setWhatsThis(w);
  m_portSpinBox->setWhatsThis(w);
  label->setBuddy(m_portSpinBox);

  label = new QLabel(i18n("Path: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_pathEdit = new GUI::LineEdit(optionsWidget());
  connect(m_pathEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_pathEdit, row, 1);
  w = i18n("Enter the path to the database used by the server.");
  label->setWhatsThis(w);
  m_pathEdit->setWhatsThis(w);
  label->setBuddy(m_pathEdit);

  label = new QLabel(i18n("Format: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_formatCombo = new GUI::ComboBox(optionsWidget());
  m_formatCombo->addItem(QLatin1String("MODS"), QLatin1String("mods"));
  m_formatCombo->addItem(QLatin1String("MARCXML"), QLatin1String("marcxml"));
  m_formatCombo->addItem(QLatin1String("PAM"), QLatin1String("pam"));
  m_formatCombo->addItem(QLatin1String("Dublin Core"), QLatin1String("dc"));
  m_formatCombo->addItem(QLatin1String(""), QLatin1String("none"));
  connect(m_formatCombo, SIGNAL(activated(int)), SLOT(slotSetModified()));
  l->addWidget(m_formatCombo, row, 1);
  w = i18n("Enter the result format used by the server.");
  label->setWhatsThis(w);
  m_formatCombo->setWhatsThis(w);
  label->setBuddy(m_formatCombo);

  l->setRowStretch(++row, 1);

  // now add additional fields widget
  addFieldsWidget(SRUFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    m_hostEdit->setText(fetcher_->m_host);
    m_portSpinBox->setValue(fetcher_->m_port);
    m_pathEdit->setText(fetcher_->m_path);
    m_formatCombo->setCurrentData(fetcher_->m_format);
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
  s = m_formatCombo->currentData().toString();
  if(!s.isEmpty()) {
    config_.writeEntry("Format", s);
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
        m_pathEdit->setText(u.path());
      }
    }
  }
}

