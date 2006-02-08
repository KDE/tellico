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
#include "../latin1literal.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../translators/grs1importer.h"
#include "../tellico_debug.h"
#include "../gui/lineedit.h"

#include <klocale.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <knuminput.h>
#include <kconfig.h>
#include <kcombobox.h>
#include <kaccelmanager.h>

#include <qfile.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qwhatsthis.h>

namespace {
  static const int Z3950_DEFAULT_PORT = 210;
  static const QString Z3950_DEFAULT_ESN = QString::fromLatin1("F");;
  static const size_t Z3950_DEFAULT_MAX_RECORDS = 25;
}

using Tellico::Fetch::Z3950Fetcher;

Z3950Fetcher::Z3950Fetcher(QObject* parent_, const char* name_)
    : Fetcher(parent_, name_), m_conn(0), m_port(Z3950_DEFAULT_PORT), m_esn(Z3950_DEFAULT_ESN),
      m_max(Z3950_DEFAULT_MAX_RECORDS), m_config(0), m_started(false), m_MARC21XMLHandler(0),
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
  return i18n("z39.50");
}

QString Z3950Fetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool Z3950Fetcher::canFetch(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::Bibtex;
}

void Z3950Fetcher::readConfig(KConfig* config_, const QString& group_) {
  // keep a pointer to config so the syntax can be saved
  m_config = config_;
  m_configGroup = group_;

  KConfigGroupSaver groupSaver(config_, group_);
  QString s = config_->readEntry("Name", defaultName());
  if(!s.isEmpty()) {
    m_name = s;
  }
  s = config_->readEntry("Host");
  if(!s.isEmpty()) {
    m_host = s;
  }
  int p = config_->readNumEntry("Port", Z3950_DEFAULT_PORT);
  if(p > 0) {
    m_port = p;
  }
  s = config_->readEntry("Database");
  if(!s.isEmpty()) {
    m_dbname = s;
  }
  s = config_->readEntry("User");
  if(!s.isEmpty()) {
    m_user = s;
  }
  s = config_->readEntry("Password");
  if(!s.isEmpty()) {
    m_password = s;
  }
  s = config_->readEntry("Charset");
  if(!s.isEmpty()) {
    m_sourceCharSet = s;
  }
  s = config_->readEntry("Syntax");
  if(!s.isEmpty()) {
    m_syntax = s;
  }
  m_fields = config_->readListEntry("Custom Fields");
}

void Z3950Fetcher::search(FetchKey key_, const QString& value_) {
#if HAVE_YAZ
  m_started = true;
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
        // for now, don't worry about multiple ISBN values
        m_pqn.truncate(0);
        QString s = m_value;
        s.remove('-');
        const QStringList isbnList = QStringList::split(QString::fromLatin1("; "), s);
        for(QStringList::ConstIterator it = isbnList.begin(); it != isbnList.end(); ++it) {
          m_pqn += QString::fromLatin1(" @attr 1=7 ") + *it;
          if(it != isbnList.fromLast()) {
            m_pqn += QString::fromLatin1(" @or");
          }
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
      kdWarning() << "Z3950Fetcher::search() - key not recognized: " << key_ << endl;
      stop();
      return;
  }
//  m_pqn = QString::fromLatin1("@attr 1=7 0253333490");
  myLog() << "Z3950Fetcher::search() - PQN query = " << m_pqn << endl;

  process();
#else // HAVE_YAZ
  Q_UNUSED(key_);
  Q_UNUSED(value_);
  stop();
  return;
#endif
}

void Z3950Fetcher::stop() {
  if(!m_started) {
    return;
  }
//  myDebug() << "Z3950Fetcher::stop()" << endl;
  m_started = false;
  if(m_conn) {
    m_conn->wait();
  }
  emit signalDone(this);
}

