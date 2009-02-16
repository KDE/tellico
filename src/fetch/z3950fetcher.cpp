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
 *   In addition, as a special exception, the author gives permission to   *
 *   link the code of this program with the OpenSSL library released by    *
 *   the OpenSSL Project (or with modified versions of OpenSSL that use    *
 *   the same license as OpenSSL), and distribute linked combinations      *
 *   including the two.  You must obey the GNU General Public License in   *
 *   all respects for all of the code used other than OpenSSL.  If you     *
 *   modify this file, you may extend this exception to your version of    *
 *   the file, but you are not obligated to do so.  If you do not wish to  *
 *   do so, delete this exception statement from your version.             *
 *                                                                         *
 ***************************************************************************/

#include <config.h>

#include "z3950fetcher.h"
#include "z3950connection.h"
#include "messagehandler.h"
#include "../collection.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../translators/grs1importer.h"
#include "../tellico_debug.h"
#include "../gui/lineedit.h"
#include "../gui/combobox.h"
#include "../isbnvalidator.h"
#include "../lccnvalidator.h"

#include <klocale.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <knuminput.h>
#include <KConfigGroup>
#include <kcombobox.h>
#include <kacceleratormanager.h>
#include <kseparator.h>

#include <QFile>
#include <QLabel>
#include <QDomDocument>
#include <QTextStream>
#include <QGridLayout>

namespace {
  static const int Z3950_DEFAULT_PORT = 210;
  static const QString Z3950_DEFAULT_ESN = QString::fromLatin1("F");
}

using Tellico::Fetch::Z3950Fetcher;

Z3950Fetcher::Z3950Fetcher(QObject* parent_)
    : Fetcher(parent_), m_conn(0), m_port(Z3950_DEFAULT_PORT), m_esn(Z3950_DEFAULT_ESN),
      m_started(false), m_done(true), m_MARC21XMLHandler(0),
      m_UNIMARCXMLHandler(0), m_MODSHandler(0) {
}

Z3950Fetcher::~Z3950Fetcher() {
  delete m_MARC21XMLHandler;
  m_MARC21XMLHandler = 0;
  delete m_UNIMARCXMLHandler;
  m_UNIMARCXMLHandler = 0;
  delete m_MODSHandler;
  m_MODSHandler = 0;
  delete m_conn;
  m_conn = 0;
}

QString Z3950Fetcher::defaultName() {
  return i18n("z39.50 Server");
}

QString Z3950Fetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool Z3950Fetcher::canFetch(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::Bibtex;
}

void Z3950Fetcher::readConfigHook(const KConfigGroup& config_) {
  QString preset = config_.readEntry("Preset");
  if(preset.isEmpty()) {
    m_host = config_.readEntry("Host");
    int p = config_.readEntry("Port", Z3950_DEFAULT_PORT);
    if(p > 0) {
      m_port = p;
    }
    m_dbname = config_.readEntry("Database");
    m_sourceCharSet = config_.readEntry("Charset");
    m_syntax = config_.readEntry("Syntax");
    m_user = config_.readEntry("User");
    m_password = config_.readEntry("Password");
  } else {
    m_preset = preset;
    QString serverFile = KStandardDirs::locate("appdata", QString::fromLatin1("z3950-servers.cfg"));
    if(!serverFile.isEmpty()) {
      KConfig serverConfig(serverFile, KConfig::SimpleConfig);
      const QStringList servers = serverConfig.groupList();
      for(QStringList::ConstIterator server = servers.begin(); server != servers.end(); ++server) {
        KConfigGroup cfg(&serverConfig, *server);

        const QString id = *server;
        if(id == preset) {
          const QString name = cfg.readEntry("Name");
          m_host = cfg.readEntry("Host");
          m_port = cfg.readEntry("Port", Z3950_DEFAULT_PORT);
          m_dbname = cfg.readEntry("Database");
          m_sourceCharSet = cfg.readEntry("Charset");
          m_syntax = cfg.readEntry("Syntax");
          m_user = cfg.readEntry("User");
          m_password = cfg.readEntry("Password");
        }
      }
    }
  }

  m_fields = config_.readEntry("Custom Fields", QStringList());
}

