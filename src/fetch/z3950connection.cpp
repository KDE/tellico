/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : $EMAIL
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "z3950connection.h"
#include "z3950fetcher.h"
#include "messagehandler.h"
#include "../latin1literal.h"
#include "../tellico_debug.h"
#include "../iso5426converter.h"
#include "../iso6937converter.h"

#include <config.h>

#if HAVE_YAZ
extern "C" {
#include <yaz/zoom.h>
#include <yaz/marcdisp.h>
}
#endif

#include <klocale.h>

namespace {
  static const size_t Z3950_DEFAULT_MAX_RECORDS = 20;
}

using Tellico::Fetch::Z3950Connection;

class Z3950Connection::Private {
public:
  Private() {}
#if HAVE_YAZ
  ~Private() {
    ZOOM_options_destroy(conn_opt);
    ZOOM_connection_destroy(conn);
  };

  ZOOM_options conn_opt;
  ZOOM_connection conn;
#endif
};

// since the character set goes into a yaz api call
// I'm paranoid about user insertions, so just grab 64
// characters at most
Z3950Connection::Z3950Connection(Z3950Fetcher* fetcher,
                                 const QString& host,
                                 uint port,
                                 const QString& dbname,
                                 const QString& sourceCharSet,
                                 const QString& syntax,
                                 const QString& esn)
    : QThread()
    , d(new Private())
    , m_connected(false)
    , m_aborted(false)
    , m_fetcher(fetcher)
    , m_host(QDeepCopy<QString>(host))
    , m_port(port)
    , m_dbname(QDeepCopy<QString>(dbname))
    , m_sourceCharSet(QDeepCopy<QString>(sourceCharSet.left(64)))
    , m_syntax(QDeepCopy<QString>(syntax))
    , m_esn(QDeepCopy<QString>(esn))
    , m_start(0)
    , m_limit(Z3950_DEFAULT_MAX_RECORDS)
    , m_hasMore(false) {
}

Z3950Connection::~Z3950Connection() {
  m_connected = false;
  delete d;
  d = 0;
}

void Z3950Connection::reset() {
  m_start = 0;
  m_limit = Z3950_DEFAULT_MAX_RECORDS;
}

void Z3950Connection::setQuery(const QString& query_) {
  m_pqn = QDeepCopy<QString>(query_);
}

void Z3950Connection::setUserPassword(const QString& user_, const QString& pword_) {
  m_user = QDeepCopy<QString>(user_);
  m_password = QDeepCopy<QString>(pword_);
}

