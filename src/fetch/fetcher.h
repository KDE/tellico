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

#ifndef FETCHER_H
#define FETCHER_H

namespace Tellico {
  namespace Data {
    class Entry;
  }
  namespace Fetch {
    class ConfigWidget;
  }
}

#include "fetch.h"
#include "../entry.h"
#include "../collection.h"

#include <kapplication.h> // for KApplication::random()

#include <qobject.h>
#include <qstring.h>

namespace Tellico {
  namespace Fetch {

class Fetcher; // forward declaration
struct SearchResult {
  SearchResult(Fetcher* f, QString t, QString d) : uid(KApplication::random()), fetcher(f), title(t), desc(d) {}
  uint uid;
  Fetcher* fetcher;
  QString title;
  QString desc;
};

/**
 * The top-level abstract class for fetching data.
 *
 * @author Robby Stephenson
 * @version $Id: fetcher.h 960 2004-11-18 05:41:01Z robby $
 */
class Fetcher : public QObject {
Q_OBJECT

public:
  /**
   */
  Fetcher(QObject* parent, const char* name = 0) : QObject(parent, name) {}
  /**
   */
  virtual ~Fetcher() {}

  /**
   * Returns true if the fetcher might return entries from a certain collection type.
   */
  virtual bool canFetch(Data::Collection::Type type) = 0;
  /**
   * Returns true if the fetcher can search using a certain key.
   */
  virtual bool canSearch(FetchKey key) const = 0;

  /**
   * Returns the type of the data source.
   */
  virtual Type type() const = 0;
  /**
   * Returns the name of the data source, as defined by the user.
   */
  virtual QString source() const = 0;
  /**
   * Starts a search, using a kew and value.
   */
  virtual void search(FetchKey key, const QString& value, bool multiple) = 0;
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
  virtual Data::Entry* fetchEntry(uint uid) = 0;

  /**
   * Reads the config for the widget, given a config group.
   */
  virtual void readConfig(KConfig* config, const QString& group) = 0;
  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual ConfigWidget* configWidget(QWidget* parent) = 0;

signals:
  void signalStatus(const QString& status);
  void signalResultFound(const Tellico::Fetch::SearchResult& result);
  void signalDone(Tellico::Fetch::Fetcher*);
};

  } // end namespace
} // end namespace

#endif
