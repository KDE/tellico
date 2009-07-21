/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FETCHER_H
#define TELLICO_FETCHER_H

#include "fetch.h"
#include "fetchrequest.h"
#include "fetchresult.h"
#include "messagehandler.h"
#include "../datavectors.h"

#include <QObject>
#include <QString>

class KConfigGroup;

namespace Tellico {
  namespace Fetch {

class ConfigWidget;

/**
 * The top-level abstract class for fetching data.
 *
 * @author Robby Stephenson
 */
class Fetcher : public QObject, public QSharedData {
Q_OBJECT

public:
  typedef KSharedPtr<Fetcher> Ptr;
  typedef KSharedPtr<const Fetcher> CPtr;

  /**
   */
  Fetcher(QObject* parent);
  /**
   */
  virtual ~Fetcher();

  /**
   * Returns true if the fetcher might return entries from a certain collection type.
   */
  virtual bool canFetch(int type) const = 0;
  /**
   * Returns true if the fetcher can search using a certain key.
   */
  virtual bool canSearch(FetchKey key) const = 0;
  virtual bool canUpdate() const;

  /**
   * Returns the type of the data source.
   */
  virtual Type type() const = 0;
  /**
   * Returns the name of the data source, as defined by the user.
   */
  virtual QString source() const = 0;
  /**
   * Returns the collection type of the most recent search
   */
  int collectionType() const;
  /**
   * Returns whether the fetcher will overwite existing info when updating
   */
  bool updateOverwrite() const;
  const FetchRequest& request() const;
  /**
   * Starts a search, using a key and value. Calls search()
   */
  void startSearch(const FetchRequest& request);
  virtual void continueSearch() {}
  void startUpdate(Data::EntryPtr entry);
  /**
   * Returns true if the fetcher is currently searching.
   */
  virtual bool isSearching() const = 0;
  /**
   * Returns true if the fetcher can continue and fetch more results
   * The fetcher is responsible for remembering state.
   */
  virtual bool hasMoreResults() const { return m_hasMoreResults; }
  /**
   * Stops the fetcher.
   */
  virtual void stop() = 0;
  /**
   * Fetches an entry, given the uid of the search result.
   */
  virtual Data::EntryPtr fetchEntry(uint uid) = 0;

  void setMessageHandler(MessageHandler* handler) { m_messager = handler; }
  MessageHandler* messageHandler() const { return m_messager; }
  /**
   */
  void message(const QString& message, int type) const;
  void infoList(const QString& message, const QStringList& list) const;

  /**
   * Reads the config for the widget, given a config group.
   */
  void readConfig(const KConfigGroup& config, const QString& groupName);
  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual ConfigWidget* configWidget(QWidget* parent) const = 0;

signals:
//  void signalStatus(const QString& status);
  void signalResultFound(Tellico::Fetch::FetchResult* result);
  void signalDone(Tellico::Fetch::Fetcher* fetcher);

protected:
  QString m_name;
  FetchRequest m_request;
  bool m_updateOverwrite : 1;
  bool m_hasMoreResults : 1;

private:
  /**
   * Starts a search, using a key and value.
   */
  virtual void search() = 0;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) = 0;
  virtual void readConfigHook(const KConfigGroup&) = 0;
  virtual void saveConfigHook(KConfigGroup&) {}

  MessageHandler* m_messager;
  QString m_configGroup;
};

  } // end namespace
} // end namespace

#endif
