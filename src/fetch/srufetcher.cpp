/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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
#include "../field.h"
#include "../collection.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../translators/dcimporter.h"
#include "../tellico_kernel.h"
#include "../tellico_debug.h"
#include "../gui/lineedit.h"
#include "../gui/combobox.h"
#include "../latin1literal.h"
#include "../tellico_utils.h"

#include <klocale.h>
#include <kio/job.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kcombobox.h>
#include <kaccelmanager.h>
#include <knuminput.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qwhatsthis.h>

//#define SRU_DEBUG

namespace {
  static const int SRU_DEFAULT_PORT = 7090;
  static const int SRU_MAX_RECORDS = 25;
}

using Tellico::Fetch::SRUFetcher;

SRUFetcher::SRUFetcher(QObject* parent_, const char* name_)
    : Fetcher(parent_, name_), m_job(0), m_xsltHandler(0), m_started(false) {
}

SRUFetcher::SRUFetcher(const QString& name_, const QString& host_, uint port_, const QString& path_,
                       QObject* parent_) : Fetcher(parent_),
      m_host(host_), m_port(port_), m_path(path_),
      m_job(0), m_xsltHandler(0), m_started(false) {
  m_name = name_; // m_name is protected in super class
}

SRUFetcher::~SRUFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = 0;
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

void SRUFetcher::readConfigHook(KConfig* config_, const QString& group_) {
  KConfigGroupSaver groupSaver(config_, group_);
  m_host = config_->readEntry("Host");
  int p = config_->readNumEntry("Port", SRU_DEFAULT_PORT);
  if(p > 0) {
    m_port = p;
  }
  m_path = config_->readEntry("Path");
  // used to be called Database
  if(m_path.isEmpty()) {
    m_path = config_->readEntry("Database");
  }
  if(!m_path.startsWith(QChar('/'))) {
    m_path.prepend('/');
  }
  m_format = config_->readEntry("Format", QString::fromLatin1("mods"));
  m_fields = config_->readListEntry("Custom Fields");
}

void SRUFetcher::search(FetchKey key_, const QString& value_) {
  if(m_host.isEmpty() || m_path.isEmpty()) {
    myDebug() << "SRUFetcher::search() - settings are not set!" << endl;
    stop();
    return;
  }

  m_started = true;

#ifdef SRU_DEBUG
  KURL u = KURL::fromPathOrURL(QString::fromLatin1("/home/robby/sru.xml"));
#else
  KURL u;
  u.setProtocol(QString::fromLatin1("http"));
  u.setHost(m_host);
  u.setPort(m_port);
  u.setPath(m_path);

  u.addQueryItem(QString::fromLatin1("operation"), QString::fromLatin1("searchRetrieve"));
  u.addQueryItem(QString::fromLatin1("version"), QString::fromLatin1("1.1"));
  u.addQueryItem(QString::fromLatin1("maximumRecords"), QString::number(SRU_MAX_RECORDS));
  u.addQueryItem(QString::fromLatin1("recordSchema"), m_format);

  const int type = Kernel::self()->collectionType();
  QString str = QChar('"') + value_ + QChar('"');
  switch(key_) {
    case Title:
      u.addQueryItem(QString::fromLatin1("query"), QString::fromLatin1("dc.title=") + str);
      break;

    case Person:
      {
        QString s;
        if(type == Data::Collection::Book || type == Data::Collection::Bibtex) {
          s = QString::fromLatin1("author=") + str + QString::fromLatin1(" or dc.author=") + str;
        } else {
          s = QString::fromLatin1("dc.creator=") + str + QString::fromLatin1(" or dc.editor=") + str;
        }
        u.addQueryItem(QString::fromLatin1("query"), s);
      }
      break;

    case ISBN:
      {
        // lccn searches here, don't do any isbn validation, just trust the user
        str.remove('-');
        // limit to first isbn
        str = str.section(';', 0, 0);
        QString s;
        if(type == Data::Collection::Book || type == Data::Collection::Bibtex) {
          s = QString::fromLatin1("bath.isbn=") + str + QString::fromLatin1(" or bath.lccn=") + str;
        } else {
          s = QString::fromLatin1("bath.isbn=") + str + QString::fromLatin1(" or bath.issn=") + str;
        }
        u.addQueryItem(QString::fromLatin1("query"), s);
      }
      break;

    case Keyword:
      u.addQueryItem(QString::fromLatin1("query"), str);
      break;

    case Raw:
      {
        QString key = value_.section('=', 0, 0).stripWhiteSpace();
        QString str = value_.section('=', 1).stripWhiteSpace();
        u.addQueryItem(key, str);
      }
      break;

    default:
      kdWarning() << "SRUFetcher::search() - key not recognized: " << key_ << endl;
      stop();
      break;
  }
#endif
  myDebug() << u.prettyURL() << endl;

  m_job = KIO::get(u, false, false);
  connect(m_job, SIGNAL(data(KIO::Job*, const QByteArray&)),
          SLOT(slotData(KIO::Job*, const QByteArray&)));
  connect(m_job, SIGNAL(result(KIO::Job*)),
          SLOT(slotComplete(KIO::Job*)));
}

void SRUFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }
  m_data.truncate(0);
  m_started = false;
  emit signalDone(this);
}

void SRUFetcher::slotData(KIO::Job*, const QByteArray& data_) {
  QDataStream stream(m_data, IO_WriteOnly | IO_Append);
  stream.writeRawBytes(data_.data(), data_.size());
}

void SRUFetcher::slotComplete(KIO::Job* job_) {
  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  if(job_->error()) {
    job_->showErrorDialog(Kernel::self()->widget());
    stop();
    return;
  }

  if(m_data.isEmpty()) {
    stop();
    return;
  }

  if(m_format == Latin1Literal("mods") && !m_xsltHandler) {
    initXSLTHandler();
    if(!m_xsltHandler) { // probably an error somewhere in the stylesheet loading
      stop();
      return;
    }
  }

  Data::CollPtr coll;
  if(m_format == Latin1Literal("mods")) {
    QString str = m_xsltHandler->applyStylesheet(QString::fromUtf8(m_data, m_data.size()));
    Import::TellicoImporter imp(str);
    coll = imp.collection();
  } else if(m_format == Latin1Literal("dc")) {
    Import::DCImporter imp(Tellico::decodeHTML(QString::fromUtf8(m_data, m_data.size())));
    coll = imp.collection();
  } else {
    myDebug() << "SRUFetcher::slotComplete() - unrecognized format: " << m_format << endl;
    stop();
    return;
  }

  if(!coll) {
    myDebug() << "SRUFetcher::slotComplete() - no collection pointer" << endl;
    stop();
    return;
  }

  const StringMap customFields = SRUFetcher::customFields();
  for(StringMap::ConstIterator it = customFields.begin(); it != customFields.end(); ++it) {
    if(!m_fields.contains(it.key())) {
      coll->removeField(it.key());
    }
  }

  Data::EntryVec entries = coll->entries();
  for(Data::EntryVec::Iterator entry = entries.begin(); entry != entries.end(); ++entry) {
    QString desc;
    switch(coll->type()) {
      case Data::Collection::Book:
        desc = entry->field(QString::fromLatin1("author"))
               + QChar('/')
               + entry->field(QString::fromLatin1("publisher"));
        if(!entry->field(QString::fromLatin1("cr_year")).isEmpty()) {
          desc += QChar('/') + entry->field(QString::fromLatin1("cr_year"));
        } else if(!entry->field(QString::fromLatin1("pub_year")).isEmpty()){
          desc += QChar('/') + entry->field(QString::fromLatin1("pub_year"));
        }
        break;

      case Data::Collection::Video:
        desc = entry->field(QString::fromLatin1("studio"))
               + QChar('/')
               + entry->field(QString::fromLatin1("director"))
               + QChar('/')
               + entry->field(QString::fromLatin1("year"));
        break;

      case Data::Collection::Album:
        desc = entry->field(QString::fromLatin1("artist"))
               + QChar('/')
               + entry->field(QString::fromLatin1("label"))
               + QChar('/')
               + entry->field(QString::fromLatin1("year"));
        break;

      default:
        break;
    }
    SearchResult* r = new SearchResult(this, entry->title(), desc, entry->field(QString::fromLatin1("isbn")));
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }
  stop();
}

Tellico::Data::EntryPtr SRUFetcher::fetchEntry(uint uid_) {
  return m_entries[uid_];
}

void SRUFetcher::updateEntry(Data::EntryPtr entry_) {
//  myDebug() << "SRUFetcher::updateEntry() - " << source() << ": " << entry_->title() << endl;
  QString isbn = entry_->field(QString::fromLatin1("isbn"));
  if(isbn.isEmpty()) {
    isbn = entry_->field(QString::fromLatin1("lccn"));
  }
  if(!isbn.isEmpty()) {
    search(Fetch::ISBN, isbn);
    return;
  } else {
    // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
    QString t = entry_->field(QString::fromLatin1("title"));
    if(!t.isEmpty()) {
      search(Fetch::Title, t);
      return;
    }
  }
  myDebug() << "SRUFetcher::updateEntry() - insufficient info to search" << endl;
  emit signalDone(this); // always need to emit this if not continuing with the search
}

