/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
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
#include "../document.h"
#include "../collection.h"
#include "../latin1literal.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../tellico_debug.h"
#include "../gui/lineedit.h"

#include <klocale.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <knuminput.h>
#include <kconfig.h>
#include <kcombobox.h>
#include <kmessagebox.h>

#include <qfile.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qwhatsthis.h>

#if HAVE_YAZ
extern "C" {
#include <yaz/zoom.h>
#include <yaz/marcdisp.h>
}
#endif

namespace {
  static const int Z3950_DEFAULT_PORT = 210;
  static const QString Z3950_DEFAULT_ESN = QString::fromLatin1("B");;
  static const size_t Z3950_DEFAULT_MAX_RECORDS = 25;
}

using Tellico::Fetch::Z3950Fetcher;

Tellico::XSLTHandler* Z3950Fetcher::s_MARC21XMLHandler = 0;
Tellico::XSLTHandler* Z3950Fetcher::s_UNIMARCXMLHandler = 0;
Tellico::XSLTHandler* Z3950Fetcher::s_MODSHandler = 0;

// static
void Z3950Fetcher::initHandlers() {
  if(s_MARC21XMLHandler && s_MODSHandler) {
    return;
  }

  QString xsltfile = locate("appdata", QString::fromLatin1("MARC21slim2MODS3.xsl"));
  if(xsltfile.isEmpty()) {
    kdWarning() << "Z3950Fetcher::initHandlers() - can not locate MARC21slim2MODS3.xsl." << endl;
    return;
  }

  KURL u;
  u.setPath(xsltfile);

  s_MARC21XMLHandler = new XSLTHandler(u);
  if(!s_MARC21XMLHandler->isValid()) {
    kdWarning() << "Z3950Fetcher::initHandlers() - error in MARC21slim2MODS3.xsl." << endl;
    delete s_MARC21XMLHandler;
    s_MARC21XMLHandler = 0;
    return;
  }

  xsltfile = locate("appdata", QString::fromLatin1("UNIMARC2MODS3.xsl"));
  if(xsltfile.isEmpty()) {
    kdWarning() << "Z3950Fetcher::initHandlers() - can not locate UNIMARC2MODS3.xsl." << endl;
    return;
  }

  u.setPath(xsltfile);

  s_UNIMARCXMLHandler = new XSLTHandler(u);
  if(!s_UNIMARCXMLHandler->isValid()) {
    kdWarning() << "Z3950Fetcher::initHandlers() - error in UNIMARC2MODS3.xsl." << endl;
    delete s_UNIMARCXMLHandler;
    s_UNIMARCXMLHandler = 0;
    return;
  }

  xsltfile = locate("appdata", QString::fromLatin1("mods2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    kdWarning() << "Z3950Fetcher::initHandlers() - can not locate mods2tellico.xsl." << endl;
    return;
  }

  u.setPath(xsltfile);

  s_MODSHandler = new XSLTHandler(u);
  if(!s_MODSHandler->isValid()) {
    kdWarning() << "Z3950Fetcher::initHandlers() - error in mods2tellico.xsl." << endl;
    delete s_MODSHandler;
    s_MODSHandler = 0;
    delete s_MARC21XMLHandler;
    s_MARC21XMLHandler = 0;
    return;
  }
}

QString Z3950Fetcher::toXML(const QCString& marc_, const QCString& charSet_) {
#if HAVE_YAZ
  if(marc_.isEmpty()) {
    kdDebug() << "Z3950Fetcher::toXML() - empty string" << endl;
    return QString::null;
  }

  yaz_iconv_t cd = yaz_iconv_open("utf-8", charSet_);
  if(!cd) {
    kdWarning() << "Z3950Fetcher::toXML() - conversion from " << charSet_ << " is unsupported" << endl;
    return QString::null;
  }

  yaz_marc_t mt = yaz_marc_create();
  yaz_marc_iconv(mt, cd);
  yaz_marc_xml(mt, YAZ_MARC_MARCXML);

  // first 5 bytes are length
  bool ok;
  int len = marc_.left(5).toInt(&ok);
  if (len < 25 || len > 100000) {
    kdDebug() << "Z3950Fetcher::toXML() - bad length: " << (ok ? len : -1) << endl;
    return QString::null;
  }

  char* result;
  int r = yaz_marc_decode_buf(mt, marc_, -1, &result, &len);
  if(r <= 0) {
    kdDebug() << "Z3950Fetcher::toXML() - can't decode buffer" << endl;
    return QString::null;
  }

  QString output = QString::fromLatin1("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  output += QString::fromUtf8(QCString(result, len+1), len+1);
//  kdDebug() << QCString(result) << endl;
//  kdDebug() << "-------------------------------------------" << endl;
//  kdDebug() << output << endl;
  yaz_iconv_close(cd);
  yaz_marc_destroy(mt);
//  kdDebug() << QString::fromUtf8(QCString(result, len+1), len) << endl;
//  return QString::fromUtf8(result, len);
  return output;
#else // no yaz
  return QString::null;
#endif
}

Z3950Fetcher::Z3950Fetcher(QObject* parent_, const char* name_)
    : Fetcher(parent_, name_), m_port(Z3950_DEFAULT_PORT), m_esn(Z3950_DEFAULT_ESN),
      m_max(Z3950_DEFAULT_MAX_RECORDS), m_config(0), m_started(false) {
}

Z3950Fetcher::Z3950Fetcher(const QString& name_, const QString& host_, uint port_, const QString& dbname_,
                           const QString& user_, const QString& password_, const QString& sourceCharSet_,
                           QObject* parent_) : Fetcher(parent_),
      m_name(name_), m_host(host_), m_port(port_), m_dbname(dbname_), m_user(user_),
      m_password(password_), m_sourceCharSet(sourceCharSet_), m_esn(Z3950_DEFAULT_ESN),
      m_max(Z3950_DEFAULT_MAX_RECORDS), m_config(0), m_started(false) {
}

Z3950Fetcher::~Z3950Fetcher() {
}

QString Z3950Fetcher::source() const {
  return m_name;
}

bool Z3950Fetcher::canFetch(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::Bibtex;
}

void Z3950Fetcher::readConfig(KConfig* config_, const QString& group_) {
  // keep a pointer to config so the syntax can be saved
  m_config = config_;
  m_configGroup = group_;

  KConfigGroupSaver groupSaver(config_, group_);
  QString s = config_->readEntry("Name");
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
}

void Z3950Fetcher::search(FetchKey key_, const QString& value_, bool multiple_) {
#if HAVE_YAZ
  m_started = true;
  if(m_host.isEmpty() || m_dbname.isEmpty()) {
    kdDebug() << "Z3950Fetcher::search() - settings are not set!" << endl;
    stop();
    return;
  }
  m_key = key_;
  m_value = value_;
  m_started = true;

  switch(key_) {
    case Title:
      m_pqn = QString::fromLatin1("@attr 1=4 \"") + m_value + '"';
      break;
    case Person:
//      m_pqn = QString::fromLatin1("@or ");
//      m_pqn += QString::fromLatin1("@attr 1=1 \"") + m_value + '"';
      m_pqn = QString::fromLatin1(" @attr 1=1003 \"") + m_value + '"';
      break;
    case ISBN:
      // for now, don't worry about multiple ISBN values
      if(multiple_) {
        QString s = m_value;
        s.remove('-');
        const QStringList isbnList = QStringList::split(';', s);
        for(QStringList::ConstIterator it = isbnList.begin(); it != isbnList.end(); ++it) {
          if((*it) != isbnList.last()) {
            m_pqn += QString::fromLatin1(" @or");
          }
          m_pqn += QString::fromLatin1(" @attr 1=7 ") + *it;
        }
      } else {
        m_value = m_value.remove('-');
        m_pqn = QString::fromLatin1("@attr 1=7 ") + m_value;
      }
      break;
    case Keyword:
      m_pqn = QString::fromLatin1("@attr 1=1016 ") + m_value;
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
//  kdDebug() << "** PQN query = " << m_pqn << endl;

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
//  kdDebug() << "Z3950Fetcher::stop()" << endl;
  emit signalDone(this);
  m_started = false;
}

void Z3950Fetcher::process() {
#if HAVE_YAZ
  initHandlers();
  if(!s_MARC21XMLHandler || !s_UNIMARCXMLHandler || !s_MODSHandler) {
    QString s = i18n("Tellico encountered an error in XSLT processing.");
    signalStatus(s);
    kdDebug() << s << endl;
    stop();
    return;
  }

#if 0
    kdWarning() << "Remove debug from z3950fetcher.cpp" << endl;
    QFile f1(QString::fromLatin1("/tmp/marc"));
    if(f1.open(IO_ReadOnly)) {
      QByteArray ba = f1.readAll();
      toXML(QCString(ba, ba.size()+1));
    }
    f1.close();
    return;
#endif

// I don't know what to do except assume database, user, and password are in locale encoding
  ZOOM_options conn_opt = ZOOM_options_create();
  ZOOM_options_set(conn_opt, "implementationName", "Tellico");
  ZOOM_options_set(conn_opt, "databaseName",       m_dbname.local8Bit());
  ZOOM_options_set(conn_opt, "user",               m_user.local8Bit());
  ZOOM_options_set(conn_opt, "password",           m_password.local8Bit());
// set by the user
  // don't do this, no way to know if it worked
//  if(m_sourceCharSet.isEmpty()) {
//    ZOOM_options_set(conn_opt, "charset", "UTF-8");
//  }

  ZOOM_connection conn = ZOOM_connection_create(conn_opt);
  ZOOM_connection_connect(conn, m_host.local8Bit(), m_port);

  int errcode;
  const char* errmsg; // unused: carries same info as 'errcode'
  const char* addinfo;
  errcode = ZOOM_connection_error(conn, &errmsg, &addinfo);
  if(errcode != 0) {
    QString s = i18n("Connection error %1: %2").arg(errcode).arg(QString::fromLatin1(addinfo));
    signalStatus(s);
    kdDebug() << s << endl;
    ZOOM_options_destroy(conn_opt);
    ZOOM_connection_destroy(conn);
    stop();
    return;
  }

  ZOOM_query query = ZOOM_query_create();
  errcode = ZOOM_query_prefix(query, m_pqn.local8Bit());
  if(errcode != 0) {
    signalStatus(i18n("Query error!"));
    kdDebug() << "Query error" << endl;
    ZOOM_options_destroy(conn_opt);
    ZOOM_query_destroy(query);
    ZOOM_connection_destroy(conn);
    stop();
    return;
  }

  ZOOM_resultset resultSet = ZOOM_connection_search(conn, query);

  ZOOM_resultset_option_set(resultSet,   "elementSetName",        m_esn.local8Bit());
  ZOOM_resultset_option_set(resultSet,   "count",                 QCString().setNum(m_max));
  if(!m_syntax.isEmpty()) {
    ZOOM_resultset_option_set(resultSet, "preferredRecordSyntax", m_syntax.utf8());
  }
//  ZOOM_resultset_option_set(resultSet, "preferredRecordSyntax", "sutrs");

  errcode = ZOOM_connection_error(conn, &errmsg, &addinfo);
  if(errcode != 0) {
    QString s = i18n("Connection search error %1: %2").arg(errcode).arg(QString::fromLatin1(addinfo));
    signalStatus(s);
    kdDebug() << s << endl;
    ZOOM_options_destroy(conn_opt);
    ZOOM_resultset_destroy(resultSet);
    ZOOM_query_destroy(query);
    ZOOM_connection_destroy(conn);
    stop();
    return;
  }

  QStringList isbnFoundList;
  const size_t size = ZOOM_resultset_size(resultSet);
  const size_t total = KMIN(size, m_max);

  QString newSyntax;
  if(size > 0) {
    int len;
    ZOOM_record rec = ZOOM_resultset_record(resultSet, 0);
    newSyntax = QString::fromLatin1(ZOOM_record_get(rec, "syntax", &len)).lower();
    // default syntax might not be marc at all!
    if(newSyntax.find(QString::fromLatin1("marc")) == -1) {
      ZOOM_resultset_option_set(resultSet, "preferredRecordSyntax", "USMARC");
      rec = ZOOM_resultset_record(resultSet, 0);
      newSyntax = QString::fromLatin1(ZOOM_record_get(rec, "syntax", &len)).lower();
    }

    ZOOM_record_get(rec, "raw", &len);
    if(len == 0) {
      // we're not ok
      if(newSyntax == Latin1Literal("unimarc")) {
        ZOOM_resultset_option_set(resultSet, "preferredRecordSyntax", "USMARC");
      } else { // might be "usmarc" or "marc21"
        ZOOM_resultset_option_set(resultSet, "preferredRecordSyntax", "UNIMARC");
      }
      rec = ZOOM_resultset_record(resultSet, 0);
      newSyntax = QString::fromLatin1(ZOOM_record_get(rec, "syntax", &len)).lower();
      ZOOM_record_get(rec, "raw", &len);
      if(len == 0) {
        signalStatus(i18n("Record syntax error"));
        ZOOM_options_destroy(conn_opt);
        ZOOM_resultset_destroy(resultSet);
        ZOOM_query_destroy(query);
        ZOOM_connection_destroy(conn);
        stop();
        return;
      }
    }
  }

  // save syntax change for next time
  if(m_syntax != newSyntax && !m_syntax.isEmpty() && m_config) {
    m_syntax = newSyntax;
    KConfigGroupSaver groupSaver(m_config, m_configGroup);
    m_config->writeEntry("Syntax", m_syntax);
  }

  for(size_t i = 0; i < total && m_started; ++i) {
    ZOOM_record rec = ZOOM_resultset_record(resultSet, i);
    if(!rec) {
      myDebug() << "Z3950Fetcher::process() - no record returned for index " << i << endl;
      continue;
    }
#if 0
    QCString dataOptions;
    if(m_sourceCharSet.isEmpty()) {
      // take whatever we get
      // TODO: heuristic for checking character set
      // for now, assume we're receiving marc8
      dataOptions = QCString("xml; charset=marc8,utf8");
    } else {
      // limit user input to 64 chars
      dataOptions = QCString("xml; charset=") + m_sourceCharSet.left(64).local8Bit() + QCString(",utf8");
    }
    int len;
//    kdDebug() << ZOOM_record_get(rec, "syntax", &len) << endl;
    QString data = QString::fromUtf8(ZOOM_record_get(rec, dataOptions, &len));
    if(len == 0) {
      kdDebug() << "Z3950Fetcher::process() - empty record found, maybe the character encoding or record format is wrong?" << endl;
      continue;
    }
#else
    int len;
    // limit user input to 64 chars
    QString data = toXML(ZOOM_record_get(rec, "raw", &len),
                         m_sourceCharSet.isEmpty() ? "utf-8" : m_sourceCharSet.left(64).latin1());
    if(len == 0 || data.isEmpty()) {
      myDebug() << "Z3950Fetcher::process() - empty record found, maybe the character encoding or record format is wrong?" << endl;
      continue;
    }
#endif
#if 0
    kdWarning() << "Remove debug from z3950fetcher.cpp: " << i << "::length = " << len << endl;
    QFile f1(QString::fromLatin1("/tmp/marc.xml"));
    if(f1.open(i == 0 ? (IO_WriteOnly) : (IO_WriteOnly | IO_Append))) {
      QTextStream t(&f1);
      t.setEncoding(QTextStream::UnicodeUTF8);
      t << data;
//      t << ZOOM_record_get(rec, "raw", &len);
    }
    f1.close();
#endif
    // assume always utf-8
    QString str = (m_syntax == Latin1Literal("unimarc")) ? s_UNIMARCXMLHandler->applyStylesheet(data)
                                                       : s_MARC21XMLHandler->applyStylesheet(data);
#if 0
    kdWarning() << "Remove debug from z3950fetcher.cpp" << endl;
    QFile f2(QString::fromLatin1("/tmp/mods.xml"));
    if(f2.open(i == 0 ? (IO_WriteOnly) : (IO_WriteOnly | IO_Append))) {
      QTextStream t(&f2);
      t.setEncoding(QTextStream::UnicodeUTF8);
      t << str;
    }
    f2.close();
#endif
    Import::TellicoImporter imp(s_MODSHandler->applyStylesheet(str));
    Data::Collection* coll = imp.collection();
    if(!coll) {
      if(!imp.statusMessage().isEmpty()) {
        signalStatus(imp.statusMessage());
      }
      myDebug() << "Z3950Fetcher::process() - no collection pointer" << endl;
      continue;
    }
    if(coll->entryCount() == 0) {
      myDebug() << "Z3950Fetcher::process() - no Tellico entry for result " << i << endl;
      continue;
    }
    for(Data::EntryVec::ConstIterator entry = coll->entries().begin(); entry != coll->entries().end(); ++entry) {
      if(m_key == Fetch::ISBN) {
        isbnFoundList.append(entry->field(QString::fromLatin1("isbn")));
      }
      QString desc = entry->field(QString::fromLatin1("author")) + '/'
                     + entry->field(QString::fromLatin1("publisher"));
      if(!entry->field(QString::fromLatin1("cr_year")).isEmpty()) {
        desc += QChar('/') + entry->field(QString::fromLatin1("cr_year"));
      } else if(!entry->field(QString::fromLatin1("pub_year")).isEmpty()){
        desc += QChar('/') + entry->field(QString::fromLatin1("pub_year"));
      }
      SearchResult* r = new SearchResult(this, entry->title(), desc);
      m_entries.insert(r->uid, Data::ConstEntryPtr(entry));
      emit signalResultFound(r);
    }
    kapp->processEvents();
  }

  ZOOM_options_destroy(conn_opt);
  ZOOM_resultset_destroy(resultSet);
  ZOOM_query_destroy(query);
  ZOOM_connection_destroy(conn);

  // tell the user if some of his isbn values were not found
  if(m_key == Fetch::ISBN) {
    const QStringList isbnSearchList = QStringList::split(QString::fromLatin1("; "), m_value);
    QStringList isbnNotFound;
    for(QStringList::ConstIterator it = isbnSearchList.begin(); it != isbnSearchList.end(); ++it) {
      if(!isbnFoundList.contains(*it)) {
        isbnNotFound.append(*it);
      }
    }
    if(!isbnNotFound.isEmpty()) {
      qHeapSort(isbnNotFound);
      KMessageBox::informationList(0, i18n("<qt>No entries were found for the following ISBN values:</qt>"),
                                   isbnNotFound);
    }
  }
  stop();
#endif // HAVE_YAZ
}

Tellico::Data::Entry* Z3950Fetcher::fetchEntry(uint uid_) {
  return new Data::Entry(*m_entries[uid_], Data::Document::self()->collection());
}

Tellico::Fetch::ConfigWidget* Z3950Fetcher::configWidget(QWidget* parent_) const {
  return new Z3950Fetcher::ConfigWidget(parent_, this);
}

// static
Tellico::Fetch::Z3950Fetcher* Z3950Fetcher::libraryOfCongress(QObject* parent_) {
  return new Z3950Fetcher(i18n("Library of Congress (US)"), QString::fromLatin1("z3950.loc.gov"), 7090,
                          QString::fromLatin1("Voyager"), QString::null, QString::null,
                          QString::fromLatin1("marc-8"), parent_);
}

Z3950Fetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const Z3950Fetcher* fetcher_/*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(this, 7, 2);
  l->setSpacing(4);
//  l->setAutoAdd(true);

  QLabel* label = new QLabel(i18n("&Host: "), this);
  l->addWidget(label, 0, 0);
  m_hostEdit = new GUI::LineEdit(this);
  connect(m_hostEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_hostEdit, 0, 1);
  QString w = i18n("Enter the host name of the z39.50 server.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_hostEdit, w);
  label->setBuddy(m_hostEdit);

  label = new QLabel(i18n("&Port: "), this);
  l->addWidget(label, 1, 0);
  m_portSpinBox = new KIntSpinBox(0, 999999, 1, Z3950_DEFAULT_PORT, 10, this);
  connect(m_portSpinBox, SIGNAL(valueChanged(int)), SLOT(slotSetModified()));
  l->addWidget(m_portSpinBox, 1, 1);
  w = i18n("Enter the port number of the z39.50 server. The default is %1.").arg(Z3950_DEFAULT_PORT);
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_portSpinBox, w);
  label->setBuddy(m_portSpinBox);

  label = new QLabel(i18n("&Database: "), this);
  l->addWidget(label, 2, 0);
  m_databaseEdit = new GUI::LineEdit(this);
  connect(m_databaseEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_databaseEdit, 2, 1);
  w = i18n("Enter the database name used by the z39.50 server.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_databaseEdit, w);
  label->setBuddy(m_databaseEdit);

  label = new QLabel(i18n("Ch&aracter set: "), this);
  l->addWidget(label, 3, 0);
  m_charSetCombo = new KComboBox(true, this);
  m_charSetCombo->insertItem(QString::null);
  m_charSetCombo->insertItem(QString::fromLatin1("marc8"));
  m_charSetCombo->insertItem(QString::fromLatin1("iso-8859-1"));
  m_charSetCombo->insertItem(QString::fromLatin1("utf-8"));
  connect(m_charSetCombo, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_charSetCombo, 3, 1);
  w = i18n("Enter the character set encoding used by the z39.50 server. The most likely choice "
           "is MARC-8, although ISO-8859-1 is common as well.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_charSetCombo, w);
  label->setBuddy(m_charSetCombo);

  label = new QLabel(i18n("&User: "), this);
  l->addWidget(label, 4, 0);
  m_userEdit = new GUI::LineEdit(this);
  m_userEdit->setHint(i18n("Optional"));
  connect(m_userEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_userEdit, 4, 1);
  w = i18n("Enter the authentication user name used by the z39.50 database. Most servers "
           "do not need one.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_userEdit, w);
  label->setBuddy(m_userEdit);

  label = new QLabel(i18n("Pass&word: "), this);
  l->addWidget(label, 5, 0);
  m_passwordEdit = new GUI::LineEdit(this);
  m_passwordEdit->setHint(i18n("Optional"));
  m_passwordEdit->setEchoMode(QLineEdit::Password);
  connect(m_passwordEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_passwordEdit, 5, 1);
  w = i18n("Enter the authentication password used by the z39.50 database. Most servers "
           "do not need one.");
  w += ' ' + i18n("The password will be saved in plain text in the Tellico configuration file.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_passwordEdit, w);
  label->setBuddy(m_passwordEdit);

  l->setRowStretch(6, 1);

  if(fetcher_) {
    m_hostEdit->setText(fetcher_->m_host);
    m_portSpinBox->setValue(fetcher_->m_port);
    m_databaseEdit->setText(fetcher_->m_dbname);
    m_userEdit->setText(fetcher_->m_user);
    m_passwordEdit->setText(fetcher_->m_password);
    m_charSetCombo->setCurrentText(fetcher_->m_sourceCharSet);
  }
}

void Z3950Fetcher::ConfigWidget::saveConfig(KConfig* config_) {
  QString s = m_hostEdit->text();
  if(!s.isEmpty()) {
    config_->writeEntry("Host", s);
  }
  int port = m_portSpinBox->value();
  if(port > 0) {
    config_->writeEntry("Port", port);
  }
  s = m_databaseEdit->text();
  if(!s.isEmpty()) {
    config_->writeEntry("Database", s);
  }
  s = m_charSetCombo->currentText();
  config_->writeEntry("Charset", s); // an empty charset is ok
  s = m_userEdit->text();
  config_->writeEntry("User", s); // empty user is ok
  s = m_passwordEdit->text();
  config_->writeEntry("Password", s); // empty password is ok
  slotSetModified(false);
}

#include "z3950fetcher.moc"
