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
#include "configwidget.h"
#include "../../config.h"
#if AMAZON_SUPPORT
#include "amazonfetcher.h"
#endif
#include "srufetcher.h"
#if IMDB_SUPPORT
#include "imdbfetcher.h"
#endif
#if HAVE_YAZ
#include "z3950fetcher.h"
#endif
#include "../tellico_kernel.h"

#include <kglobal.h>
#include <kconfig.h>
#include <kdebug.h>

using Tellico::Fetch::Manager;
Manager* Manager::s_self = 0;
Tellico::Fetch::FetchKeyMap Manager::s_keyMap;

Manager::Manager() : QObject(), m_count(0) {
  m_fetchers.setAutoDelete(true);
  reloadFetchers();
}

void Manager::reloadFetchers() {
//  kdDebug() << "Manager::reloadFetchers()" << endl;
  m_fetchers.clear();
  KConfig* config = KGlobal::config();
  if(config->hasGroup(QString::fromLatin1("Data Sources"))) {
    config->setGroup(QString::fromLatin1("Data Sources"));
    int nSources = config->readNumEntry("Sources Count", 0);
    for(int i = 0; i < nSources; ++i) {
      QString group = QString::fromLatin1("Data Source %1").arg(i);
      if(!config->hasGroup(group)) {
        kdDebug() << "Manager::reloadFetchers() - no config group for " << group << endl;
        continue;
      }
      config->setGroup(group);
      int fetchType = config->readNumEntry("Type", Fetch::Unknown);
      if(fetchType == Fetch::Unknown) {
        kdDebug() << "Manager::reloadFetchers() - unknown type " << fetchType << ", skipping" << endl;
        continue;
      }

      Fetcher* f = 0;
      switch(fetchType) {
        case Amazon:
#if AMAZON_SUPPORT
        {
          int site = config->readNumEntry("Site", AmazonFetcher::Unknown);
          if(site == AmazonFetcher::Unknown) {
            kdDebug() << "Manager::reloadFetchers() - unknown amazon site " << site << ", skipping" << endl;
            continue;
          }
          f = new AmazonFetcher(static_cast<AmazonFetcher::Site>(site), this);
        }
#endif
        break;

        case IMDB:
#if IMDB_SUPPORT
          f = new IMDBFetcher(this);
#endif
          break;

        case Z3950:
#if HAVE_YAZ
          f = new Z3950Fetcher(this);
#endif
          break;

        case SRU:
//        m_fetchers.append(new SRUFetcher(this));
          break;

        case Unknown:
        default:
          continue;
      }
      if(f) {
        f->readConfig(config, group);
        m_fetchers.append(f);
      }
    }
  } else { // add default sources
#if AMAZON_SUPPORT
    m_fetchers.append(new AmazonFetcher(AmazonFetcher::US, this));
    m_fetchers.append(new AmazonFetcher(AmazonFetcher::UK, this));
    m_fetchers.append(new AmazonFetcher(AmazonFetcher::DE, this));
    m_fetchers.append(new AmazonFetcher(AmazonFetcher::JP, this));
#endif
#if IMDB_SUPPORT
    m_fetchers.append(new IMDBFetcher(this));
#endif
#if HAVE_YAZ
    m_fetchers.append(Z3950Fetcher::libraryOfCongress(this));
#endif
  }

  for(FetcherListIterator it(m_fetchers); it.current(); ++it) {
    connect(it.current(), SIGNAL(signalStatus(const QString&)),
            SIGNAL(signalStatus(const QString&)));
    connect(it.current(), SIGNAL(signalResultFound(const Tellico::Fetch::SearchResult&)),
            SIGNAL(signalResultFound(const Tellico::Fetch::SearchResult&)));
    connect(it.current(), SIGNAL(signalDone(Tellico::Fetch::Fetcher*)),
            SLOT(slotFetcherDone(Tellico::Fetch::Fetcher*)));
  }
}

QStringList Manager::sources() const {
  QStringList sources;
  for(FetcherListIterator it(m_fetchers); it.current(); ++it) {
    if(it.current()->canFetch(Kernel::self()->collection()->type())) {
      sources << it.current()->source();
    }
  }
  return sources;
}

