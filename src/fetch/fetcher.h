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

#include "../entry.h"
#include "../collection.h"

#include <kapplication.h> // for KApplication::random()

#include <qobject.h>
#include <qstring.h>

namespace Bookcase {
  namespace Data {
    class Entry;
  }
  namespace Fetch {

enum FetchKey {
  Title,
  Person,
  ISBN,
  Keyword,
  Raw
};

class Fetcher;
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
 * @version $Id: fetcher.h 713 2004-07-23 00:55:33Z robby $
 */
class Fetcher : public QObject {
Q_OBJECT

public:
  /**
   */
  Fetcher(Data::Collection* coll, QObject* parent, const char* name = 0) : QObject(parent, name), m_coll(coll) {}
  /**
   */
  virtual ~Fetcher() {}

  Data::Collection* collection() const { return m_coll; }
  /**
   */
  virtual QString source() const = 0;
  virtual bool isSearching() const = 0;
  virtual void search(FetchKey key, const QString& value) = 0;
  virtual void stop() = 0;
  virtual Data::Entry* fetchEntry(uint uid) = 0;

signals:
  void signalStatus(const QString& status);
  void signalResultFound(const Bookcase::Fetch::SearchResult& result);
  void signalDone();

private:
  Data::Collection* m_coll;
};

  } // end namespace
} // end namespace

#endif
