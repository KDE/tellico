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

#ifndef FETCHER_H
#define FETCHER_H

#include "fetch.h"
#include "../datavectors.h"

#include <kapplication.h> // for KApplication::random()

#include <qobject.h>
#include <qstring.h>

namespace Tellico {
  namespace Fetch {
    class ConfigWidget;
    class MessageHandler;
    class SearchResult;

/**
 * The top-level abstract class for fetching data.
 *
 * @author Robby Stephenson
 */
class Fetcher : public QObject, public KShared {
Q_OBJECT

public:
  typedef KSharedPtr<Fetcher> Ptr;
  typedef KSharedPtr<const Fetcher> CPtr;

  /**
   */
  Fetcher(QObject* parent, const char* name = 0) : QObject(parent, name), KShared(), m_messager(0) {}
  /**
   */
  virtual ~Fetcher() {}

  /**
   * Returns true if the fetcher might return entries from a certain collection type.
   */
  virtual bool canFetch(int type) const = 0;
  /**
   * Returns true if the fetcher can search using a certain key.
   */
  virtual bool canSearch(FetchKey key) const = 0;
  virtual bool canUpdate() const { return true; }

  /**
   * Returns the type of the data source.
   */
  virtual Type type() const = 0;
  /**
   * Returns the name of the data source, as defined by the user.
   */
  virtual QString source() const = 0;
  /**
   * Returns whether the fetcher will overwite existing info when updating
   */
  bool updateOverwrite() const { return m_updateOverwrite; }
  /**
   * Starts a search, using a kew and value.
   */
  virtual void search(FetchKey key, const QString& value) = 0;
  virtual void updateEntry(Data::EntryPtr);
  /**
   * Returns true if the fetcher is currently searching.
   */
  virtual bool isSearching() const = 0;
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
  void readConfig(KConfig* config, const QString& group);
  virtual void readConfigHook(KConfig* config, const QString& group) = 0;
  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual ConfigWidget* configWidget(QWidget* parent) const = 0;

signals:
//  void signalStatus(const QString& status);
  void signalResultFound(Tellico::Fetch::SearchResult* result);
  void signalDone(Tellico::Fetch::Fetcher::Ptr);

protected:
  QString m_name;
  bool m_updateOverwrite : 1;

private:
  MessageHandler* m_messager;
};

class SearchResult {
public:
  SearchResult(Fetcher::Ptr f, QString t, QString d) : uid(KApplication::random()), fetcher(f), title(t), desc(d) {}
  Data::EntryPtr fetchEntry();
  uint uid;
  Fetcher::Ptr fetcher;
  QString title;
  QString desc;
};

  } // end namespace
} // end namespace

#endif
