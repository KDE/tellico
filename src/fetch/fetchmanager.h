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

#ifndef FETCHMANAGER_H
#define FETCHMANAGER_H

namespace Tellico {
  namespace Fetch {
    class SearchResult;
    class ConfigWidget;
    class ManagerMessage;
  }
}

#include "fetcher.h"
#include "../ptrvector.h"

#include <qobject.h>

namespace Tellico {
  namespace Fetch {

typedef QMap<Type, QString> FetchMap;
typedef QMap<FetchKey, QString> FetchKeyMap;
typedef QMap<Fetcher::Ptr, QString> FetchConfigMap;
typedef Vector<Fetcher> FetcherVec;

/**
 * A manager for handling all the different classes of Fetcher.
 *
 * @author Robby Stephenson
 */
class Manager : public QObject {
Q_OBJECT

public:
  static Manager* self() {  if(!s_self) s_self = new Manager(); return s_self; }

  ~Manager();

  QStringList sources() const;
  QStringList keys(const QString& source) const;
  void startSearch(const QString& source, FetchKey key, const QString& value);
  void stop();
  bool canFetch() const;
  void loadFetchers();
  const FetcherVec& fetchers() const { return m_fetchers; }
  FetchKey fetchKey(const QString& key) const;
  const QString& fetchKeyString(FetchKey key) const;

  // create fetcher for updating an entry
  FetcherVec createUpdateFetchers(int collType);
  Fetcher::Ptr createUpdateFetcher(int collType, const QString& source);

  static ConfigWidget* configWidget(Type type, QWidget* parent);
  static FetchMap sourceMap();
  static QString defaultSourceName(const QString& sourceType);

signals:
  void signalStatus(const QString& status);
  void signalResultFound(Tellico::Fetch::SearchResult* result);
  void signalDone();

private slots:
  void slotFetcherDone(Tellico::Fetch::Fetcher::Ptr);

private:
  friend class ManagerMessage;
  static Manager* s_self;

  Manager();
  Fetcher::Ptr createFetcher(KConfig* config, const QString& configGroup);
  FetcherVec defaultFetchers();
  void updateStatus(const QString& message);

  FetcherVec m_fetchers;
  FetchKeyMap m_keyMap;
  FetchConfigMap m_configMap;
  ManagerMessage* m_messager;
  uint m_count;
  bool m_loadDefaults : 1;
};

  } // end namespace
} // end namespace
#endif
