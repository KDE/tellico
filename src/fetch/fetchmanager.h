/***************************************************************************
    Copyright (C) 2003-2021 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FETCHMANAGER_H
#define TELLICO_FETCHMANAGER_H

#include "fetcher.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QObject>
#include <QMap>
#include <QList>
#include <QPixmap>

class QUrl;
class FetcherTest;
class MultiFetcherTest;

namespace Tellico {
  namespace Fetch {

class FetchResult;
class ConfigWidget;
class ManagerMessage;
class FetcherInitializer;

typedef QHash<QString, Type> NameTypeHash; // map fetcher name to type
typedef QMap<FetchKey, QString> KeyMap; // map key type to name of key
typedef QList<Fetcher::Ptr> FetcherVec;

/**
 * A manager for handling all the different classes of Fetcher.
 *
 * @author Robby Stephenson
 */
class Manager : public QObject {
Q_OBJECT

  /**
   * Keep a hash of all the function pointers to create classes and provide
   * functions to "fake" static virtual methods
   */
  typedef Fetcher::Ptr (*FETCHER_CREATE_FN)(QObject*);
  typedef QString (*FETCHER_NAME_FN)(void);
  typedef QString (*FETCHER_ICON_FN)(void);
  typedef StringHash (*FETCHER_OPTIONALFIELDS_FN)(void);
  typedef ConfigWidget* (*FETCHER_CONFIGWIDGET_FN)(QWidget*);

public:
  struct FetcherFunction  {
    FETCHER_CREATE_FN create;
    FETCHER_NAME_FN name;
    FETCHER_ICON_FN icon;
    FETCHER_OPTIONALFIELDS_FN optionalFields;
    FETCHER_CONFIGWIDGET_FN configWidget;
  };
  static Manager* self();

  ~Manager();

  KeyMap keyMap(const QString& source = QString());
  void startSearch(const QString& source, FetchKey key, const QString& value, Data::Collection::Type collType);
  void continueSearch();
  bool canFetch(Data::Collection::Type collType) const;
  bool hasMoreResults() const;
  void loadFetchers();
  const FetcherVec& fetchers();
  FetcherVec fetchers(int type);
  Fetcher::Ptr fetcherByUuid(const QString& uuid);
  NameTypeHash nameTypeHash();
  ConfigWidget* configWidget(QWidget* parent, Type type, const QString& name);

  // create fetcher for updating an entry
  FetcherVec createUpdateFetchers(int collType);
  FetcherVec createUpdateFetchers(int collType, FetchKey key);
  Fetcher::Ptr createUpdateFetcher(int collType, const QString& source);

  /**
   * Classes derived from Fetcher call this function once
   * per program to register the class ID key.
   */
  void registerFunction(int type, const FetcherFunction& func);

  static QString typeName(Type type);
  static QPixmap fetcherIcon(Type type, int iconGroup=3 /*Small*/, int size=0 /* default */);
  static QPixmap fetcherIcon(Fetcher* ptr, int iconGroup=3 /*Small*/, int size=0 /* default*/);
  static StringHash optionalFields(Type type);

Q_SIGNALS:
  void signalStatus(const QString& status);
  void signalResultFound(Tellico::Fetch::FetchResult* result);
  void signalDone();

public Q_SLOTS:
  void stop();

private Q_SLOTS:
  void slotResultFound(Tellico::Fetch::FetchResult* result);
  void slotFetcherDone(Tellico::Fetch::Fetcher* fetcher);

private:
  friend class ManagerMessage;
  friend class FetcherInitializer;
  friend class ::FetcherTest;
  friend class ::MultiFetcherTest;

  Manager();
  void addFetcher(Fetcher::Ptr fetcher);
  Fetcher::Ptr createFetcher(KSharedConfigPtr config, const QString& configGroup);
  FetcherVec defaultFetchers();
  void updateStatus(const QString& message);

  static QPixmap fetcherIconFromName(const QString& name, int iconGroup, int size);
  static bool bundledScriptHasExecPath(const QString& specFile, KConfigGroup& config);

  typedef QHash<int, FetcherFunction> FunctionRegistry;
  FunctionRegistry functionRegistry;

  FetcherVec m_fetchers;
  int m_currentFetcherIndex;
  KeyMap m_keyMap;
  QHash<QString, Fetcher::Ptr> m_uuidHash;

  StringMap m_scriptMap;
  ManagerMessage* m_messager;
  uint m_count;
  bool m_loadDefaults;
};

  } // end namespace
} // end namespace
#endif
