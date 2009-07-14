/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#include "z3950connection.h"
#include "z3950fetcher.h"
#include "messagehandler.h"
#include "../tellico_debug.h"
#include "../utils/iso5426converter.h"
#include "../utils/iso6937converter.h"

#include <config.h>

#include <klocale.h>
#include <kapplication.h>
#include <KConfigGroup>

#include <QFile>

#ifdef HAVE_YAZ
extern "C" {
#include <yaz/zoom.h>
#include <yaz/marcdisp.h>
#include <yaz/yaz-version.h>
}
#endif

namespace {
  static const size_t Z3950_DEFAULT_MAX_RECORDS = 20;

#ifdef HAVE_YAZ
  class QueryDestroyer {
  public:
    QueryDestroyer(ZOOM_query query_) : query(query_) {}
    ~QueryDestroyer() { if(query) ZOOM_query_destroy(query); }
  private:
    ZOOM_query query;
  };

  class ResultDestroyer {
  public:
    ResultDestroyer(ZOOM_resultset result_) : result(result_) {}
    ~ResultDestroyer() { if(result) ZOOM_resultset_destroy(result); }
  private:
    ZOOM_resultset result;
  };

  class YazCloser {
  public:
    YazCloser(yaz_iconv_t iconv_) : iconv(iconv_), marc(0) {}
    YazCloser(yaz_iconv_t iconv_, yaz_marc_t marc_) : iconv(iconv_), marc(marc_) {}
    ~YazCloser() {
      if(iconv) yaz_iconv_close(iconv);
      if(marc) yaz_marc_destroy(marc);
    }
  private:
    yaz_iconv_t iconv;
    yaz_marc_t marc;
  };
#endif
}

using Tellico::Fetch::Z3950ResultFound;
using Tellico::Fetch::Z3950Connection;

Z3950ResultFound::Z3950ResultFound(const QString& s) : QEvent(uid())
    , m_result(s) {
  ++Z3950Connection::resultsLeft;
}

Z3950ResultFound::~Z3950ResultFound() {
  --Z3950Connection::resultsLeft;
}

class Z3950Connection::Private {
public:
  Private() {}
#ifdef HAVE_YAZ
  ~Private() {
    ZOOM_options_destroy(conn_opt);
    ZOOM_connection_destroy(conn);
  };

  ZOOM_options conn_opt;
  ZOOM_connection conn;
#endif
};

int Z3950Connection::resultsLeft = 0;

// since the character set goes into a yaz api call
// I'm paranoid about user insertions, so just grab 64
// characters at most
Z3950Connection::Z3950Connection(Tellico::Fetch::Z3950Fetcher* fetcher,
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
    , m_host(host)
    , m_port(port)
    , m_dbname(dbname)
    , m_sourceCharSet(sourceCharSet.left(64))
    , m_syntax(syntax)
    , m_esn(esn)
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
  m_pqn = query_;
}

void Z3950Connection::setUserPassword(const QString& user_, const QString& pword_) {
  m_user = user_;
  m_password = pword_;
}