void Z3950Fetcher::saveConfigHook(KConfigGroup& config_) {
  config_.writeEntry("Syntax", m_syntax);
  config_.sync();
}

void Z3950Fetcher::search(Tellico::Fetch::FetchKey key_, const QString& value_) {
#ifdef HAVE_YAZ
  m_started = true;
  m_done = false;
  if(m_host.isEmpty() || m_dbname.isEmpty()) {
    myDebug() << "Z3950Fetcher::search() - settings are not set!" << endl;
    stop();
    return;
  }
  m_key = key_;
  m_value = value_;
  m_started = true;

  QString svalue = m_value;
  QRegExp rx1(QString::fromLatin1("['\"].*\\1"));
  if(!rx1.exactMatch(svalue)) {
    svalue.prepend('"').append('"');
  }

  switch(key_) {
    case Title:
      m_pqn = QString::fromLatin1("@attr 1=4 ") + svalue;
      break;
    case Person:
//      m_pqn = QString::fromLatin1("@or ");
//      m_pqn += QString::fromLatin1("@attr 1=1 \"") + m_value + '"';
      m_pqn = QString::fromLatin1(" @attr 1=1003 ") + svalue;
      break;
    case ISBN:
      {
        m_pqn.truncate(0);
        QString s = m_value;
        s.remove('-');
        QStringList isbnList = s.split(QString::fromLatin1("; "));
        // also going to search for isbn10 values
        for(QStringList::Iterator it = isbnList.begin(); it != isbnList.end(); ++it) {
          if((*it).startsWith(QString::fromLatin1("978"))) {
            QString isbn10 = ISBNValidator::isbn10(*it);
            isbn10.remove('-');
            isbnList.insert(it, isbn10);
          }
        }
        const int count = isbnList.count();
        if(count > 1) {
          m_pqn = QString::fromLatin1("@or ");
        }
        for(int i = 0; i < count; ++i) {
          m_pqn += QString::fromLatin1(" @attr 1=7 ") + isbnList[i];
          if(count > 1 && i < count-2) {
            m_pqn += QString::fromLatin1(" @or");
          }
        }
      }
      break;
    case LCCN:
      {
        m_pqn.truncate(0);
        QString s = m_value;
        s.remove('-');
        QStringList lccnList = s.split(QString::fromLatin1("; "));
        while(!lccnList.isEmpty()) {
          m_pqn += QString::fromLatin1(" @or @attr 1=9 ") + lccnList.front();
          if(lccnList.count() > 1) {
            m_pqn += QString::fromLatin1(" @or");
          }
          m_pqn += QString::fromLatin1(" @attr 1=9 ") + LCCNValidator::formalize(lccnList.front());
          lccnList.pop_front();
        }
      }
      break;
    case Keyword:
      m_pqn = QString::fromLatin1("@attr 1=1016 ") + svalue;
      break;
    case Raw:
      m_pqn = m_value;
      break;
    default:
      kWarning() << "Z3950Fetcher::search() - key not recognized: " << key_;
      stop();
      return;
  }
//  m_pqn = QString::fromLatin1("@attr 1=7 0253333490");
  myLog() << "Z3950Fetcher::search() - PQN query = " << m_pqn << endl;

  if(m_conn) {
    m_conn->reset(); // reset counts
  }

  process();
#else // HAVE_YAZ
  Q_UNUSED(key_);
  Q_UNUSED(value_);
  stop();
  return;
#endif
}

void Z3950Fetcher::continueSearch() {
#ifdef HAVE_YAZ
  m_started = true;
  process();
#endif
}

void Z3950Fetcher::stop() {
  if(!m_started) {
    return;
  }
//  myDebug() << "Z3950Fetcher::stop()" << endl;
  m_started = false;
  if(m_conn) {
   // give it a second to cleanup
    m_conn->abort();
    m_conn->wait(1000);
  }
  emit signalDone(this);
}

