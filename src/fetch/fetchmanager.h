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

#include "fetcher.h"

#include <qobject.h>
#include <qptrlist.h>

namespace Bookcase {
  namespace Data {
    class Collection;
  }
  namespace Fetch {

/**
 * A manager for handling all the different classes of Fetcher.
 *
 * @author Robby Stephenson
 * @version $Id: fetchmanager.h 727 2004-08-04 01:38:57Z robby $
 */
class Manager : public QObject {
Q_OBJECT

public:
  Manager(Data::Collection* coll, QObject* parent, const char* name = 0);

  QStringList sources() const;
  void startSearch(const QString& source, FetchKey key, const QString& value);
  void stop();

signals:
  void signalStatus(const QString& status);
  void signalResultFound(const Bookcase::Fetch::SearchResult& result);
  void signalDone();

private slots:
  void slotFetcherDone();

private:
  typedef QPtrList<Fetcher> FetcherList;
  typedef QPtrListIterator<Fetcher> FetcherListIterator;
  FetcherList m_fetchers;
  unsigned m_count;
};

  } // end namespace
} // end namespace
#endif