void Z3950Connection::run() {
//  myDebug() << "Z3950Connection::run() - " << m_fetcher->source() << endl;
  m_aborted = false;
  m_hasMore = false;
#if HAVE_YAZ

  if(!makeConnection()) {
    done();
    return;
  }

  ZOOM_query query = ZOOM_query_create();
  int errcode = ZOOM_query_prefix(query, toCString(m_pqn));
  if(errcode != 0) {
    myDebug() << "Z3950Connection::run() - query error: " << m_pqn << endl;
    ZOOM_query_destroy(query);
    QString s = i18n("Query error!");
    s += '\n' + m_pqn;
    done(s, MessageHandler::Error);
    return;
  }

  ZOOM_resultset resultSet = ZOOM_connection_search(d->conn, query);

  // check abort status
  if(m_aborted) {
    done();
    return;
  }

  // I know the LOC wants the syntax = "xml" and esn = "mods"
  // to get MODS data, that seems a bit odd...
  // esn only makes sense for marc and grs-1
  // if syntax is mods, set esn to mods too
  QCString type = "raw";
  if(m_syntax == Latin1Literal("mods")) {
    m_syntax = QString::fromLatin1("xml");
    ZOOM_resultset_option_set(resultSet, "elementSetName", "mods");
    type = "xml";
  } else {
    ZOOM_resultset_option_set(resultSet, "elementSetName", m_esn.latin1());
  }
  ZOOM_resultset_option_set(resultSet, "start", QCString().setNum(m_start));
  ZOOM_resultset_option_set(resultSet, "count", QCString().setNum(m_limit-m_start));
  // search in default syntax, unless syntax is already set
  if(!m_syntax.isEmpty()) {
    ZOOM_resultset_option_set(resultSet, "preferredRecordSyntax", m_syntax.latin1());
  }

  const char* errmsg;
  const char* addinfo;
  errcode = ZOOM_connection_error(d->conn, &errmsg, &addinfo);
  if(errcode != 0) {
    ZOOM_resultset_destroy(resultSet);
    ZOOM_query_destroy(query);
    m_connected = false;

    QString s = i18n("Connection search error %1: %2").arg(errcode).arg(toString(errmsg));
    if(!QCString(addinfo).isEmpty()) {
      s += " (" + toString(addinfo) + ")";
    }
    myDebug() << "Z3950Connection::run() - " << s << endl;
    done(s, MessageHandler::Error);
    return;
  }

  const size_t numResults = ZOOM_resultset_size(resultSet);

  QString newSyntax = m_syntax;
  if(numResults > 0) {
    myLog() << "Z3950Connection::run() - current syntax is " << newSyntax << " (" << numResults << " results)" << endl;
    // so now we know that results exist, might have to check syntax
    int len;
    ZOOM_record rec = ZOOM_resultset_record(resultSet, 0);
    // want raw unless it's mods
    ZOOM_record_get(rec, type, &len);
    if(len > 0 && m_syntax.isEmpty()) {
      newSyntax = QString::fromLatin1(ZOOM_record_get(rec, "syntax", &len)).lower();
      myLog() << "Z3950Connection::run() - syntax guess is " << newSyntax << endl;
      if(newSyntax == Latin1Literal("mods") || newSyntax == Latin1Literal("xml")) {
        m_syntax = QString::fromLatin1("xml");
        ZOOM_resultset_option_set(resultSet, "elementSetName", "mods");
      } else if(newSyntax == Latin1Literal("grs-1")) {
        // if it's defaulting to grs-1, go ahead and change it to try to get a marc
        // record since grs-1 is a last resort for us
        newSyntax.truncate(0);
      }
    }
    // right now, we just understand mods, unimarc, marc21/usmarc, and grs-1
    if(newSyntax != Latin1Literal("xml") &&
       newSyntax != Latin1Literal("usmarc") &&
       newSyntax != Latin1Literal("marc21") &&
       newSyntax != Latin1Literal("unimarc") &&
       newSyntax != Latin1Literal("grs-1")) {
      myLog() << "Z3950Connection::run() - changing z39.50 syntax to MODS" << endl;
      newSyntax = QString::fromLatin1("xml");
      ZOOM_resultset_option_set(resultSet, "elementSetName", "mods");
      ZOOM_resultset_option_set(resultSet, "preferredRecordSyntax", newSyntax.latin1());
      rec = ZOOM_resultset_record(resultSet, 0);
      ZOOM_record_get(rec, "xml", &len);
      if(len == 0) {
        // change set name back
        ZOOM_resultset_option_set(resultSet, "elementSetName", m_esn.latin1());
        newSyntax = QString::fromLatin1("usmarc"); // try usmarc
        myLog() << "Z3950Connection::run() - changing z39.50 syntax to USMARC" << endl;
        ZOOM_resultset_option_set(resultSet, "preferredRecordSyntax", newSyntax.latin1());
        rec = ZOOM_resultset_record(resultSet, 0);
        ZOOM_record_get(rec, "raw", &len);
      }
      if(len == 0) {
        newSyntax = QString::fromLatin1("marc21"); // try marc21
        myLog() << "Z3950Connection::run() - changing z39.50 syntax to MARC21" << endl;
        ZOOM_resultset_option_set(resultSet, "preferredRecordSyntax", newSyntax.latin1());
        rec = ZOOM_resultset_record(resultSet, 0);
        ZOOM_record_get(rec, "raw", &len);
      }
      if(len == 0) {
        newSyntax = QString::fromLatin1("unimarc"); // try unimarc
        myLog() << "Z3950Connection::run() - changing z39.50 syntax to UNIMARC" << endl;
        ZOOM_resultset_option_set(resultSet, "preferredRecordSyntax", newSyntax.latin1());
        rec = ZOOM_resultset_record(resultSet, 0);
        ZOOM_record_get(rec, "raw", &len);
      }
      if(len == 0) {
        newSyntax = QString::fromLatin1("grs-1"); // try grs-1
        myLog() << "Z3950Connection::run() - changing z39.50 syntax to GRS-1" << endl;
        ZOOM_resultset_option_set(resultSet, "preferredRecordSyntax", newSyntax.latin1());
        rec = ZOOM_resultset_record(resultSet, 0);
        ZOOM_record_get(rec, "raw", &len);
      }
      if(len == 0) {
        myLog() << "Z3950Connection::run() - giving up" << endl;
        ZOOM_resultset_destroy(resultSet);
        ZOOM_query_destroy(query);
        done(i18n("Record syntax error"), MessageHandler::Error);
        return;
      }
      myLog() << "Z3950Connection::run() - final syntax is " << newSyntax << endl;
    }
  }

  // go back to fooling ourselves and calling it mods
  if(m_syntax == Latin1Literal("xml")) {
    m_syntax = QString::fromLatin1("mods");
  }
  if(newSyntax == Latin1Literal("xml")) {
    newSyntax = QString::fromLatin1("mods");
  }
  // save syntax change for next time
  if(m_syntax != newSyntax) {
    kapp->postEvent(m_fetcher, new Z3950SyntaxChange(newSyntax));
    m_syntax = newSyntax;
  }

  if(m_sourceCharSet.isEmpty()) {
    m_sourceCharSet = QString::fromLatin1("marc-8");
  }

  const size_t realLimit = QMIN(numResults, m_limit);

  for(size_t i = m_start; i < realLimit && !m_aborted; ++i) {
    ZOOM_record rec = ZOOM_resultset_record(resultSet, i);
    if(!rec) {
      myDebug() << "Z3950Fetcher::process() - no record returned for index " << i << endl;
      continue;
    }
    int len;
    QString data;
    if(m_syntax == Latin1Literal("mods")) {
      // we're going to parse the rendered data, very ugly...
      data = toString(ZOOM_record_get(rec, "xml", &len));
    } else if(m_syntax == Latin1Literal("grs-1")) { // grs-1
      // we're going to parse the rendered data, very ugly...
      data = toString(ZOOM_record_get(rec, "render", &len));
    } else {
      data = toXML(ZOOM_record_get(rec, "raw", &len), m_sourceCharSet);
    }
    Z3950ResultFound ev(data);
    kapp->sendEvent(m_fetcher, &ev);
  }

  ZOOM_resultset_destroy(resultSet);
  ZOOM_query_destroy(query);

  m_hasMore = m_limit < numResults;
  if(m_hasMore) {
    m_start = m_limit;
    m_limit += Z3950_DEFAULT_MAX_RECORDS;
  }
#endif
  done();
}