bool Z3950Fetcher::initMARC21Handler() {
  if(m_MARC21XMLHandler) {
    return true;
  }

  QString xsltfile = KStandardDirs::locate("appdata", QString::fromLatin1("MARC21slim2MODS3.xsl"));
  if(xsltfile.isEmpty()) {
    kWarning() << "Z3950Fetcher::initHandlers() - can not locate MARC21slim2MODS3.xsl.";
    return false;
  }

  KUrl u;
  u.setPath(xsltfile);

  m_MARC21XMLHandler = new XSLTHandler(u);
  if(!m_MARC21XMLHandler->isValid()) {
    kWarning() << "Z3950Fetcher::initHandlers() - error in MARC21slim2MODS3.xsl.";
    delete m_MARC21XMLHandler;
    m_MARC21XMLHandler = 0;
    return false;
  }
  return true;
}

bool Z3950Fetcher::initUNIMARCHandler() {
  if(m_UNIMARCXMLHandler) {
    return true;
  }

  QString xsltfile = KStandardDirs::locate("appdata", QString::fromLatin1("UNIMARC2MODS3.xsl"));
  if(xsltfile.isEmpty()) {
    kWarning() << "Z3950Fetcher::initHandlers() - can not locate UNIMARC2MODS3.xsl.";
    return false;
  }

  KUrl u;
  u.setPath(xsltfile);

  m_UNIMARCXMLHandler = new XSLTHandler(u);
  if(!m_UNIMARCXMLHandler->isValid()) {
    kWarning() << "Z3950Fetcher::initHandlers() - error in UNIMARC2MODS3.xsl.";
    delete m_UNIMARCXMLHandler;
    m_UNIMARCXMLHandler = 0;
    return false;
  }
  return true;
}

