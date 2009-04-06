/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "srufetcher.h"
#include "messagehandler.h"
#include "searchresult.h"
#include "../field.h"
#include "../collection.h"
#include "../translators/tellico_xml.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../translators/dcimporter.h"
#include "../tellico_kernel.h"
#include "../tellico_debug.h"
#include "../gui/lineedit.h"
#include "../gui/combobox.h"
#include "../tellico_utils.h"
#include "../utils/lccnvalidator.h"

#include <klocale.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kstandarddirs.h>
#include <KConfigGroup>
#include <kcombobox.h>
#include <kacceleratormanager.h>
#include <knuminput.h>

#include <QLabel>
#include <QGridLayout>

//#define SRU_DEBUG

namespace {
  // 7090 was the old default port, but that wa sjust because LoC used it
  // let's use default HTTP port of 80 now
  static const int SRU_DEFAULT_PORT = 80;
  static const int SRU_MAX_RECORDS = 25;
}

using Tellico::Fetch::SRUFetcher;
using Tellico::Fetch::SRUConfigWidget;

SRUFetcher::SRUFetcher(QObject* parent_)
    : Fetcher(parent_), m_job(0), m_MARCXMLHandler(0), m_MODSHandler(0), m_started(false) {
}

SRUFetcher::SRUFetcher(const QString& name_, const QString& host_, uint port_, const QString& path_,
                       QObject* parent_) : Fetcher(parent_),
      m_host(host_), m_port(port_), m_path(path_),
      m_job(0), m_MARCXMLHandler(0), m_MODSHandler(0), m_started(false) {
  m_name = name_; // m_name is protected in super class
}

SRUFetcher::~SRUFetcher() {
  delete m_MARCXMLHandler;
  m_MARCXMLHandler = 0;
  delete m_MODSHandler;
  m_MODSHandler = 0;
}

QString SRUFetcher::defaultName() {
  return i18n("SRU Server");
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
  m_fields = config_.readEntry("Custom Fields", QStringList());
}

void SRUFetcher::search(Tellico::Fetch::FetchKey key_, const QString& value_) {
  if(m_host.isEmpty() || m_path.isEmpty()) {
    myDebug() << "SRUFetcher::search() - settings are not set!" << endl;
    stop();
    return;
  }

  m_started = true;

#ifdef SRU_DEBUG
  KUrl u(QLatin1String("/home/robby/sru.xml"));
#else
  KUrl u;
  u.setProtocol(QLatin1String("http"));
  u.setHost(m_host);
  u.setPort(m_port);
  u.setPath(m_path);

  u.addQueryItem(QLatin1String("operation"), QLatin1String("searchRetrieve"));
  u.addQueryItem(QLatin1String("version"), QLatin1String("1.1"));
  u.addQueryItem(QLatin1String("maximumRecords"), QString::number(SRU_MAX_RECORDS));
  u.addQueryItem(QLatin1String("recordSchema"), m_format);

  const int type = Kernel::self()->collectionType();
  QString str = QLatin1Char('"') + value_ + QLatin1Char('"');
  switch(key_) {
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
      // no validation here
      str.remove(QLatin1Char('-'));
      // limit to first isbn
      str = str.section(QLatin1Char(';'), 0, 0);
      u.addQueryItem(QLatin1String("query"), QLatin1String("bath.isbn=") + str);
      break;

    case LCCN:
      {
        // limit to first lccn
        str.remove(QLatin1Char('-'));
        str = str.section(QLatin1Char(';'), 0, 0);
        // also try formalized lccn
        QString lccn = LCCNValidator::formalize(str);
        u.addQueryItem(QLatin1String("query"),
                       QLatin1String("bath.lccn=") + str +
                       QLatin1String(" or bath.lccn=") + lccn
                       );
      }
      break;

    case Keyword:
      u.addQueryItem(QLatin1String("query"), str);
      break;

    case Raw:
      {
        QString key = value_.section(QLatin1Char('='), 0, 0).trimmed();
        QString str = value_.section(QLatin1Char('='), 1).trimmed();
        u.addQueryItem(key, str);
      }
      break;

    default:
      kWarning() << "SRUFetcher::search() - key not recognized: " << key_;
      stop();
      break;
  }
#endif
//  myDebug() << u.prettyUrl() << endl;

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(Kernel::self()->widget());
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

  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

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
        myDebug() << "SRUFetcher::slotComplete() - " << d << endl;
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
  } else if(m_format == QLatin1String("dc")) {
    Import::DCImporter imp(dom);
    coll = imp.collection();
    if(!msg.isEmpty()) {
      msg += QLatin1Char('\n');
    }
    msg += imp.statusMessage();
  } else {
    myDebug() << "SRUFetcher::slotComplete() - unrecognized format: " << m_format << endl;
    stop();
    return;
  }

  if(coll && !msg.isEmpty()) {
    message(msg, coll->entryCount() == 0 ? MessageHandler::Warning : MessageHandler::Status);
  }

  if(!coll) {
    myDebug() << "SRUFetcher::slotComplete() - no collection pointer" << endl;
    if(!msg.isEmpty()) {
      message(msg, MessageHandler::Error);
    }
    stop();
    return;
  }

  const StringMap customFields = SRUFetcher::customFields();
  for(StringMap::ConstIterator it = customFields.begin(); it != customFields.end(); ++it) {
    if(!m_fields.contains(it.key())) {
      coll->removeField(it.key());
    }
  }

  Data::EntryList entries = coll->entries();
  foreach(Data::EntryPtr entry, entries) {
    SearchResult* r = new SearchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }
  stop();
}