void Z3950Connection::run() {
//  myDebug() << m_fetcher->source();
  m_aborted = false;
  m_hasMore = false;
  resultsLeft = 0;
#ifdef HAVE_YAZ

  if(!makeConnection()) {
    done();
    return;
  }

  ZOOM_query query = ZOOM_query_create();
  QueryDestroyer qd(query);

  int errcode = ZOOM_query_prefix(query, toCString(m_pqn));
  if(errcode != 0) {
    myDebug() << "query error: " << m_pqn;
    QString s = i18n("Query error!");
    s += QLatin1Char(' ') + m_pqn;
    done(s, MessageHandler::Error);
    return;
  }

  ZOOM_resultset resultSet = ZOOM_connection_search(d->conn, query);
  ResultDestroyer rd(resultSet);

  // check abort status
  if(m_aborted) {
    done();
    return;
  }

  // I know the LOC wants the syntax = "xml" and esn = "mods"
  // to get MODS data, that seems a bit odd...
  // esn only makes sense for marc and grs-1
  // if syntax is mods, set esn to mods too
  QByteArray type = "raw";
  if(m_syntax == QLatin1String("mods")) {
    m_syntax = QLatin1String("xml");
    ZOOM_resultset_option_set(resultSet, "elementSetName", "mods");
    type = "xml";
  } else {
    ZOOM_resultset_option_set(resultSet, "elementSetName", m_esn.toLatin1());
  }
  ZOOM_resultset_option_set(resultSet, "start", QByteArray::number(static_cast<int>(m_start)));
  ZOOM_resultset_option_set(resultSet, "count", QByteArray::number(static_cast<int>(m_limit-m_start)));
  // search in default syntax, unless syntax is already set
  if(!m_syntax.isEmpty()) {
    ZOOM_resultset_option_set(resultSet, "preferredRecordSyntax", m_syntax.toLatin1());
  }

  const char* errmsg;
  const char* addinfo;
  errcode = ZOOM_connection_error(d->conn, &errmsg, &addinfo);
  if(errcode != 0) {
    m_connected = false;

    QString s = i18n("Connection search error %1: %2", errcode, toString(errmsg));
    if(!QByteArray(addinfo).isEmpty()) {
      s += QLatin1String(" (") + toString(addinfo) + QLatin1Char(')');
    }
    myDebug() << QString::fromLatin1("[%1/%2]").arg(m_host, m_dbname) << s;
    done(s, MessageHandler::Error);
    return;
  }

  const size_t numResults = ZOOM_resultset_size(resultSet);

  QString newSyntax = m_syntax;
  if(numResults > 0) {
    myLog() << "current syntax is " << m_syntax << " (" << numResults << " results)";
    // so now we know that results exist, might have to check syntax
    int len;
    ZOOM_record rec = ZOOM_resultset_record(resultSet, 0);
    // want raw unless it's mods
    ZOOM_record_get(rec, type, &len);
    if(len > 0 && m_syntax.isEmpty()) {
      newSyntax = QString::fromLatin1(ZOOM_record_get(rec, "syntax", &len)).toLower();
      myLog() << "syntax guess is " << newSyntax;
      if(newSyntax == QLatin1String("mods") || newSyntax == QLatin1String("xml")) {
        m_syntax = QLatin1String("xml");
        ZOOM_resultset_option_set(resultSet, "elementSetName", "mods");
      } else if(newSyntax == QLatin1String("grs-1")) {
        // if it's defaulting to grs-1, go ahead and change it to try to get a marc
        // record since grs-1 is a last resort for us
        newSyntax.clear();
      }
    }
    // right now, we just understand mods, unimarc, marc21/usmarc, and grs-1
    if(newSyntax != QLatin1String("xml") &&
       newSyntax != QLatin1String("usmarc") &&
       newSyntax != QLatin1String("marc21") &&
       newSyntax != QLatin1String("unimarc") &&
       newSyntax != QLatin1String("grs-1")) {
      myLog() << "changing z39.50 syntax to MODS";
      newSyntax = QLatin1String("xml");
      ZOOM_resultset_option_set(resultSet, "elementSetName", "mods");
      ZOOM_resultset_option_set(resultSet, "preferredRecordSyntax", newSyntax.toLatin1());
      rec = ZOOM_resultset_record(resultSet, 0);
      ZOOM_record_get(rec, "xml", &len);
      if(len == 0) {
        // change set name back
        ZOOM_resultset_option_set(resultSet, "elementSetName", m_esn.toLatin1());
        newSyntax = QLatin1String("usmarc"); // try usmarc
        myLog() << "changing z39.50 syntax to USMARC";
        ZOOM_resultset_option_set(resultSet, "preferredRecordSyntax", newSyntax.toLatin1());
        rec = ZOOM_resultset_record(resultSet, 0);
        ZOOM_record_get(rec, "raw", &len);
      }
      if(len == 0) {
        newSyntax = QLatin1String("marc21"); // try marc21
        myLog() << "changing z39.50 syntax to MARC21";
        ZOOM_resultset_option_set(resultSet, "preferredRecordSyntax", newSyntax.toLatin1());
        rec = ZOOM_resultset_record(resultSet, 0);
        ZOOM_record_get(rec, "raw", &len);
      }
      if(len == 0) {
        newSyntax = QLatin1String("unimarc"); // try unimarc
        myLog() << "changing z39.50 syntax to UNIMARC";
        ZOOM_resultset_option_set(resultSet, "preferredRecordSyntax", newSyntax.toLatin1());
        rec = ZOOM_resultset_record(resultSet, 0);
        ZOOM_record_get(rec, "raw", &len);
      }
      if(len == 0) {
        newSyntax = QLatin1String("grs-1"); // try grs-1
        myLog() << "changing z39.50 syntax to GRS-1";
        ZOOM_resultset_option_set(resultSet, "preferredRecordSyntax", newSyntax.toLatin1());
        rec = ZOOM_resultset_record(resultSet, 0);
        ZOOM_record_get(rec, "raw", &len);
      }
      if(len == 0) {
        myLog() << "giving up";
        done(i18n("Record syntax error"), MessageHandler::Error);
        return;
      }
      myLog() << "final syntax is " << newSyntax;
    }
  }

  // go back to fooling ourselves and calling it mods
  if(m_syntax == QLatin1String("xml")) {
    m_syntax = QLatin1String("mods");
  }
  if(newSyntax == QLatin1String("xml")) {
    newSyntax = QLatin1String("mods");
  }
  // save syntax change for next time
  if(m_syntax != newSyntax) {
    kapp->postEvent(m_fetcher.data(), new Z3950SyntaxChange(newSyntax));
    m_syntax = newSyntax;
  }

  if(m_sourceCharSet.isEmpty()) {
    m_sourceCharSet = QLatin1String("marc-8");
  }

  const size_t realLimit = qMin(numResults, m_limit);

  for(size_t i = m_start; i < realLimit && !m_aborted; ++i) {
//    myLog() << "grabbing index" << i;
    ZOOM_record rec = ZOOM_resultset_record(resultSet, i);
    if(!rec) {
      myDebug() << "no record returned for index" << i;
      continue;
    }
    int len;
    QString data;
    if(m_syntax == QLatin1String("mods")) {
      data = toString(ZOOM_record_get(rec, "xml", &len));
    } else if(m_syntax == QLatin1String("grs-1")) { // grs-1
      // we're going to parse the rendered data, very ugly...
      data = toString(ZOOM_record_get(rec, "render", &len));
    } else {
#if 0
      myWarning() << "Remove debug from z3950connection.cpp";
      {
        QFile f1(QLatin1String("/tmp/z3950.raw"));
        if(f1.open(QIODevice::WriteOnly)) {
          QDataStream t(&f1);
          t << ZOOM_record_get(rec, "raw", &len);
        }
        f1.close();
      }
#endif
      data = toXML(ZOOM_record_get(rec, "raw", &len), m_sourceCharSet);
    }
    Z3950ResultFound* ev = new Z3950ResultFound(data);
    QApplication::postEvent(m_fetcher.data(), ev);
  }

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
//  myDebug() << m_fetcher->source();
// I don't know what to do except assume database, user, and password are in locale encoding
#ifdef HAVE_YAZ
  d->conn_opt = ZOOM_options_create();
  ZOOM_options_set(d->conn_opt, "implementationName", "Tellico");
  ZOOM_options_set(d->conn_opt, "databaseName",       toCString(m_dbname));
  ZOOM_options_set(d->conn_opt, "user",               toCString(m_user));
  ZOOM_options_set(d->conn_opt, "password",           toCString(m_password));

  d->conn = ZOOM_connection_create(d->conn_opt);
  ZOOM_connection_connect(d->conn, m_host.toLatin1(), m_port);

  int errcode;
  const char* errmsg; // unused: carries same info as 'errcode'
  const char* addinfo;
  errcode = ZOOM_connection_error(d->conn, &errmsg, &addinfo);
  if(errcode != 0) {
    ZOOM_options_destroy(d->conn_opt);
    ZOOM_connection_destroy(d->conn);
    m_connected = false;

    QString s = i18n("Connection error %1: %2", errcode, toString(errmsg));
    if(!QByteArray(addinfo).isEmpty()) {
      s += QLatin1String(" (") + toString(addinfo) + QLatin1Char(')');
    }
    myDebug() << QString::fromLatin1("[%1/%2]").arg(m_host, m_dbname) << s;
    done(s, MessageHandler::Error);
    return false;
  }
#endif
  m_connected = true;
  return true;
}