bool Z3950Fetcher::initMARC21Handler() {
  if(m_MARC21XMLHandler) {
    return true;
  }

  QString xsltfile = locate("appdata", QString::fromLatin1("MARC21slim2MODS3.xsl"));
  if(xsltfile.isEmpty()) {
    kdWarning() << "Z3950Fetcher::initHandlers() - can not locate MARC21slim2MODS3.xsl." << endl;
    return false;
  }

  KURL u;
  u.setPath(xsltfile);

  m_MARC21XMLHandler = new XSLTHandler(u);
  if(!m_MARC21XMLHandler->isValid()) {
    kdWarning() << "Z3950Fetcher::initHandlers() - error in MARC21slim2MODS3.xsl." << endl;
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

  QString xsltfile = locate("appdata", QString::fromLatin1("UNIMARC2MODS3.xsl"));
  if(xsltfile.isEmpty()) {
    kdWarning() << "Z3950Fetcher::initHandlers() - can not locate UNIMARC2MODS3.xsl." << endl;
    return false;
  }

  KURL u;
  u.setPath(xsltfile);

  m_UNIMARCXMLHandler = new XSLTHandler(u);
  if(!m_UNIMARCXMLHandler->isValid()) {
    kdWarning() << "Z3950Fetcher::initHandlers() - error in UNIMARC2MODS3.xsl." << endl;
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

  QString xsltfile = locate("appdata", QString::fromLatin1("mods2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    kdWarning() << "Z3950Fetcher::initHandlers() - can not locate mods2tellico.xsl." << endl;
    return false;
  }

  KURL u;
  u.setPath(xsltfile);

  m_MODSHandler = new XSLTHandler(u);
  if(!m_MODSHandler->isValid()) {
    kdWarning() << "Z3950Fetcher::initHandlers() - error in mods2tellico.xsl." << endl;
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
    m_conn = new Z3950Connection(this, m_host, m_port, m_dbname, m_sourceCharSet, m_syntax, m_esn, m_max);
    if(!m_user.isEmpty()) {
      m_conn->setUserPassword(m_user, m_password);
    }
  }

  m_conn->setQuery(m_pqn);
  m_conn->start();
}

void Z3950Fetcher::handleResult(const QString& result_) {
  if(result_.isEmpty()) {
    myDebug() << "Z3950Fetcher::process() - empty record found, maybe the character encoding or record format is wrong?" << endl;
    return;
  }

#if 0
  kdWarning() << "Remove debug from z3950fetcher.cpp" << endl;
  QFile f1(QString::fromLatin1("/tmp/marc.xml"));
  if(f1.open(IO_WriteOnly)) {
//    if(f1.open(IO_WriteOnly | IO_Append)) {
    QTextStream t(&f1);
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << result_;
  }
  f1.close();
#endif
  // assume always utf-8
  QString str, msg;
  Data::CollPtr coll = 0;
  // not marc, has to be grs-1
  if(m_syntax == Latin1Literal("grs-1")) {
    Import::GRS1Importer imp(result_);
    coll = imp.collection();
    msg = imp.statusMessage();
  } else { // now the MODS stuff
    if(m_syntax == Latin1Literal("mods")) {
      str = result_;
    } else if(m_syntax == Latin1Literal("unimarc") && initUNIMARCHandler()) {
      str = m_UNIMARCXMLHandler->applyStylesheet(result_);
    } else if(initMARC21Handler()) { // got to be usmarc/marc21
      str = m_MARC21XMLHandler->applyStylesheet(result_);
    }
    if(str.isEmpty() || !initMODSHandler()) {
      stop();
      return;
    }
#if 0
    kdWarning() << "Remove debug from z3950fetcher.cpp" << endl;
    QFile f2(QString::fromLatin1("/tmp/mods.xml"));
//    if(f2.open(IO_WriteOnly)) {
    if(f2.open(IO_WriteOnly | IO_Append)) {
      QTextStream t(&f2);
      t.setEncoding(QTextStream::UnicodeUTF8);
      t << str;
    }
    f2.close();
#endif
    Import::TellicoImporter imp(m_MODSHandler->applyStylesheet(str));
    coll = imp.collection();
    msg = imp.statusMessage();
  }

  if(!coll) {
    if(!msg.isEmpty()) {
      message(msg, MessageHandler::Warning);
    }
    myDebug() << "Z3950Fetcher::process() - no collection pointer" << endl;
    return;
  }

  if(coll->entryCount() == 0) {
//    myDebug() << "Z3950Fetcher::process() - no Tellico entry in result" << endl;
    return;
  }

  const StringMap customFields = Z3950Fetcher::customFields();
  for(StringMap::ConstIterator it = customFields.begin(); it != customFields.end(); ++it) {
    if(!m_fields.contains(it.key())) {
      coll->removeField(it.key());
    }
  }

  Data::EntryVec entries = coll->entries();
  for(Data::EntryVec::Iterator entry = entries.begin(); entry != entries.end(); ++entry) {
    if(m_key == Fetch::ISBN) {
      m_isbnList.append(entry->field(QString::fromLatin1("isbn")));
    }
    QString desc = entry->field(QString::fromLatin1("author")) + '/'
                   + entry->field(QString::fromLatin1("publisher"));
    if(!entry->field(QString::fromLatin1("cr_year")).isEmpty()) {
      desc += QChar('/') + entry->field(QString::fromLatin1("cr_year"));
    } else if(!entry->field(QString::fromLatin1("pub_year")).isEmpty()){
      desc += QChar('/') + entry->field(QString::fromLatin1("pub_year"));
    }
    SearchResult* r = new SearchResult(this, entry->title(), desc);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }
}

void Z3950Fetcher::done() {
  // tell the user if some of his isbn values were not found
  if(m_key == Fetch::ISBN) {
    const QStringList isbnSearchList = QStringList::split(QString::fromLatin1("; "), m_value);
    QStringList isbnNotFound;
    for(QStringList::ConstIterator it = isbnSearchList.begin(); it != isbnSearchList.end(); ++it) {
      if(!m_isbnList.contains(*it)) {
        isbnNotFound.append(*it);
      }
    }
    // only show message if we were looking for more than one in the first place
    if(isbnSearchList.count() > 1 && !isbnNotFound.isEmpty()) {
      qHeapSort(isbnNotFound);
      infoList(i18n("<qt>No entries were found for the following ISBN values:</qt>"), isbnNotFound);
    }
  }
  stop();
}

Tellico::Data::EntryPtr Z3950Fetcher::fetchEntry(uint uid_) {
  return m_entries[uid_];
}

void Z3950Fetcher::customEvent(QCustomEvent* event_) {
  if(!m_conn) {
    return;
  }

  if(event_->type() == Z3950ResultFound::uid()) {
    Z3950ResultFound* e = static_cast<Z3950ResultFound*>(event_);
    handleResult(e->result());
  } else if(event_->type() == Z3950ConnectionDone::uid()) {
    Z3950ConnectionDone* e = static_cast<Z3950ConnectionDone*>(event_);
    if(e->messageType() > -1) {
      message(e->message(), e->messageType());
    }
    m_conn->wait();
    done();
  } else if(event_->type() == Z3950SyntaxChange::uid()) {
    Z3950SyntaxChange* e = static_cast<Z3950SyntaxChange*>(event_);
    if(m_syntax != e->syntax()) {
      m_syntax = e->syntax();
      if(m_config) {
        KConfigGroupSaver groupSaver(m_config, m_configGroup);
        m_config->writeEntry("Syntax", m_syntax);
        m_config->sync();
      }
    }
  }
}

void Z3950Fetcher::updateEntry(Data::EntryPtr entry_) {
//  myDebug() << "Z3950Fetcher::updateEntry() - " << source() << ": " << entry_->title() << endl;
  QString isbn = entry_->field(QString::fromLatin1("isbn"));
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
  myDebug() << "Z3950Fetcher::updateEntry() - insufficient info to search" << endl;
  emit signalDone(this); // always need to emit this if not continuing with the search
}

Tellico::Fetch::ConfigWidget* Z3950Fetcher::configWidget(QWidget* parent_) const {
  return new Z3950Fetcher::ConfigWidget(parent_, this);
}

Z3950Fetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const Z3950Fetcher* fetcher_/*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget(), 7, 2);
  l->setSpacing(4);
  l->setColStretch(1, 10);

  int row = 0;
  QLabel* label = new QLabel(i18n("Hos&t: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_hostEdit = new GUI::LineEdit(optionsWidget());
  connect(m_hostEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_hostEdit, row, 1);
  QString w = i18n("Enter the host name of the server.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_hostEdit, w);
  label->setBuddy(m_hostEdit);

  label = new QLabel(i18n("&Port: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_portSpinBox = new KIntSpinBox(0, 999999, 1, Z3950_DEFAULT_PORT, 10, optionsWidget());
  connect(m_portSpinBox, SIGNAL(valueChanged(int)), SLOT(slotSetModified()));
  l->addWidget(m_portSpinBox, row, 1);
  w = i18n("Enter the port number of the server. The default is %1.").arg(Z3950_DEFAULT_PORT);
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_portSpinBox, w);
  label->setBuddy(m_portSpinBox);

  label = new QLabel(i18n("&Database: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_databaseEdit = new GUI::LineEdit(optionsWidget());
  connect(m_databaseEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_databaseEdit, row, 1);
  w = i18n("Enter the database name used by the server.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_databaseEdit, w);
  label->setBuddy(m_databaseEdit);

  label = new QLabel(i18n("Ch&aracter set: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_charSetCombo = new KComboBox(true, optionsWidget());
  m_charSetCombo->insertItem(QString::null);
  m_charSetCombo->insertItem(QString::fromLatin1("marc8"));
  m_charSetCombo->insertItem(QString::fromLatin1("iso-8859-1"));
  m_charSetCombo->insertItem(QString::fromLatin1("utf-8"));
  connect(m_charSetCombo, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_charSetCombo, row, 1);
  w = i18n("Enter the character set encoding used by the z39.50 server. The most likely choice "
           "is MARC-8, although ISO-8859-1 is common as well.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_charSetCombo, w);
  label->setBuddy(m_charSetCombo);

  label = new QLabel(i18n("&User: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_userEdit = new GUI::LineEdit(optionsWidget());
  m_userEdit->setHint(i18n("Optional"));
  connect(m_userEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_userEdit, row, 1);
  w = i18n("Enter the authentication user name used by the z39.50 database. Most servers "
           "do not need one.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_userEdit, w);
  label->setBuddy(m_userEdit);

  label = new QLabel(i18n("Pass&word: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_passwordEdit = new GUI::LineEdit(optionsWidget());
  m_passwordEdit->setHint(i18n("Optional"));
  m_passwordEdit->setEchoMode(QLineEdit::Password);
  connect(m_passwordEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_passwordEdit, row, 1);
  w = i18n("Enter the authentication password used by the z39.50 database. Most servers "
           "do not need one. The password will be saved in plain text in the Tellico "
           "configuration file.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_passwordEdit, w);
  label->setBuddy(m_passwordEdit);

  l->setRowStretch(++row, 1);

  // now add additional fields widget
  addFieldsWidget(Z3950Fetcher::customFields(), fetcher_ ? fetcher_->m_fields : QStringList());

  if(fetcher_) {
    m_hostEdit->setText(fetcher_->m_host);
    m_portSpinBox->setValue(fetcher_->m_port);
    m_databaseEdit->setText(fetcher_->m_dbname);
    m_userEdit->setText(fetcher_->m_user);
    m_passwordEdit->setText(fetcher_->m_password);
    m_charSetCombo->setCurrentText(fetcher_->m_sourceCharSet);
    // the syntax is detected automatically by the fetcher
    // since the config group gets deleted in the config file,
    // the value needs to be retained here
    m_syntax = fetcher_->m_syntax;
  }
  KAcceleratorManager::manage(optionsWidget());
}

void Z3950Fetcher::ConfigWidget::saveConfig(KConfig* config_) {
  QString s = m_hostEdit->text().stripWhiteSpace();
  if(!s.isEmpty()) {
    config_->writeEntry("Host", s);
  }
  int port = m_portSpinBox->value();
  if(port > 0) {
    config_->writeEntry("Port", port);
  }
  s = m_databaseEdit->text().stripWhiteSpace();
  if(!s.isEmpty()) {
    config_->writeEntry("Database", s);
  }
  s = m_charSetCombo->currentText();
  if(!s.isEmpty()) {
    config_->writeEntry("Charset", s);
  }
  s = m_userEdit->text();
  if(!s.isEmpty()) {
    config_->writeEntry("User", s);
  }
  s = m_passwordEdit->text();
  if(!s.isEmpty()) {
    config_->writeEntry("Password", s);
  }
  if(!m_syntax.isEmpty()) {
    config_->writeEntry("Syntax", m_syntax);
  }
  saveFieldsConfig(config_);
  slotSetModified(false);
}

// static
Tellico::StringMap Z3950Fetcher::customFields() {
  StringMap map;
  map[QString::fromLatin1("address")]  = i18n("Address");
  map[QString::fromLatin1("abstract")] = i18n("Abstract");
  return map;
}

#include "z3950fetcher.moc"