QStringList Manager::keys(const QString& source_) const {
  // assume there's only one fetcher match
  Fetcher* f = 0;
  for(FetcherListIterator it(m_fetchers); it.current(); ++it) {
    if(source_ == it.current()->source()) {
      f = it.current();
      break;
    }
  }
  // a null string means return all
  if(!f && !source_.isNull()) {
    kdWarning() << "Manager::keys() - no fetcher found!" << endl;
    return QStringList();
  }

  QStringList keys;
  for(int i = FetchFirst+1; i < FetchLast; ++i) {
    if(source_.isNull() || f->canSearch(static_cast<FetchKey>(i))) {
      keys += fetchKeyString(static_cast<FetchKey>(i));
    }
  }
  return keys;
}

void Manager::startSearch(const QString& source_, FetchKey key_, const QString& value_, bool multiple_) {
  if(value_.isEmpty()) {
    emit signalDone();
    return;
  }

  // assume there's only one fetcher match
  for(FetcherListIterator it(m_fetchers); it.current(); ++it) {
    if(source_ == it.current()->source()) {
      ++m_count; // Fetcher::search() might emit done(), so increment before calling search()
      it.current()->search(key_, value_, multiple_);
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

void Manager::slotFetcherDone(Tellico::Fetch::Fetcher*) {
//  kdDebug() << "Manager::slotFetcherDone() - " << (fetcher_ ? fetcher_->source() : QString::null)
//            << " :" << m_count << endl;
  --m_count;
  if(m_count <= 0) {
    emit signalDone();
  }
}

// static
void Manager::initMap() {
  s_keyMap.clear();
  s_keyMap.insert(FetchFirst, QString::null);
  s_keyMap.insert(Title,      i18n("Title"));
  s_keyMap.insert(Person,     i18n("Person"));
  s_keyMap.insert(ISBN,       i18n("ISBN"));
  s_keyMap.insert(Keyword,    i18n("Keyword"));
  s_keyMap.insert(Raw,        i18n("Raw Query"));
  s_keyMap.insert(FetchLast,  QString::null);
}

Tellico::Fetch::FetchKey Manager::fetchKey(const QString& key_) {
  if(s_keyMap.isEmpty()) {
    initMap();
  }
  for(FetchKeyMap::Iterator it = s_keyMap.begin(); it != s_keyMap.end(); ++it) {
    if(it.data() == key_) {
      return it.key();
    }
  }
  return FetchFirst;
}

QString Manager::fetchKeyString(FetchKey key_) {
  if(s_keyMap.isEmpty()) {
    initMap();
  }
  return s_keyMap[key_];
}

bool Manager::canFetch() const {
  for(FetcherListIterator it(m_fetchers); it.current(); ++it) {
    if(it.current()->canFetch(Kernel::self()->collection()->type())) {
      return true;
    }
  }
  return false;
}

// called when creating a new fetcher
Tellico::Fetch::ConfigWidget* Manager::configWidget(Type type_, QWidget* parent_) {
  switch(type_) {
#if AMAZON_SUPPORT
    case Amazon:
      return new AmazonFetcher::ConfigWidget(parent_);
#endif
    case SRU:
      return new SRUFetcher::ConfigWidget(parent_);
#if IMDB_SUPPORT
    case IMDB:
      return new IMDBFetcher::ConfigWidget(parent_);
#endif
#if HAVE_YAZ
    case Z3950:
      return new Z3950Fetcher::ConfigWidget(parent_);
#endif
    default:
      kdDebug() << "Fetch::Manager::configWidget() - no widget defined for type = " << type_ << endl;
      return 0;
  }
  return 0;
}

Tellico::Fetch::FetchMap Manager::sourceMap() {
  Fetch::FetchMap map;
#if AMAZON_SUPPORT
  map.insert(Amazon, i18n("Amazon.com Web Services"));
#endif
#if IMDB_SUPPORT
  map.insert(IMDB, i18n("Internet Movie Database"));
#endif
#if HAVE_YAZ
  map.insert(Z3950, i18n("z39.50"));
#endif
  // SRU not ready yet
//  map.insert(SRU, i18n("SRU"));
  return map;
}

#include "fetchmanager.moc"