bool Z3950Connection::makeConnection() {
  if(m_connected) {
    return true;
  }
//  myDebug() << "Z3950Connection::makeConnection() - " << m_fetcher->source() << endl;
// I don't know what to do except assume database, user, and password are in locale encoding
#if HAVE_YAZ
  d->conn_opt = ZOOM_options_create();
  ZOOM_options_set(d->conn_opt, "implementationName", "Tellico");
  ZOOM_options_set(d->conn_opt, "databaseName",       toCString(m_dbname));
  ZOOM_options_set(d->conn_opt, "user",               toCString(m_user));
  ZOOM_options_set(d->conn_opt, "password",           toCString(m_password));

  d->conn = ZOOM_connection_create(d->conn_opt);
  ZOOM_connection_connect(d->conn, m_host.latin1(), m_port);

  int errcode;
  const char* errmsg; // unused: carries same info as 'errcode'
  const char* addinfo;
  errcode = ZOOM_connection_error(d->conn, &errmsg, &addinfo);
  if(errcode != 0) {
    ZOOM_options_destroy(d->conn_opt);
    ZOOM_connection_destroy(d->conn);
    m_connected = false;

    QString s = i18n("Connection error %1: %2").arg(errcode).arg(toString(errmsg));
    if(!QCString(addinfo).isEmpty()) {
      s += " (" + toString(addinfo) + ")";
    }
    myDebug() << "Z3950Connection::makeConnection() - " << s << endl;
    done(s, MessageHandler::Error);
    return false;
  }
#endif
  m_connected = true;
  return true;
}

void Z3950Connection::done() {
  kapp->postEvent(m_fetcher, new Z3950ConnectionDone(m_hasMore));
}

void Z3950Connection::done(const QString& msg_, int type_) {
  if(m_aborted) {
    kapp->postEvent(m_fetcher, new Z3950ConnectionDone(m_hasMore));
  } else {
    kapp->postEvent(m_fetcher, new Z3950ConnectionDone(m_hasMore, msg_, type_));
  }
}

inline
QCString Z3950Connection::toCString(const QString& text_) {
  return iconvRun(text_.utf8(), QString::fromLatin1("utf-8"), m_sourceCharSet);
}

