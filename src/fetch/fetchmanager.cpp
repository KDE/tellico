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

#include <config.h>

#include "fetchmanager.h"
#include "configwidget.h"
#include "messagehandler.h"
#include "../tellico_kernel.h"
#include "../entry.h"
#include "../collection.h"

#if AMAZON_SUPPORT
#include "amazonfetcher.h"
#endif
#if IMDB_SUPPORT
#include "imdbfetcher.h"
#endif
#if HAVE_YAZ
#include "z3950fetcher.h"
#endif
#include "srufetcher.h"
#include "entrezfetcher.h"
#include "execexternalfetcher.h"
#include "yahoofetcher.h"
#include "animenfofetcher.h"
#include "ibsfetcher.h"

#include <kglobal.h>
#include <kconfig.h>
#include <klocale.h>

using Tellico::Fetch::Manager;
Manager* Manager::s_self = 0;

Manager::Manager() : QObject(), m_messager(new ManagerMessage()), m_count(0), m_loadDefaults(false) {
  loadFetchers();

  m_keyMap.insert(FetchFirst, QString::null);
  m_keyMap.insert(Title,      i18n("Title"));
  m_keyMap.insert(Person,     i18n("Person"));
  m_keyMap.insert(ISBN,       i18n("ISBN"));
  m_keyMap.insert(UPC,        i18n("UPC"));
  m_keyMap.insert(Keyword,    i18n("Keyword"));
  m_keyMap.insert(Raw,        i18n("Raw Query"));
  m_keyMap.insert(FetchLast,  QString::null);
}

Manager::~Manager() {
  delete m_messager;
}

