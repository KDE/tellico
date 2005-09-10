/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include <config.h>

#include "fetchmanager.h"
#include "configwidget.h"
#include "../tellico_kernel.h"

#if AMAZON_SUPPORT
#include "amazonfetcher.h"
#endif
#if IMDB_SUPPORT
#include "imdbfetcher.h"
#endif
#if HAVE_YAZ
#include "z3950fetcher.h"
#endif
//#include "srufetcher.h"
#include "entrezfetcher.h"
#include "execexternalfetcher.h"

#include <kglobal.h>
#include <kconfig.h>
#include <klocale.h>
#include <kdebug.h>

using Tellico::Fetch::Manager;
Manager* Manager::s_self = 0;

Manager::Manager() : QObject(), m_count(0) {
  reloadFetchers();

  m_keyMap.insert(FetchFirst, QString::null);
  m_keyMap.insert(Title,      i18n("Title"));
  m_keyMap.insert(Person,     i18n("Person"));
  m_keyMap.insert(ISBN,       i18n("ISBN"));
  m_keyMap.insert(Keyword,    i18n("Keyword"));
  m_keyMap.insert(Raw,        i18n("Raw Query"));
  m_keyMap.insert(FetchLast,  QString::null);
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

        case Entrez:
          f = new EntrezFetcher(this);
          break;

        case ExecExternal:
          f = new ExecExternalFetcher(this);
          break;

        case Unknown:
        default:
          continue;
      }
      if(f) {
        f->readConfig(config, group);
        m_fetchers.push_back(f);
      }
    }
  } else { // add default sources
#if AMAZON_SUPPORT
    m_fetchers.push_back(new AmazonFetcher(AmazonFetcher::US, this));
#endif
#if IMDB_SUPPORT
    m_fetchers.push_back(new IMDBFetcher(this));
#endif
#if HAVE_YAZ
    m_fetchers.push_back(Z3950Fetcher::libraryOfCongress(this));
#endif
  }

  for(FetcherVec::ConstIterator it = m_fetchers.constBegin(); it != m_fetchers.constEnd(); ++it) {
    connect(it.ptr(), SIGNAL(signalStatus(const QString&)),
            SIGNAL(signalStatus(const QString&)));
    connect(it.ptr(), SIGNAL(signalResultFound(Tellico::Fetch::SearchResult*)),
            SIGNAL(signalResultFound(Tellico::Fetch::SearchResult*)));
    connect(it.ptr(), SIGNAL(signalDone(Tellico::Fetch::Fetcher*)),
            SLOT(slotFetcherDone(Tellico::Fetch::Fetcher*)));
  }
}

QStringList Manager::sources() const {
  QStringList sources;
  for(FetcherVec::ConstIterator it = m_fetchers.constBegin(); it != m_fetchers.constEnd(); ++it) {
    if(it->canFetch(Kernel::self()->collectionType())) {
      sources << it->source();
    }
  }
  return sources;
}

QStringList Manager::keys(const QString& source_) const {
  // assume there's only one fetcher match
  const Fetcher* f = 0;
  for(FetcherVec::ConstIterator it = m_fetchers.constBegin(); it != m_fetchers.constEnd(); ++it) {
    if(source_ == it->source()) {
      f = it.ptr();
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
  for(FetcherVec::Iterator it = m_fetchers.begin(); it != m_fetchers.end(); ++it) {
    if(source_ == it->source()) {
      ++m_count; // Fetcher::search() might emit done(), so increment before calling search()
      it->search(key_, value_, multiple_);
      break;
    }
  }
}

void Manager::stop() {
//  kdDebug() << "Manager::stop()" << endl;
  for(FetcherVec::Iterator it = m_fetchers.begin(); it != m_fetchers.end(); ++it) {
    if(it->isSearching()) {
      it->stop();
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

Tellico::Fetch::FetchKey Manager::fetchKey(const QString& key_) const {
  for(FetchKeyMap::ConstIterator it = m_keyMap.begin(); it != m_keyMap.end(); ++it) {
    if(it.data() == key_) {
      return it.key();
    }
  }
  return FetchFirst;
}

const QString& Manager::fetchKeyString(FetchKey key_) const {
  return m_keyMap[key_];
}

bool Manager::canFetch() const {
  for(FetcherVec::ConstIterator it = m_fetchers.constBegin(); it != m_fetchers.constEnd(); ++it) {
    if(it->canFetch(Kernel::self()->collectionType())) {
      return true;
    }
  }
  return false;
}

// static
// called when creating a new fetcher
Tellico::Fetch::ConfigWidget* Manager::configWidget(Type type_, QWidget* parent_) {
  switch(type_) {
#if AMAZON_SUPPORT
    case Amazon:
      return new AmazonFetcher::ConfigWidget(parent_);
#endif
#if IMDB_SUPPORT
    case IMDB:
      return new IMDBFetcher::ConfigWidget(parent_);
#endif
#if HAVE_YAZ
    case Z3950:
      return new Z3950Fetcher::ConfigWidget(parent_);
#endif
//    case SRU:
//      return new SRUFetcher::ConfigWidget(parent_);
    case Entrez:
      return new EntrezFetcher::ConfigWidget(parent_);
    case ExecExternal:
      return new ExecExternalFetcher::ConfigWidget(parent_);
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
  map.insert(Entrez, i18n("Entrez Database"));
  map.insert(ExecExternal, i18n("External Application"));
  return map;
}

#include "fetchmanager.moc"