void Z3950Connection::done() {
  checkPendingEvents();
  kapp->postEvent(m_fetcher.data(), new Z3950ConnectionDone(m_hasMore));
}

void Z3950Connection::done(const QString& msg_, int type_) {
  checkPendingEvents();
  if(m_aborted) {
    kapp->postEvent(m_fetcher.data(), new Z3950ConnectionDone(m_hasMore));
  } else {
    kapp->postEvent(m_fetcher.data(), new Z3950ConnectionDone(m_hasMore, msg_, type_));
  }
}

void Z3950Connection::checkPendingEvents() {
  // if there's still some pending result events, go ahead and just wait 1 second
  if(resultsLeft > 0) {
    sleep(1);
  }
}

inline
QByteArray Z3950Connection::toCString(const QString& text_) {
  return iconvRun(text_.toUtf8(), QLatin1String("utf-8"), m_sourceCharSet);
}

inline
QString Z3950Connection::toString(const QByteArray& text_) {
  return QString::fromUtf8(iconvRun(text_, m_sourceCharSet, QLatin1String("utf-8")));
}

// static
QByteArray Z3950Connection::iconvRun(const QByteArray& text_, const QString& fromCharSet_, const QString& toCharSet_) {
#ifdef HAVE_YAZ
  if(text_.isEmpty()) {
    return text_;
  }

  if(fromCharSet_ == toCharSet_) {
    return text_;
  }

  yaz_iconv_t cd = yaz_iconv_open(toCharSet_.toLatin1(), fromCharSet_.toLatin1());
  if(!cd) {
    // maybe it's iso 5426, which we sorta support
    QString charSetLower = fromCharSet_.toLower();
    charSetLower.remove(QLatin1Char('-')).remove(QLatin1Char(' '));
    if(charSetLower == QLatin1String("iso5426")) {
      return iconvRun(Iso5426Converter::toUtf8(text_).toUtf8(), QLatin1String("utf-8"), toCharSet_);
    } else if(charSetLower == QLatin1String("iso6937")) {
      return iconvRun(Iso6937Converter::toUtf8(text_).toUtf8(), QLatin1String("utf-8"), toCharSet_);
    }
    myWarning() << "conversion from" << fromCharSet_
                << "to" << toCharSet_ << "is unsupported";
    return text_;
  }

  YazCloser closer(cd);

  const char* input = text_;
  size_t inlen = text_.length();

  size_t outlen = 2 * inlen;  // this is enough, right?
  QVector<char> result0(outlen);
  char* result = result0.data();

  int r = yaz_iconv(cd, const_cast<char**>(&input), &inlen, &result, &outlen);
  if(r <= 0) {
    myDebug() << "can't decode buffer";
    return text_;
  }
  // bug in yaz, need to flush buffer to catch last character
  yaz_iconv(cd, 0, 0, &result, &outlen);

  // length is pointer difference
  int len = result - result0.data();

  QByteArray output(result0.data(), len+1);
//  myDebug() << "-------------------------------------------";
//  myDebug() << output;
//  myDebug() << "-------------------------------------------";
  return output;
#endif
  return text_;
}

