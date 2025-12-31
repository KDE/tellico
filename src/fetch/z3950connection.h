/***************************************************************************
    Copyright (C) 2005-2020 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FETCH_Z3950CONNECTION_H
#define TELLICO_FETCH_Z3950CONNECTION_H

#include <QThread>
#include <QEvent>
#include <QPointer>

namespace Tellico {
  namespace Fetch {
    class Fetcher;

class Z3950ResultFound : public QEvent {
public:
  Z3950ResultFound(const QString& s);
  ~Z3950ResultFound();
  const QString& result() const { return m_result; }

  static QEvent::Type uid() { return static_cast<QEvent::Type>(QEvent::User + 11111); }

private:
  Q_DISABLE_COPY(Z3950ResultFound)
  QString m_result;
};

class Z3950ConnectionDone : public QEvent {
public:
  Z3950ConnectionDone(bool more) : QEvent(uid()), m_type(-1), m_hasMore(more) {}
  Z3950ConnectionDone(bool more, const QString& s, int t) : QEvent(uid()), m_msg(s), m_type(t), m_hasMore(more) {}

  const QString& message() const { return m_msg; }
  int messageType() const { return m_type; }
  bool hasMoreResults() const { return m_hasMore; }

  static QEvent::Type uid() { return static_cast<QEvent::Type>(QEvent::User + 22222); }

private:
  Q_DISABLE_COPY(Z3950ConnectionDone)
  QString m_msg;
  int m_type;
  bool m_hasMore;
};

class Z3950SyntaxChange : public QEvent {
public:
  Z3950SyntaxChange(const QString& s) : QEvent(uid()), m_syntax(s) {}
  const QString& syntax() const { return m_syntax; }

  static QEvent::Type uid() { return static_cast<QEvent::Type>(QEvent::User + 33333); }

private:
  Q_DISABLE_COPY(Z3950SyntaxChange)
  QString m_syntax;
};

/**
 * @author Robby Stephenson
 */
class Z3950Connection : public QThread {
Q_OBJECT

public:
  Z3950Connection(Fetcher* fetcher,
                  const QString& host,
                  uint port,
                  const QString& dbname,
                  const QString& syntax,
                  const QString& esn);
  ~Z3950Connection();

  void reset();
  void setQuery(const QString& query);
  void setUserPassword(const QString& user, const QString& pword);
  void setCharacterSet(const QString& queryCharSet, const QString& responseCharSet);
  void run() override;

  void abort() { m_aborted = true; }

private:
  static QByteArray iconvRun(const QByteArray& text, const QString& fromCharSet, const QString& toCharSet);
  static QString toXML(const QByteArray& marc, const QString& fromCharSet);

  bool makeConnection();
  void done();
  void done(const QString& message, int type);
  QByteArray queryToByteArray(const QString& text);
  QString responseToString(const QByteArray& text);
  void checkPendingEvents();

  class Private;
  Private* d;

  bool m_connected;
  bool m_aborted;

  QPointer<Fetcher> m_fetcher;
  QString m_host;
  uint m_port;
  QString m_dbname;
  QString m_user;
  QString m_password;
  QString m_queryCharSet;
  QString m_responseCharSet;
  QString m_syntax;
  QString m_pqn;
  QString m_esn;
  size_t m_start;
  size_t m_limit;
  bool m_hasMore;

  friend class Z3950ResultFound;
  static int resultsLeft;
};

  } // end namespace
} // end namespace

#endif