void SRUFetcher::initXSLTHandler() {
  QString xsltfile = locate("appdata", QString::fromLatin1("mods2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    kdWarning() << "SRUFetcher::initXSLTHandler() - can not locate mods2tellico.xsl." << endl;
    return;
  }

  KURL u;
  u.setPath(xsltfile);

  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    kdWarning() << "SRUFetcher::initXSLTHandler() - error in mods2tellico.xsl." << endl;
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

Tellico::Fetch::Fetcher::Ptr SRUFetcher::libraryOfCongress(QObject* parent_) {
  return new SRUFetcher(i18n("Library of Congress (US)"), QString::fromLatin1("z3950.loc.gov"), 7090,
                        QString::fromLatin1("voyager"), parent_);
}

Tellico::Fetch::ConfigWidget* SRUFetcher::configWidget(QWidget* parent_) const {
  return new SRUFetcher::ConfigWidget(parent_, this);
}

SRUFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const SRUFetcher* fetcher_ /*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget(), 4, 2);
  l->setSpacing(4);
  l->setColStretch(1, 10);

  int row = -1;
  QLabel* label = new QLabel(i18n("Hos&t: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_hostEdit = new GUI::LineEdit(optionsWidget());
  connect(m_hostEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  connect(m_hostEdit, SIGNAL(textChanged(const QString&)), SIGNAL(signalName(const QString&)));
  l->addWidget(m_hostEdit, row, 1);
  QString w = i18n("Enter the host name of the server.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_hostEdit, w);
  label->setBuddy(m_hostEdit);

  label = new QLabel(i18n("&Port: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_portSpinBox = new KIntSpinBox(0, 999999, 1, 7090, 10, optionsWidget());
  connect(m_portSpinBox, SIGNAL(valueChanged(int)), SLOT(slotSetModified()));
  l->addWidget(m_portSpinBox, row, 1);
  w = i18n("Enter the port number of the server. The default is %1.").arg(SRU_DEFAULT_PORT);
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_portSpinBox, w);
  label->setBuddy(m_portSpinBox);

  label = new QLabel(i18n("Path: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_pathEdit = new GUI::LineEdit(optionsWidget());
  connect(m_pathEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_pathEdit, row, 1);
  w = i18n("Enter the path to the database used by the server.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_pathEdit, w);
  label->setBuddy(m_pathEdit);

  label = new QLabel(i18n("Format: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_formatCombo = new GUI::ComboBox(optionsWidget());
  m_formatCombo->insertItem(QString::fromLatin1("MODS"), QString::fromLatin1("mods"));
  m_formatCombo->insertItem(QString::fromLatin1("Dublin Core"), QString::fromLatin1("dc"));
  connect(m_formatCombo, SIGNAL(activated(int)), SLOT(slotSetModified()));
  l->addWidget(m_formatCombo, row, 1);
  w = i18n("Enter the result format used by the server.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_formatCombo, w);
  label->setBuddy(m_formatCombo);

  l->setRowStretch(++row, 1);

  // now add additional fields widget
  addFieldsWidget(SRUFetcher::customFields(), fetcher_ ? fetcher_->m_fields : QStringList());

  if(fetcher_) {
    m_hostEdit->setText(fetcher_->m_host);
    m_portSpinBox->setValue(fetcher_->m_port);
    m_pathEdit->setText(fetcher_->m_path);
  }
  KAcceleratorManager::manage(optionsWidget());
}

void SRUFetcher::ConfigWidget::saveConfig(KConfig* config_) {
  QString s = m_hostEdit->text().stripWhiteSpace();
  if(!s.isEmpty()) {
    config_->writeEntry("Host", s);
  }
  int port = m_portSpinBox->value();
  if(port > 0) {
    config_->writeEntry("Port", port);
  }
  s = m_pathEdit->text().stripWhiteSpace();
  if(!s.isEmpty()) {
    config_->writeEntry("Path", s);
  }
  s = m_formatCombo->currentData().toString();
  if(!s.isEmpty()) {
    config_->writeEntry("Format", s);
  }
  saveFieldsConfig(config_);
  slotSetModified(false);
}

QString SRUFetcher::ConfigWidget::preferredName() const {
  QString s = m_hostEdit->text();
  return s.isEmpty() ? SRUFetcher::defaultName() : s;
}

// static
Tellico::StringMap SRUFetcher::customFields() {
  StringMap map;
  map[QString::fromLatin1("address")]  = i18n("Address");
  map[QString::fromLatin1("abstract")] = i18n("Abstract");
  return map;
}

#include "srufetcher.moc"