QString Z3950Connection::toXML(const QByteArray& marc_, const QString& charSet_) {
#ifdef HAVE_YAZ
  if(marc_.isEmpty()) {
    myDebug() << "empty string";
    return QString();
  }

  yaz_iconv_t cd = yaz_iconv_open("utf-8", charSet_.toLatin1());
  if(!cd) {
    // maybe it's iso 5426, which we sorta support
    QString charSetLower = charSet_.toLower();
    charSetLower.remove(QLatin1Char('-')).remove(QLatin1Char(' '));
    if(charSetLower == QLatin1String("iso5426")) {
      return toXML(Iso5426Converter::toUtf8(marc_).toUtf8(), QLatin1String("utf-8"));
    } else if(charSetLower == QLatin1String("iso6937")) {
      return toXML(Iso6937Converter::toUtf8(marc_).toUtf8(), QLatin1String("utf-8"));
    }
    myWarning() << "conversion from " << charSet_ << " is unsupported";
    return QString();
  }

  yaz_marc_t mt = yaz_marc_create();
  yaz_marc_iconv(mt, cd);
  yaz_marc_xml(mt, YAZ_MARC_MARCXML);

  YazCloser closer(cd, mt);

  // first 5 bytes are length
  bool ok;
#if YAZ_VERSIONL < 0x030000
  int len = marc_.left(5).toInt(&ok);
#else
  size_t len = marc_.left(5).toInt(&ok);
#endif
  if(ok && (len < 25 || len > 100000)) {
    myDebug() << "bad length: " << (ok ? len : -1);
    return QString();
  }

#if YAZ_VERSIONL < 0x030000
  char* result;
#else
  const char* result;
#endif
  int r = yaz_marc_decode_buf(mt, marc_, -1, &result, &len);
  if(r <= 0) {
    myDebug() << "can't decode buffer";
    return QString();
  }

  QString output = QLatin1String("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  output += QString::fromUtf8(QByteArray(result, len+1), len+1);
//  myDebug() << QCString(result);
//  myDebug() << "-------------------------------------------";
//  myDebug() << output;

  return output;
#else // no yaz
  return QString();
#endif
}