void Manager::loadFetchers() {
//  myDebug() << "Manager::loadFetchers()" << endl;
  m_fetchers.clear();
  m_configMap.clear();

  KConfig* config = KGlobal::config();
  if(config->hasGroup(QString::fromLatin1("Data Sources"))) {
    KConfigGroupSaver saver(config, QString::fromLatin1("Data Sources"));
    int nSources = config->readNumEntry("Sources Count", 0);
    for(int i = 0; i < nSources; ++i) {
      QString group = QString::fromLatin1("Data Source %1").arg(i);
      Fetcher::Ptr f = createFetcher(config, group);
      if(f) {
        m_configMap.insert(f, group);
        m_fetchers.append(f);
        f->setMessageHandler(m_messager);
      }
    }
    m_loadDefaults = false;
  } else { // add default sources
    m_fetchers = defaultFetchers();
    m_loadDefaults = true;
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
  KSharedPtr<const Fetcher> f = 0;
  for(FetcherVec::ConstIterator it = m_fetchers.constBegin(); it != m_fetchers.constEnd(); ++it) {
    if(source_ == it->source()) {
      f = it.data();
      break;
    }
  }
  // an empty string means return all
  if(!f && !source_.isEmpty()) {
    kdWarning() << "Manager::keys() - no fetcher found!" << endl;
    return QStringList();
  }

  int type = Kernel::self()->collectionType();
  QStringList keys;
  for(int i = FetchFirst+1; i < FetchLast; ++i) {
    if(source_.isNull() || f->canSearch(static_cast<FetchKey>(i))) {
      // ISBN is only allowed for books
      if(i != Fetch::ISBN || type == Data::Collection::Book || type == Data::Collection::Bibtex) {
        keys += fetchKeyString(static_cast<FetchKey>(i));
      }
    }
  }
  return keys;
}

void Manager::startSearch(const QString& source_, FetchKey key_, const QString& value_) {
  if(value_.isEmpty()) {
    emit signalDone();
    return;
  }

  // assume there's only one fetcher match
  for(FetcherVec::Iterator it = m_fetchers.begin(); it != m_fetchers.end(); ++it) {
    if(source_ == it->source()) {
      ++m_count; // Fetcher::search() might emit done(), so increment before calling search()
      connect(it.data(), SIGNAL(signalResultFound(Tellico::Fetch::SearchResult*)),
              SIGNAL(signalResultFound(Tellico::Fetch::SearchResult*)));
      connect(it.data(), SIGNAL(signalDone(Tellico::Fetch::Fetcher::Ptr)),
              SLOT(slotFetcherDone(Tellico::Fetch::Fetcher::Ptr)));
      it->search(key_, value_);
      break;
    }
  }
}

void Manager::stop() {
//  myDebug() << "Manager::stop()" << endl;
  for(FetcherVec::Iterator it = m_fetchers.begin(); it != m_fetchers.end(); ++it) {
    if(it->isSearching()) {
      it->stop();
    }
  }
#ifndef NDEBUG
  if(m_count != 0) {
    myDebug() << "Manager::stop() - count should be 0!" << endl;
  }
#endif
  m_count = 0;
}

void Manager::slotFetcherDone(Fetcher::Ptr fetcher_) {
//  myDebug() << "Manager::slotFetcherDone() - " << (fetcher_ ? fetcher_->source() : QString::null)
//            << " :" << m_count << endl;
  fetcher_->disconnect(); // disconnect all signals
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

Tellico::Fetch::Fetcher::Ptr Manager::createFetcher(KConfig* config_, const QString& group_) {
  if(!config_->hasGroup(group_)) {
    myDebug() << "Manager::createFetcher() - no config group for " << group_ << endl;
    return 0;
  }

  KConfigGroupSaver groupSaver(config_, group_);

  int fetchType = config_->readNumEntry("Type", Fetch::Unknown);
  if(fetchType == Fetch::Unknown) {
    myDebug() << "Manager::createFetcher() - unknown type " << fetchType << ", skipping" << endl;
    return 0;
  }

  Fetcher::Ptr f = 0;
  switch(fetchType) {
    case Amazon:
#if AMAZON_SUPPORT
      {
        int site = config_->readNumEntry("Site", AmazonFetcher::Unknown);
        if(site == AmazonFetcher::Unknown) {
          myDebug() << "Manager::createFetcher() - unknown amazon site " << site << ", skipping" << endl;
        } else {
          f = new AmazonFetcher(static_cast<AmazonFetcher::Site>(site), this);
        }
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
      f = new SRUFetcher(this);
      break;

    case Entrez:
      f = new EntrezFetcher(this);
      break;

    case ExecExternal:
      f = new ExecExternalFetcher(this);
      break;

    case Yahoo:
      f = new YahooFetcher(this);
      break;

    case AnimeNfo:
      f = new AnimeNfoFetcher(this);
      break;

    case IBS:
      f = new IBSFetcher(this);
      break;

    case Unknown:
    default:
      break;
  }
  if(f) {
    f->readConfig(config_, group_);
  }
  return f;
}

// static
Tellico::Fetch::FetcherVec Manager::defaultFetchers() {
  FetcherVec vec;
#if AMAZON_SUPPORT
  vec.append(new AmazonFetcher(AmazonFetcher::US, this));
#endif
#if IMDB_SUPPORT
  vec.append(new IMDBFetcher(this));
#endif
  vec.append(SRUFetcher::libraryOfCongress(this));
  vec.append(new YahooFetcher(this));
  vec.append(new AnimeNfoFetcher(this));
// only add IBS if user includes italian
  if(KGlobal::locale()->languagesTwoAlpha().contains(QString::fromLatin1("it"))) {
    vec.append(new IBSFetcher(this));
  }
  return vec;
}

Tellico::Fetch::FetcherVec Manager::createUpdateFetchers(int collType_) {
  if(m_loadDefaults) {
    return defaultFetchers();
  }

  FetcherVec vec;
  KConfig* config = KGlobal::config();
  if(config->hasGroup(QString::fromLatin1("Data Sources"))) {
    KConfigGroupSaver saver(config, QString::fromLatin1("Data Sources"));
    int nSources = config->readNumEntry("Sources Count", 0);
    for(int i = 0; i < nSources; ++i) {
      QString group = QString::fromLatin1("Data Source %1").arg(i);
      Fetcher::Ptr f = createFetcher(config, group);
      if(f && f->canFetch(collType_) && f->canUpdate()) {
        vec.append(f);
      }
    }
  }
  return vec;
}

Tellico::Fetch::Fetcher::Ptr Manager::createUpdateFetcher(int collType_, const QString& source_) {
  Fetcher::Ptr fetcher = 0;
  // creates new fetchers
  FetcherVec fetchers = createUpdateFetchers(collType_);
  for(Fetch::FetcherVec::Iterator it = fetchers.begin(); it != fetchers.end(); ++it) {
    if(it->source() == source_) {
      fetcher = it;
      break;
    }
  }
  return fetcher;
}

void Manager::updateStatus(const QString& message_) {
  emit signalStatus(message_);
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
    case SRU:
      return new SRUFetcher::ConfigWidget(parent_);
    case Entrez:
      return new EntrezFetcher::ConfigWidget(parent_);
    case ExecExternal:
      return new ExecExternalFetcher::ConfigWidget(parent_);
    case Yahoo:
      return new YahooFetcher::ConfigWidget(parent_);
    case AnimeNfo:
      return new AnimeNfoFetcher::ConfigWidget(parent_);
    case IBS:
      return new IBSFetcher::ConfigWidget(parent_);
    default:
      kdWarning() << "Fetch::Manager::configWidget() - no widget defined for type = " << type_ << endl;
      return 0;
  }
  return 0;
}

Tellico::Fetch::FetchMap Manager::sourceMap() {
  Fetch::FetchMap map;
#if AMAZON_SUPPORT
  map.insert(Amazon, AmazonFetcher::defaultName());
#endif
#if IMDB_SUPPORT
  map.insert(IMDB, IMDBFetcher::defaultName());
#endif
#if HAVE_YAZ
  map.insert(Z3950, Z3950Fetcher::defaultName());
#endif
  map.insert(SRU, SRUFetcher::defaultName());
  map.insert(Entrez, EntrezFetcher::defaultName());
  map.insert(ExecExternal, ExecExternalFetcher::defaultName());
  map.insert(Yahoo, YahooFetcher::defaultName());
  map.insert(AnimeNfo, AnimeNfoFetcher::defaultName());
  map.insert(IBS, IBSFetcher::defaultName());
  return map;
}

QString Manager::defaultSourceName(const QString& typeName_) {
  Type type = Unknown;
  FetchMap fetchMap = sourceMap();
  for(FetchMap::ConstIterator it = fetchMap.begin(); it != fetchMap.end(); ++it) {
    if(it.data() == typeName_) {
      type = it.key();
      break;
    }
  }
  if(type == Unknown) {
    return QString::null;
  }

  switch(type) {
#if AMAZON_SUPPORT
    case Amazon:
      return AmazonFetcher::defaultName();
#endif
#if IMDB_SUPPORT
    case IMDB:
      return IMDBFetcher::defaultName();
#endif
#if HAVE_YAZ
    case Z3950:
      return Z3950Fetcher::defaultName();
#endif
    case SRU:
      return SRUFetcher::defaultName();
    case Entrez:
      return EntrezFetcher::defaultName();
    case ExecExternal:
      return ExecExternalFetcher::defaultName();
    case Yahoo:
      return YahooFetcher::defaultName();
    case AnimeNfo:
      return AnimeNfoFetcher::defaultName();
    case IBS:
      return IBSFetcher::defaultName();
    default:
      kdWarning() << "Fetch::Manager::defaultName() - no name defined for type = " << type << endl;
  }
  return QString::null;
}

#include "fetchmanager.moc"