inline
QString Z3950Connection::toString(const QCString& text_) {
  return QString::fromUtf8(iconvRun(text_, m_sourceCharSet, QString::fromLatin1("utf-8")));
}

// static
QCString Z3950Connection::iconvRun(const QCString& text_, const QString& fromCharSet_, const QString& toCharSet_) {
#if HAVE_YAZ
  if(text_.isEmpty()) {
    return text_;
  }

  if(fromCharSet_ == toCharSet_) {
    return text_;
  }

  yaz_iconv_t cd = yaz_iconv_open(toCharSet_.latin1(), fromCharSet_.latin1());
  if(!cd) {
    // maybe it's iso 5426, which we sorta support
    QString charSetLower = fromCharSet_.lower();
    charSetLower.remove('-').remove(' ');
    if(charSetLower == Latin1Literal("iso5426")) {
      return iconvRun(Iso5426Converter::toUtf8(text_).utf8(), QString::fromLatin1("utf-8"), toCharSet_);
    } else if(charSetLower == Latin1Literal("iso6937")) {
      return iconvRun(Iso6937Converter::toUtf8(text_).utf8(), QString::fromLatin1("utf-8"), toCharSet_);
    }
    kdWarning() << "Z3950Fetcher::iconvRun() - conversion from " << fromCharSet_
                << " to " << toCharSet_ << " is unsupported" << endl;
    return text_;
  }

  const char* input = text_;
  size_t inlen = text_.length();

  size_t outlen = 2 * inlen;  // this is enough, right?
  char result0[outlen];
  char* result = result0;

  int r = yaz_iconv(cd, const_cast<char**>(&input), &inlen, &result, &outlen);
  if(r <= 0) {
    myDebug() << "Z3950Fetcher::iconvRun() - can't decode buffer" << endl;
    return text_;
  }

  // length is pointer difference
  size_t len = result - result0;

  QCString output = QCString(result0, len+1);
//  myDebug() << "-------------------------------------------" << endl;
//  myDebug() << output << endl;
//  myDebug() << "-------------------------------------------" << endl;
  yaz_iconv_close(cd);
  return output;
#endif
  return text_;
}

QString Z3950Connection::toXML(const QCString& marc_, const QString& charSet_) {
#if HAVE_YAZ
  if(marc_.isEmpty()) {
    myDebug() << "Z3950Fetcher::toXML() - empty string" << endl;
    return QString::null;
  }

  yaz_iconv_t cd = yaz_iconv_open("utf-8", charSet_.latin1());
  if(!cd) {
    // maybe it's iso 5426, which we sorta support
    QString charSetLower = charSet_.lower();
    charSetLower.remove('-').remove(' ');
    if(charSetLower == Latin1Literal("iso5426")) {
      return toXML(Iso5426Converter::toUtf8(marc_).utf8(), QString::fromLatin1("utf-8"));
    } else if(charSetLower == Latin1Literal("iso6937")) {
      return toXML(Iso6937Converter::toUtf8(marc_).utf8(), QString::fromLatin1("utf-8"));
    }
    kdWarning() << "Z3950Fetcher::toXML() - conversion from " << charSet_ << " is unsupported" << endl;
    return QString::null;
  }

  yaz_marc_t mt = yaz_marc_create();
  yaz_marc_iconv(mt, cd);
  yaz_marc_xml(mt, YAZ_MARC_MARCXML);

  // first 5 bytes are length
  bool ok;
  int len = marc_.left(5).toInt(&ok);
  if(ok && (len < 25 || len > 100000)) {
    myDebug() << "Z3950Fetcher::toXML() - bad length: " << (ok ? len : -1) << endl;
    return QString::null;
  }

  char* result;
  int r = yaz_marc_decode_buf(mt, marc_, -1, &result, &len);
  if(r <= 0) {
    myDebug() << "Z3950Fetcher::toXML() - can't decode buffer" << endl;
    return QString::null;
  }

  QString output = QString::fromLatin1("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  output += QString::fromUtf8(QCString(result, len+1), len+1);
//  myDebug() << QCString(result) << endl;
//  myDebug() << "-------------------------------------------" << endl;
//  myDebug() << output << endl;
  yaz_iconv_close(cd);
  yaz_marc_destroy(mt);

  return output;
#else // no yaz
  return QString::null;
#endif
}
