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

#include "fetchmanager.h"
#include "srufetcher.h"
#include "amazonfetcher.h"

#include <kdebug.h>

using Bookcase::Fetch::Manager;

Manager::Manager(Data::Collection* coll_, QObject* parent_, const char* name_) : QObject(parent_, name_), m_count(0) {
  m_fetchers.setAutoDelete(true);

  m_fetchers.append(new AmazonFetcher(AmazonFetcher::US, coll_, this));
  m_fetchers.append(new AmazonFetcher(AmazonFetcher::UK, coll_, this));
  m_fetchers.append(new AmazonFetcher(AmazonFetcher::DE, coll_, this));
  m_fetchers.append(new AmazonFetcher(AmazonFetcher::JP, coll_, this));
//  m_fetchers.append(new SRUFetcher(coll_, this));

  for(FetcherListIterator it(m_fetchers); it.current(); ++it) {
    connect(it.current(), SIGNAL(signalStatus(const QString&)),
            SIGNAL(signalStatus(const QString&)));
    connect(it.current(), SIGNAL(signalResultFound(const Bookcase::Fetch::SearchResult&)),
            SIGNAL(signalResultFound(const Bookcase::Fetch::SearchResult&)));
    connect(it.current(), SIGNAL(signalDone()),
            SLOT(slotFetcherDone()));
  }
}

QStringList Manager::sources() const {
  QStringList sources;
  for(FetcherListIterator it(m_fetchers); it.current(); ++it) {
    sources << it.current()->source();
  }
  return sources;
}

void Manager::startSearch(const QString& source_, FetchKey key_, const QString& value_) {
  if(value_.isEmpty()) {
    emit signalDone();
    return;
  }

  // assume there's only one fetcher match
  for(FetcherListIterator it(m_fetchers); it.current(); ++it) {
    if(source_ == it.current()->source()) {
      it.current()->search(key_, value_);
      ++m_count;
      break;
    }
  }
}

void Manager::stop() {
//  kdDebug() << "Manager::stop()" << endl;
  for(FetcherListIterator it(m_fetchers); it.current(); ++it) {
    if(it.current()->isSearching()) {
      it.current()->stop();
    }
  }
#ifndef NDEBUG
  if(m_count != 0) {
    kdDebug() << "Manager::stop() - count should be 0!" << endl;
  }
#endif
  m_count = 0;
}

void Manager::slotFetcherDone() {
//  kdDebug() << "Manager::slotFetcherDone() - " << m_count << endl;
  --m_count;
  if(m_count == 0) {
    emit signalDone();
  }
}

#include "fetchmanager.moc"
