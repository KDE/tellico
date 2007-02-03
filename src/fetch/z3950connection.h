/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_FETCH_Z3950CONNECTION_H
#define TELLICO_FETCH_Z3950CONNECTION_H

#include <qthread.h>
#include <qevent.h>
#include <qdeepcopy.h>

#include <ksharedptr.h>

namespace Tellico {
  namespace Fetch {
    class Z3950Fetcher;

class Z3950ResultFound : public QCustomEvent {
public:
  Z3950ResultFound(const QString& s) : QCustomEvent(uid()), m_result(QDeepCopy<QString>(s)) {}
  const QString& result() const { return m_result; }

  static int uid() { return User + 11111; }

private:
  QString m_result;
};

class Z3950ConnectionDone : public QCustomEvent {
public:
  Z3950ConnectionDone(bool more) : QCustomEvent(uid()), m_type(-1), m_hasMore(more) {}
  Z3950ConnectionDone(bool more, const QString& s, int t) : QCustomEvent(uid()), m_msg(QDeepCopy<QString>(s)), m_type(t), m_hasMore(more) {}

  const QString& message() const { return m_msg; }
  int messageType() const { return m_type; }
  bool hasMoreResults() const { return m_hasMore; }

  static int uid() { return User + 22222; }

private:
  QString m_msg;
  int m_type;
  bool m_hasMore;
};

class Z3950SyntaxChange : public QCustomEvent {
public:
  Z3950SyntaxChange(const QString& s) : QCustomEvent(uid()), m_syntax(QDeepCopy<QString>(s)) {}
  const QString& syntax() const { return m_syntax; }

  static int uid() { return User + 33333; }

private:
  QString m_syntax;
};

/**
 * @author Robby Stephenson
 */
class Z3950Connection : public QThread {
public:
  Z3950Connection(Z3950Fetcher* fetcher,
                  const QString& host,
                  uint port,
                  const QString& dbname,
                  const QString& sourceCharSet,
                  const QString& syntax,
                  const QString& esn);
  ~Z3950Connection();

  void reset();
  void setQuery(const QString& query);
  void setUserPassword(const QString& user, const QString& pword);
  void run();

  void abort() { m_aborted = true; }

private:
  static QCString iconvRun(const QCString& text, const QString& fromCharSet, const QString& toCharSet);
  static QString toXML(const QCString& marc, const QString& fromCharSet);

  bool makeConnection();
  void done();
  void done(const QString& message, int type);
  QCString toCString(const QString& text);
  QString toString(const QCString& text);

  class Private;
  Private* d;

  bool m_connected;
  bool m_aborted;

  KSharedPtr<Z3950Fetcher> m_fetcher;
  QString m_host;
  uint m_port;
  QString m_dbname;
  QString m_user;
  QString m_password;
  QString m_sourceCharSet;
  QString m_syntax;
  QString m_pqn;
  QString m_esn;
  size_t m_start;
  size_t m_limit;
  bool m_hasMore;
};

  } // end namespace
} // end namespace

#endif
