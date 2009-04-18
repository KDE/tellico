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
 ***************************************************************************/

#ifndef TELLICO_FETCHMANAGER_H
#define TELLICO_FETCHMANAGER_H

#include "fetcher.h"

#include <KSortableList>
#include <KConfigGroup>

#include <QObject>
#include <QMap>
#include <QList>
#include <QPixmap>

class KUrl;

namespace Tellico {
  namespace Fetch {

class SearchResult;
class ConfigWidget;
class ManagerMessage;

typedef KSortableItem<Type, QString> TypePair; // fetcher info, type and name of type
typedef KSortableList<Type, QString> TypePairList;
typedef QMap<FetchKey, QString> KeyMap; // map key type to name of key
typedef QList<Fetcher::Ptr> FetcherVec;

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

  KeyMap keyMap(const QString& source = QString()) const;
  void startSearch(const QString& source, FetchKey key, const QString& value);
  void continueSearch();
  void stop();
  bool canFetch() const;
  bool hasMoreResults() const;
  void loadFetchers();
  const FetcherVec& fetchers() const { return m_fetchers; }
  FetcherVec fetchers(int type);
  TypePairList typeList();
  ConfigWidget* configWidget(QWidget* parent, Type type, const QString& name);

  // create fetcher for updating an entry
  FetcherVec createUpdateFetchers(int collType);
  FetcherVec createUpdateFetchers(int collType, FetchKey key);
  Fetcher::Ptr createUpdateFetcher(int collType, const QString& source);

  static QString typeName(Type type);
  static QPixmap fetcherIcon(Fetch::Type type, int iconGroup=3 /*Small*/, int size=0 /* default */);
  static QPixmap fetcherIcon(Fetch::Fetcher::Ptr ptr, int iconGroup=3 /*Small*/, int size=0 /* default*/);

signals:
  void signalStatus(const QString& status);
  void signalResultFound(Tellico::Fetch::SearchResult* result);
  void signalDone();

private slots:
  void slotFetcherDone(Tellico::Fetch::Fetcher* fetcher);

private:
  friend class ManagerMessage;
  static Manager* s_self;

  Manager();
  Fetcher::Ptr createFetcher(KSharedPtr<KSharedConfig> config, const QString& configGroup);
  FetcherVec defaultFetchers();
  void updateStatus(const QString& message);

  static QString favIcon(const char* url_);
  static QString favIcon(const KUrl& url_);
  static bool bundledScriptHasExecPath(const QString& specFile, KConfigGroup& config);

  FetcherVec m_fetchers;
  int m_currentFetcherIndex;
  KeyMap m_keyMap;

  StringMap m_scriptMap;
  ManagerMessage* m_messager;
  uint m_count;
  bool m_loadDefaults : 1;
};

  } // end namespace
} // end namespace
#endif
