/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef FETCHMANAGER_H
#define FETCHMANAGER_H

namespace Tellico {
  namespace Data {
    class Collection;
  }
  namespace Fetch {
    class Fetcher;
    class SearchResult;
    class ConfigWidget;
  }
}

#include "fetch.h"

#include <qobject.h>
#include <qptrlist.h>

namespace Tellico {
  namespace Fetch {

typedef QMap<Type, QString> FetchMap;
typedef QMap<FetchKey, QString> FetchKeyMap;
typedef QPtrList<Fetcher> FetcherList;
typedef QPtrListIterator<Fetcher> FetcherListIterator;

/**
 * A manager for handling all the different classes of Fetcher.
 *
 * @author Robby Stephenson
 * @version $Id: fetchmanager.h 1065 2005-02-02 02:51:05Z robby $
 */
class Manager : public QObject {
Q_OBJECT

public:
  static Manager* self() {  if(!s_self) s_self = new Manager(); return s_self; }

  QStringList sources() const;
  QStringList keys(const QString& source) const;
  void startSearch(const QString& source, FetchKey key, const QString& value, bool multiple);
  void stop();
  bool canFetch() const;
  void reloadFetchers();
  const FetcherList& fetcherList() const { return m_fetchers; }

  static ConfigWidget* configWidget(Type type, QWidget* parent);
  static FetchMap sourceMap();
  static FetchKey fetchKey(const QString& key);
  static QString fetchKeyString(FetchKey key);

signals:
  void signalStatus(const QString& status);
  void signalResultFound(const Tellico::Fetch::SearchResult& result);
  void signalDone();

private slots:
  void slotFetcherDone(Tellico::Fetch::Fetcher*);

private:
  static Manager* s_self;
  Manager();

  static FetchKeyMap s_keyMap;
  static void initMap();

  FetcherList m_fetchers;
  unsigned m_count;
};

  } // end namespace
} // end namespace
#endif