bool Z3950Fetcher::initMODSHandler() {
  if(m_MODSHandler) {
    return true;
  }

  QString xsltfile = KStandardDirs::locate("appdata", QString::fromLatin1("mods2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    kWarning() << "Z3950Fetcher::initHandlers() - can not locate mods2tellico.xsl.";
    return false;
  }

  KUrl u;
  u.setPath(xsltfile);

  m_MODSHandler = new XSLTHandler(u);
  if(!m_MODSHandler->isValid()) {
    kWarning() << "Z3950Fetcher::initHandlers() - error in mods2tellico.xsl.";
    delete m_MODSHandler;
    m_MODSHandler = 0;
    // no use in keeping the MARC handlers now
    delete m_MARC21XMLHandler;
    m_MARC21XMLHandler = 0;
    delete m_UNIMARCXMLHandler;
    m_UNIMARCXMLHandler = 0;
    return false;
  }
  return true;
}

void Z3950Fetcher::process() {
  if(m_conn) {
    m_conn->wait();
  } else {
    m_conn = new Z3950Connection(this, m_host, m_port, m_dbname, m_sourceCharSet, m_syntax, m_esn);
    if(!m_user.isEmpty()) {
      m_conn->setUserPassword(m_user, m_password);
    }
  }

  m_conn->setQuery(m_pqn);
  m_conn->start();
}

void Z3950Fetcher::handleResult(const QString& result_) {
  if(result_.isEmpty()) {
    myDebug() << "Z3950Fetcher::handleResult() - empty record found, maybe the character encoding or record format is wrong?" << endl;
    return;
  }

#if 0
  kWarning() << "Remove debug from z3950fetcher.cpp";
  {
    QFile f1(QString::fromLatin1("/tmp/marc.xml"));
    if(f1.open(QIODevice::WriteOnly)) {
//      if(f1.open(QIODevice::WriteOnly | QIODevice::Append)) {
      QTextStream t(&f1);
      t.setEncoding(QTextStream::UnicodeUTF8);
      t << result_;
    }
    f1.close();
  }
#endif
  // assume always utf-8
  QString str, msg;
  Data::CollPtr coll;
  // not marc, has to be grs-1
  if(m_syntax == QLatin1String("grs-1")) {
    Import::GRS1Importer imp(result_);
    coll = imp.collection();
    msg = imp.statusMessage();
  } else { // now the MODS stuff
    if(m_syntax == QLatin1String("mods")) {
      str = result_;
    } else if(m_syntax == QLatin1String("unimarc") && initUNIMARCHandler()) {
      str = m_UNIMARCXMLHandler->applyStylesheet(result_);
    } else if(initMARC21Handler()) { // got to be usmarc/marc21
      str = m_MARC21XMLHandler->applyStylesheet(result_);
    }
    if(str.isEmpty() || !initMODSHandler()) {
      myDebug() << "Z3950Fetcher::handleResult() - empty string or can't init" << endl;
      stop();
      return;
    }
#if 0
    kWarning() << "Remove debug from z3950fetcher.cpp";
    {
      QFile f2(QString::fromLatin1("/tmp/mods.xml"));
//      if(f2.open(QIODevice::WriteOnly)) {
      if(f2.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream t(&f2);
        t.setEncoding(QTextStream::UnicodeUTF8);
        t << str;
      }
      f2.close();
    }
#endif
    Import::TellicoImporter imp(m_MODSHandler->applyStylesheet(str));
    imp.setOptions(imp.options() & ~Import::ImportProgress); // no progress needed
    coll = imp.collection();
    msg = imp.statusMessage();
  }

  if(!coll) {
    if(!msg.isEmpty()) {
      message(msg, MessageHandler::Warning);
    }
    myDebug() << "Z3950Fetcher::handleResult() - no collection pointer: " << msg << endl;
    return;
  }

  if(coll->entryCount() == 0) {
//    myDebug() << "Z3950Fetcher::handleResult() - no Tellico entry in result" << endl;
    return;
  }

  const StringMap customFields = Z3950Fetcher::customFields();
  for(StringMap::ConstIterator it = customFields.begin(); it != customFields.end(); ++it) {
    if(!m_fields.contains(it.key())) {
      coll->removeField(it.key());
    }
  }

  Data::EntryList entries = coll->entries();
  foreach(Data::EntryPtr entry, entries) {
    QString desc = entry->field(QString::fromLatin1("author")) + '/'
                   + entry->field(QString::fromLatin1("publisher"));
    if(!entry->field(QString::fromLatin1("cr_year")).isEmpty()) {
      desc += QChar('/') + entry->field(QString::fromLatin1("cr_year"));
    } else if(!entry->field(QString::fromLatin1("pub_year")).isEmpty()){
      desc += QChar('/') + entry->field(QString::fromLatin1("pub_year"));
    }
    SearchResult* r = new SearchResult(Fetcher::Ptr(this), entry->title(), desc, entry->field(QString::fromLatin1("isbn")));
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }
}

void Z3950Fetcher::done() {
  m_done = true;
  stop();
}

Tellico::Data::EntryPtr Z3950Fetcher::fetchEntry(uint uid_) {
  return m_entries[uid_];
}

void Z3950Fetcher::customEvent(QEvent* event_) {
  if(!m_conn) {
    return;
  }

  if(event_->type() == Z3950ResultFound::uid()) {
    if(m_done) {
      kWarning() << "Z3950Fetcher::customEvent() - result returned after done signal!";
    }
    Z3950ResultFound* e = static_cast<Z3950ResultFound*>(event_);
    handleResult(e->result());
  } else if(event_->type() == Z3950ConnectionDone::uid()) {
    Z3950ConnectionDone* e = static_cast<Z3950ConnectionDone*>(event_);
    if(e->messageType() > -1) {
      message(e->message(), e->messageType());
    }
    m_hasMoreResults = e->hasMoreResults();
    m_conn->wait();
    done();
  } else if(event_->type() == Z3950SyntaxChange::uid()) {
    if(m_done) {
      kWarning() << "Z3950Fetcher::customEvent() - syntax changed after done signal!";
    }
    Z3950SyntaxChange* e = static_cast<Z3950SyntaxChange*>(event_);
    if(m_syntax != e->syntax()) {
      m_syntax = e->syntax();
      // it gets saved when saveConfigHook() get's called from the Fetcher() d'tor
    }
  } else {
    kWarning() << "Z3950Fetcher::customEvent() - weird type: " << event_->type();
  }
}

void Z3950Fetcher::updateEntry(Tellico::Data::EntryPtr entry_) {
//  myDebug() << "Z3950Fetcher::updateEntry() - " << source() << ": " << entry_->title() << endl;
  QString isbn = entry_->field(QString::fromLatin1("isbn"));
  if(!isbn.isEmpty()) {
    search(Fetch::ISBN, isbn);
    return;
  }

  QString lccn = entry_->field(QString::fromLatin1("lccn"));
  if(!lccn.isEmpty()) {
    search(Fetch::LCCN, lccn);
    return;
  }

  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  QString t = entry_->field(QString::fromLatin1("title"));
  if(!t.isEmpty()) {
    search(Fetch::Title, t);
    return;
  }

  myDebug() << "Z3950Fetcher::updateEntry() - insufficient info to search" << endl;
  emit signalDone(this); // always need to emit this if not continuing with the search
}

Tellico::Fetch::ConfigWidget* Z3950Fetcher::configWidget(QWidget* parent_) const {
  return new Z3950Fetcher::ConfigWidget(parent_, this);
}

Z3950Fetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const Z3950Fetcher* fetcher_/*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  m_usePreset = new QCheckBox(i18n("Use preset &server:"), optionsWidget());
  l->addWidget(m_usePreset, ++row, 0);
  connect(m_usePreset, SIGNAL(toggled(bool)), SLOT(slotTogglePreset(bool)));
  m_serverCombo = new GUI::ComboBox(optionsWidget());
  connect(m_serverCombo, SIGNAL(activated(int)), SLOT(slotPresetChanged()));
  l->addWidget(m_serverCombo, row, 1);
  ++row;
  l->addWidget(new KSeparator(optionsWidget()), row, 0, 1, 2);
  l->setRowMinimumHeight(row, 10);

  QLabel* label = new QLabel(i18n("Hos&t: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_hostEdit = new GUI::LineEdit(optionsWidget());
  connect(m_hostEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  connect(m_hostEdit, SIGNAL(textChanged(const QString&)), SIGNAL(signalName(const QString&)));
  l->addWidget(m_hostEdit, row, 1);
  QString w = i18n("Enter the host name of the server.");
  label->setWhatsThis(w);
  m_hostEdit->setWhatsThis(w);
  label->setBuddy(m_hostEdit);

  label = new QLabel(i18n("&Port: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_portSpinBox = new KIntSpinBox(0, 999999, 1, Z3950_DEFAULT_PORT, optionsWidget());
  connect(m_portSpinBox, SIGNAL(valueChanged(int)), SLOT(slotSetModified()));
  l->addWidget(m_portSpinBox, row, 1);
  w = i18n("Enter the port number of the server. The default is %1.", Z3950_DEFAULT_PORT);
  label->setWhatsThis(w);
  m_portSpinBox->setWhatsThis(w);
  label->setBuddy(m_portSpinBox);

  label = new QLabel(i18n("&Database: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_databaseEdit = new GUI::LineEdit(optionsWidget());
  connect(m_databaseEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_databaseEdit, row, 1);
  w = i18n("Enter the database name used by the server.");
  label->setWhatsThis(w);
  m_databaseEdit->setWhatsThis(w);
  label->setBuddy(m_databaseEdit);

  label = new QLabel(i18n("Ch&aracter set: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_charSetCombo = new KComboBox(true, optionsWidget());
  m_charSetCombo->addItem(QString::null);
  m_charSetCombo->addItem(QString::fromLatin1("marc8"));
  m_charSetCombo->addItem(QString::fromLatin1("iso-8859-1"));
  m_charSetCombo->addItem(QString::fromLatin1("utf-8"));
  connect(m_charSetCombo, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_charSetCombo, row, 1);
  w = i18n("Enter the character set encoding used by the z39.50 server. The most likely choice "
           "is MARC-8, although ISO-8859-1 is common as well.");
  label->setWhatsThis(w);
  m_charSetCombo->setWhatsThis(w);
  label->setBuddy(m_charSetCombo);

  label = new QLabel(i18n("&Format: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_syntaxCombo = new GUI::ComboBox(optionsWidget());
  m_syntaxCombo->addItem(i18n("Auto-detect"), QString());
  m_syntaxCombo->addItem(QString::fromLatin1("MODS"), QString::fromLatin1("mods"));
  m_syntaxCombo->addItem(QString::fromLatin1("MARC21"), QString::fromLatin1("marc21"));
  m_syntaxCombo->addItem(QString::fromLatin1("UNIMARC"), QString::fromLatin1("unimarc"));
  m_syntaxCombo->addItem(QString::fromLatin1("USMARC"), QString::fromLatin1("usmarc"));
  m_syntaxCombo->addItem(QString::fromLatin1("GRS-1"), QString::fromLatin1("grs-1"));
  connect(m_syntaxCombo, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_syntaxCombo, row, 1);
  w = i18n("Enter the data format used by the z39.50 server. Tellico will attempt to "
           "automatically detect the best setting if <i>auto-detect</i> is selected.");
  label->setWhatsThis(w);
  m_syntaxCombo->setWhatsThis(w);
  label->setBuddy(m_syntaxCombo);

  label = new QLabel(i18n("&User: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_userEdit = new GUI::LineEdit(optionsWidget());
  m_userEdit->setClickMessage(i18n("Optional"));
  connect(m_userEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_userEdit, row, 1);
  w = i18n("Enter the authentication user name used by the z39.50 database. Most servers "
           "do not need one.");
  label->setWhatsThis(w);
  m_userEdit->setWhatsThis(w);
  label->setBuddy(m_userEdit);

  label = new QLabel(i18n("Pass&word: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_passwordEdit = new GUI::LineEdit(optionsWidget());
  m_passwordEdit->setClickMessage(i18n("Optional"));
  m_passwordEdit->setEchoMode(QLineEdit::Password);
  connect(m_passwordEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_passwordEdit, row, 1);
  w = i18n("Enter the authentication password used by the z39.50 database. Most servers "
           "do not need one. The password will be saved in plain text in the Tellico "
           "configuration file.");
  label->setWhatsThis(w);
  m_passwordEdit->setWhatsThis(w);
  label->setBuddy(m_passwordEdit);

  l->setRowStretch(++row, 1);

  // now add additional fields widget
  addFieldsWidget(Z3950Fetcher::customFields(), fetcher_ ? fetcher_->m_fields : QStringList());

  loadPresets(fetcher_ ? fetcher_->m_preset : QString::null);
  if(fetcher_) {
    m_hostEdit->setText(fetcher_->m_host);
    m_portSpinBox->setValue(fetcher_->m_port);
    m_databaseEdit->setText(fetcher_->m_dbname);
    m_userEdit->setText(fetcher_->m_user);
    m_passwordEdit->setText(fetcher_->m_password);
    m_charSetCombo->setEditText(fetcher_->m_sourceCharSet);
    // the syntax is detected automatically by the fetcher
    // since the config group gets deleted in the config file,
    // the value needs to be retained here
    m_syntax = fetcher_->m_syntax;
    m_syntaxCombo->setCurrentData(m_syntax);
  }
  KAcceleratorManager::manage(optionsWidget());

  // start with presets turned off
  m_usePreset->setChecked(fetcher_ && !fetcher_->m_preset.isEmpty());

  slotTogglePreset(m_usePreset->isChecked());
}

Z3950Fetcher::ConfigWidget::~ConfigWidget() {
}

void Z3950Fetcher::ConfigWidget::saveConfig(KConfigGroup& config_) {
  if(m_usePreset->isChecked()) {
    QString presetID = m_serverCombo->currentData().toString();
    config_.writeEntry("Preset", presetID);
    return;
  }
  config_.deleteEntry("Preset");

  QString s = m_hostEdit->text().trimmed();
  if(!s.isEmpty()) {
    config_.writeEntry("Host", s);
  }
  int port = m_portSpinBox->value();
  if(port > 0) {
    config_.writeEntry("Port", port);
  }
  s = m_databaseEdit->text().trimmed();
  if(!s.isEmpty()) {
    config_.writeEntry("Database", s);
  }
  s = m_charSetCombo->currentText();
  if(!s.isEmpty()) {
    config_.writeEntry("Charset", s);
  }
  s = m_userEdit->text();
  if(!s.isEmpty()) {
    config_.writeEntry("User", s);
  }
  s = m_passwordEdit->text();
  if(!s.isEmpty()) {
    config_.writeEntry("Password", s);
  }
  s = m_syntaxCombo->currentData().toString();
  if(!s.isEmpty()) {
    m_syntax = s;
  }
  config_.writeEntry("Syntax", m_syntax);

  saveFieldsConfig(config_);
  slotSetModified(false);
}

// static
Tellico::StringMap Z3950Fetcher::customFields() {
  StringMap map;
  map[QString::fromLatin1("address")]  = i18n("Address");
  map[QString::fromLatin1("abstract")] = i18n("Abstract");
  map[QString::fromLatin1("illustrator")] = i18n("Illustrator");
  return map;
}

void Z3950Fetcher::ConfigWidget::slotTogglePreset(bool on) {
  m_serverCombo->setEnabled(on);
  if(on) {
    emit signalName(m_serverCombo->currentText());
  }
  m_hostEdit->setEnabled(!on);
  if(!on && !m_hostEdit->text().isEmpty()) {
    emit signalName(m_hostEdit->text());
  }
  m_portSpinBox->setEnabled(!on);
  m_databaseEdit->setEnabled(!on);
  m_userEdit->setEnabled(!on);
  m_passwordEdit->setEnabled(!on);
  m_charSetCombo->setEnabled(!on);
  m_syntaxCombo->setEnabled(!on);
  if(on) {
    emit signalName(m_serverCombo->currentText());
  }
}

void Z3950Fetcher::ConfigWidget::slotPresetChanged() {
  emit signalName(m_serverCombo->currentText());
}

void Z3950Fetcher::ConfigWidget::loadPresets(const QString& current_) {
  QString lang = KGlobal::locale()->languageList().first();
  QString lang2A;
  {
    QString dummy;
    KGlobal::locale()->splitLocale(lang, lang2A, dummy, dummy, dummy);
  }

  QString serverFile = KStandardDirs::locate("appdata", QString::fromLatin1("z3950-servers.cfg"));
  if(serverFile.isEmpty()) {
    kWarning() << "Z3950Fetcher::loadPresets() - no z3950 servers file found";
    return;
  }

  int idx = -1;

  KConfig serverConfig(serverFile, KConfig::SimpleConfig);
  const QStringList servers = serverConfig.groupList();
  // I want the list of servers sorted by name
  QMap<QString, QString> serverNameMap;
  for(QStringList::ConstIterator server = servers.constBegin(); server != servers.constEnd(); ++server) {
    if((*server).isEmpty()) {
      myDebug() << "Z3950Fetcher::ConfigWidget::loadPresets() - empty id" << endl;
      continue;
    }
    KConfigGroup cfg(&serverConfig, *server);
    const QString name = cfg.readEntry("Name");
    if(!name.isEmpty()) {
      serverNameMap.insert(name, *server);
    }
  }
  for(QMap<QString, QString>::ConstIterator it = serverNameMap.constBegin(); it != serverNameMap.constEnd(); ++it) {
    const QString name = it.key();
    const QString group = it.value();
    KConfigGroup cfg(&serverConfig, group);

    m_serverCombo->addItem(i18n(name.toUtf8()), group);
    if(current_.isEmpty() && idx == -1) {
      // set the initial selection to something depending on the language
      const QStringList locales = cfg.readEntry("Locale", QStringList());
      if(locales.indexOf(lang) > -1 || locales.indexOf(lang2A) > -1) {
        idx = m_serverCombo->count() - 1;
      }
    } else if(group == current_) {
      idx = m_serverCombo->count() - 1;
    }
  }
  if(idx > -1) {
    m_serverCombo->setCurrentIndex(idx);
  }
}

QString Z3950Fetcher::ConfigWidget::preferredName() const {
  if(m_usePreset->isChecked()) {
    return m_serverCombo->currentText();
  }
  QString s = m_hostEdit->text();
  return s.isEmpty() ? i18n("z39.50 Server") : s;
}

#include "z3950fetcher.moc"