Tellico::Data::EntryPtr SRUFetcher::fetchEntry(uint uid_) {
  return m_entries[uid_];
}

void SRUFetcher::updateEntry(Tellico::Data::EntryPtr entry_) {
//  myDebug() << "SRUFetcher::updateEntry() - " << source() << ": " << entry_->title() << endl;
  QString isbn = entry_->field(QLatin1String("isbn"));
  if(!isbn.isEmpty()) {
    search(Fetch::ISBN, isbn);
    return;
  }

  QString lccn = entry_->field(QLatin1String("lccn"));
  if(!lccn.isEmpty()) {
    search(Fetch::LCCN, lccn);
    return;
  }

  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  QString t = entry_->field(QLatin1String("title"));
  if(!t.isEmpty()) {
    search(Fetch::Title, t);
    return;
  }

  myDebug() << "SRUFetcher::updateEntry() - insufficient info to search" << endl;
  emit signalDone(this); // always need to emit this if not continuing with the search
}

bool SRUFetcher::initMARCXMLHandler() {
  if(m_MARCXMLHandler) {
    return true;
  }

  QString xsltfile = KStandardDirs::locate("appdata", QLatin1String("MARC21slim2MODS3.xsl"));
  if(xsltfile.isEmpty()) {
    kWarning() << "SRUFetcher::initHandlers() - can not locate MARC21slim2MODS3.xsl.";
    return false;
  }

  KUrl u;
  u.setPath(xsltfile);

  m_MARCXMLHandler = new XSLTHandler(u);
  if(!m_MARCXMLHandler->isValid()) {
    kWarning() << "SRUFetcher::initHandlers() - error in MARC21slim2MODS3.xsl.";
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

  QString xsltfile = KStandardDirs::locate("appdata", QLatin1String("mods2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    kWarning() << "SRUFetcher::initHandlers() - can not locate mods2tellico.xsl.";
    return false;
  }

  KUrl u;
  u.setPath(xsltfile);

  m_MODSHandler = new XSLTHandler(u);
  if(!m_MODSHandler->isValid()) {
    kWarning() << "SRUFetcher::initHandlers() - error in mods2tellico.xsl.";
    delete m_MODSHandler;
    m_MODSHandler = 0;
    return false;
  }
  return true;
}

Tellico::Fetch::Fetcher::Ptr SRUFetcher::libraryOfCongress(QObject* parent_) {
  return Fetcher::Ptr(new SRUFetcher(i18n("Library of Congress (US)"), QLatin1String("z3950.loc.gov"), 7090,
                                     QLatin1String("voyager"), parent_));
}

// static
Tellico::StringMap SRUFetcher::customFields() {
  StringMap map;
  map[QLatin1String("address")]  = i18n("Address");
  map[QLatin1String("abstract")] = i18n("Abstract");
  return map;
}

Tellico::Fetch::ConfigWidget* SRUFetcher::configWidget(QWidget* parent_) const {
  return new SRUConfigWidget(parent_, this);
}

SRUConfigWidget::SRUConfigWidget(QWidget* parent_, const SRUFetcher* fetcher_ /*=0*/)
    : ConfigWidget(parent_) {
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
  m_formatCombo->addItem(QLatin1String("Dublin Core"), QLatin1String("dc"));
  connect(m_formatCombo, SIGNAL(activated(int)), SLOT(slotSetModified()));
  l->addWidget(m_formatCombo, row, 1);
  w = i18n("Enter the result format used by the server.");
  label->setWhatsThis(w);
  m_formatCombo->setWhatsThis(w);
  label->setBuddy(m_formatCombo);

  l->setRowStretch(++row, 1);

  // now add additional fields widget
  addFieldsWidget(SRUFetcher::customFields(), fetcher_ ? fetcher_->m_fields : QStringList());

  if(fetcher_) {
    m_hostEdit->setText(fetcher_->m_host);
    m_portSpinBox->setValue(fetcher_->m_port);
    m_pathEdit->setText(fetcher_->m_path);
    m_formatCombo->setCurrentData(fetcher_->m_format);
  }
  KAcceleratorManager::manage(optionsWidget());
}

void SRUConfigWidget::saveConfig(KConfigGroup& config_) {
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
  saveFieldsConfig(config_);
  slotSetModified(false);
}

QString SRUConfigWidget::preferredName() const {
  QString s = m_hostEdit->text();
  return s.isEmpty() ? SRUFetcher::defaultName() : s;
}

void SRUConfigWidget::slotCheckHost() {
  QString s = m_hostEdit->text();
  // someone might be pasting a full URL, check that
  if(s.indexOf(QLatin1Char(':')) > -1 || s.indexOf(QLatin1Char('/')) > -1) {
    KUrl u(s);
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

#include "srufetcher.moc"
