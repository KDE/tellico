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

#include <ksortablevaluelist.h>

#include <qobject.h>

namespace Tellico {
  namespace Fetch {

typedef KSortableItem<Type, QString> TypePair; // fetcher info, type and name of type
typedef KSortableValueList<Type, QString> TypePairList;
typedef QMap<FetchKey, QString> KeyMap; // map key type to name of key
typedef Vector<Fetcher> FetcherVec;
typedef Vector<const Fetcher> CFetcherVec;

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

  KeyMap keyMap(const QString& source = QString::null) const;
  void startSearch(const QString& source, FetchKey key, const QString& value);
  void continueSearch();
  void stop();
  bool canFetch() const;
  bool hasMoreResults() const;
  void loadFetchers();
  const FetcherVec& fetchers() const { return m_fetchers; }
  CFetcherVec fetchers(int type) const;
  TypePairList typeList();
  ConfigWidget* configWidget(QWidget* parent, Type type, const QString& name);

  // create fetcher for updating an entry
  FetcherVec createUpdateFetchers(int collType);
  Fetcher::Ptr createUpdateFetcher(int collType, const QString& source);

  static QString typeName(Type type);
  static QPixmap fetcherIcon(Fetch::Type type);
  static QPixmap fetcherIcon(Fetch::Fetcher::CPtr ptr);

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

  static QString favIcon(const KURL& url);
  static bool bundledScriptHasExecPath(const QString& specFile, KConfig* config);

  FetcherVec m_fetchers;
  int m_currentFetcherIndex;
  KeyMap m_keyMap;
  typedef QMap<Fetcher::Ptr, QString> ConfigMap;
  ConfigMap m_configMap;
  typedef QMap<QString, QString> StringMap;
  StringMap m_scriptMap;
  ManagerMessage* m_messager;
  uint m_count;
  bool m_loadDefaults : 1;
};

  } // end namespace
} // end namespace
#endif
