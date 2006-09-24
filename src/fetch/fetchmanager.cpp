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
#include "../tellico_utils.h"

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
#include "isbndbfetcher.h"

#include <kglobal.h>
#include <kconfig.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <dcopref.h>
#include <ktempfile.h>
#include <kio/netaccess.h>

#include <qfileinfo.h>
#include <qdir.h>

using Tellico::Fetch::Manager;
Manager* Manager::s_self = 0;

Manager::Manager() : QObject(), m_currentFetcherIndex(-1), m_messager(new ManagerMessage()),
                     m_count(0), m_loadDefaults(false) {
  loadFetchers();

//  m_keyMap.insert(FetchFirst, QString::null);
  m_keyMap.insert(Title,      i18n("Title"));
  m_keyMap.insert(Person,     i18n("Person"));
  m_keyMap.insert(ISBN,       i18n("ISBN"));
  m_keyMap.insert(UPC,        i18n("UPC"));
  m_keyMap.insert(Keyword,    i18n("Keyword"));
  m_keyMap.insert(Raw,        i18n("Raw Query"));
//  m_keyMap.insert(FetchLast,  QString::null);
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

Tellico::Fetch::CFetcherVec Manager::fetchers(int type_) const {
  CFetcherVec vec;
  for(FetcherVec::ConstIterator it = m_fetchers.constBegin(); it != m_fetchers.constEnd(); ++it) {
    if(it->canFetch(type_)) {
      vec.append(it.data());
    }
  }
  return vec;
}

Tellico::Fetch::KeyMap Manager::keyMap(const QString& source_) const {
  // an empty string means return all
  if(source_.isEmpty()) {
    return m_keyMap;
  }

  // assume there's only one fetcher match
  KSharedPtr<const Fetcher> f = 0;
  for(FetcherVec::ConstIterator it = m_fetchers.constBegin(); it != m_fetchers.constEnd(); ++it) {
    if(source_ == it->source()) {
      f = it.data();
      break;
    }
  }
  if(!f) {
    kdWarning() << "Manager::keyMap() - no fetcher found!" << endl;
    return KeyMap();
  }

  KeyMap map;
  for(KeyMap::ConstIterator it = m_keyMap.begin(); it != m_keyMap.end(); ++it) {
    if(f->canSearch(it.key())) {
      map.insert(it.key(), it.data());
    }
  }
  return map;
}

void Manager::startSearch(const QString& source_, FetchKey key_, const QString& value_) {
  if(value_.isEmpty()) {
    emit signalDone();
    return;
  }

  // assume there's only one fetcher match
  int i = 0;
  m_currentFetcherIndex = -1;
  for(FetcherVec::Iterator it = m_fetchers.begin(); it != m_fetchers.end(); ++it, ++i) {
    if(source_ == it->source()) {
      ++m_count; // Fetcher::search() might emit done(), so increment before calling search()
      connect(it.data(), SIGNAL(signalResultFound(Tellico::Fetch::SearchResult*)),
              SIGNAL(signalResultFound(Tellico::Fetch::SearchResult*)));
      connect(it.data(), SIGNAL(signalDone(Tellico::Fetch::Fetcher::Ptr)),
              SLOT(slotFetcherDone(Tellico::Fetch::Fetcher::Ptr)));
      it->search(key_, value_);
      m_currentFetcherIndex = i;
      break;
    }
  }
}

void Manager::continueSearch() {
  if(m_currentFetcherIndex < 0 || m_currentFetcherIndex >= static_cast<int>(m_fetchers.count())) {
    myDebug() << "Manager::continueSearch() - can't continue!" << endl;
    emit signalDone();
    return;
  }
  Fetcher::Ptr f = m_fetchers[m_currentFetcherIndex];
  if(f && f->hasMoreResults()) {
    ++m_count;
    connect(f, SIGNAL(signalResultFound(Tellico::Fetch::SearchResult*)),
            SIGNAL(signalResultFound(Tellico::Fetch::SearchResult*)));
    connect(f, SIGNAL(signalDone(Tellico::Fetch::Fetcher::Ptr)),
            SLOT(slotFetcherDone(Tellico::Fetch::Fetcher::Ptr)));
    f->continueSearch();
  } else {
    emit signalDone();
  }
}

bool Manager::hasMoreResults() const {
  if(m_currentFetcherIndex < 0 || m_currentFetcherIndex >= static_cast<int>(m_fetchers.count())) {
    return false;
  }
  Fetcher::Ptr f = m_fetchers[m_currentFetcherIndex];
  return f && f->hasMoreResults();
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

    case ISBNdb:
      f = new ISBNdbFetcher(this);
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
  vec.append(new ISBNdbFetcher(this));
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

Tellico::Fetch::TypePairList Manager::typeList() {
  Fetch::TypePairList list;
#if AMAZON_SUPPORT
  list.append(TypePair(AmazonFetcher::defaultName(), Amazon));
#endif
#if IMDB_SUPPORT
  list.append(TypePair(IMDBFetcher::defaultName(),         IMDB));
#endif
#if HAVE_YAZ
  list.append(TypePair(Z3950Fetcher::defaultName(),        Z3950));
#endif
  list.append(TypePair(SRUFetcher::defaultName(),          SRU));
  list.append(TypePair(EntrezFetcher::defaultName(),       Entrez));
  list.append(TypePair(ExecExternalFetcher::defaultName(), ExecExternal));
  list.append(TypePair(YahooFetcher::defaultName(),        Yahoo));
  list.append(TypePair(AnimeNfoFetcher::defaultName(),     AnimeNfo));
  list.append(TypePair(IBSFetcher::defaultName(),          IBS));
  list.append(TypePair(ISBNdbFetcher::defaultName(),       ISBNdb));

  // now find all the scripts distributed with tellico
  QStringList files = KGlobal::dirs()->findAllResources("appdata", QString::fromLatin1("data-sources/*.spec"),
                                                        false, true);
  for(QStringList::Iterator it = files.begin(); it != files.end(); ++it) {
    KConfig spec(*it, false, false);
    QString name = spec.readEntry("Name");
    if(name.isEmpty()) {
      myDebug() << "Fetch::Manager::typeList() - no Name for " << *it << endl;
      continue;
    }

    if(!bundledScriptHasExecPath(*it, &spec)) { // no available exec
      continue;
    }

    list.append(TypePair(name, ExecExternal));
    m_scriptMap.insert(name, *it);
  }
  list.sort();
  return list;
}


// called when creating a new fetcher
Tellico::Fetch::ConfigWidget* Manager::configWidget(QWidget* parent_, Type type_, const QString& name_) {
  ConfigWidget* w = 0;
  switch(type_) {
#if AMAZON_SUPPORT
    case Amazon:
      w = new AmazonFetcher::ConfigWidget(parent_);
      break;
#endif
#if IMDB_SUPPORT
    case IMDB:
      w = new IMDBFetcher::ConfigWidget(parent_);
      break;
#endif
#if HAVE_YAZ
    case Z3950:
      w = new Z3950Fetcher::ConfigWidget(parent_);
      break;
#endif
    case SRU:
      w = new SRUFetcher::ConfigWidget(parent_);
      break;
    case Entrez:
      w = new EntrezFetcher::ConfigWidget(parent_);
      break;
    case ExecExternal:
      w = new ExecExternalFetcher::ConfigWidget(parent_);
      if(!name_.isEmpty() && m_scriptMap.contains(name_)) {
        // bundledScriptHasExecPath() actually needs to write the exec path
        // back to the config so the configWidget can read it. But if the spec file
        // is not readablle, that doesn't work. So work around it with a copy to a temp file
        KTempFile tmpFile;
        tmpFile.setAutoDelete(true);
        KURL from, to;
        from.setPath(m_scriptMap[name_]);
        to.setPath(tmpFile.name());
        // have to overwrite since KTempFile already created it
        if(!KIO::NetAccess::file_copy(from, to, -1, true /*overwrite*/)) {
          myDebug() << KIO::NetAccess::lastErrorString() << endl;
        }
        KConfig spec(to.path(), false, false);
        // pass actual location of spec file
        if(name_ == spec.readEntry("Name") && bundledScriptHasExecPath(m_scriptMap[name_], &spec)) {
          static_cast<ExecExternalFetcher::ConfigWidget*>(w)->readConfig(&spec);
        } else {
          kdWarning() << "Fetch::Manager::configWidget() - Can't read config file for " << to.path() << endl;
        }
      }
      break;
    case Yahoo:
      w = new YahooFetcher::ConfigWidget(parent_);
      break;
    case AnimeNfo:
      w = new AnimeNfoFetcher::ConfigWidget(parent_);
      break;
    case IBS:
      w = new IBSFetcher::ConfigWidget(parent_);
      break;
    case ISBNdb:
      w = new ISBNdbFetcher::ConfigWidget(parent_);
      break;
    default:
      kdWarning() << "Fetch::Manager::configWidget() - no widget defined for type = " << type_ << endl;
  }
  return w;
}

// static
QString Manager::typeName(Fetch::Type type_) {
  switch(type_) {
#if AMAZON_SUPPORT
    case Amazon: return AmazonFetcher::defaultName();
#endif
#if IMDB_SUPPORT
    case IMDB: return IMDBFetcher::defaultName();
#endif
#if HAVE_YAZ
    case Z3950: return Z3950Fetcher::defaultName();
#endif
    case SRU: return SRUFetcher::defaultName();
    case Entrez: return EntrezFetcher::defaultName();
    case ExecExternal: return ExecExternalFetcher::defaultName();
    case Yahoo: return YahooFetcher::defaultName();
    case AnimeNfo: return AnimeNfoFetcher::defaultName();
    case IBS: return IBSFetcher::defaultName();
    case ISBNdb: return ISBNdbFetcher::defaultName();
    default: break;
  }
  return QString::null;
}

QPixmap Manager::fetcherIcon(Fetch::Fetcher::CPtr fetcher_) {
#if HAVE_YAZ
  if(fetcher_->type() == Fetch::Z3950) {
    const Fetch::Z3950Fetcher* f = static_cast<const Fetch::Z3950Fetcher*>(fetcher_.data());
    KURL u;
    u.setProtocol(QString::fromLatin1("http"));
    u.setHost(f->host());
    QString icon = favIcon(u);
    if(u.isValid() && !icon.isEmpty()) {
      return SmallIcon(icon);
    }
  } else
#endif
  if(fetcher_->type() == Fetch::ExecExternal) {
    const Fetch::ExecExternalFetcher* f = static_cast<const Fetch::ExecExternalFetcher*>(fetcher_.data());
    QString p = f->execPath();
    KURL u;
    if(p.find(QString::fromLatin1("allocine")) > -1) {
      u = QString::fromLatin1("http://www.allocine.fr");
    } else if(p.find(QString::fromLatin1("ministerio_de_cultura")) > -1) {
      u = QString::fromLatin1("http://www.mcu.es");
    } else if(p.find(QString::fromLatin1("dark_horse_comics")) > -1) {
      u = QString::fromLatin1("http://www.darkhorse.com");
    } else if(f->source().find(QString::fromLatin1("amarok"), 0, false /*case-sensitive*/) > -1) {
      return SmallIcon(QString::fromLatin1("amarok"));
    }
    if(!u.isEmpty() && u.isValid()) {
      QString icon = favIcon(u);
      if(!icon.isEmpty()) {
        return SmallIcon(icon);
      }
    }
  }
  return fetcherIcon(fetcher_->type());
}

QPixmap Manager::fetcherIcon(Fetch::Type type_) {
  QString name;
  switch(type_) {
    case Amazon:
      name = favIcon("http://amazon.com"); break;
    case IMDB:
      name = favIcon("http://imdb.com"); break;
    case Z3950:
      name = QString::fromLatin1("network"); break; // rather arbitrary
    case SRU:
      name = QString::fromLatin1("network_local"); break; // just to be different than z3950
    case Entrez:
      name = favIcon("http://www.ncbi.nlm.nih.gov"); break;
    case ExecExternal:
      name = QString::fromLatin1("exec"); break;
    case Yahoo:
      name = favIcon("http://yahoo.com"); break;
    case AnimeNfo:
      name = favIcon("http://animenfo.com"); break;
    case IBS:
      name = favIcon("http://internetbookshop.it"); break;
    case ISBNdb:
      name = favIcon("http://isbndb.com"); break;
    default:
      kdWarning() << "Fetch::Manager::fetcherIcon() - no pixmap defined for type = " << type_ << endl;
  }

  return name.isEmpty() ? QPixmap() : SmallIcon(name);
}

QString Manager::favIcon(const KURL& url_) {
  DCOPRef kded("kded", "favicons");
  DCOPReply reply = kded.call("iconForURL(KURL)", url_);
  QString result = reply;
  if(reply.isValid() && !result.isEmpty()) {
    return result;
  } else {
    kded.call("downloadHostIcon(KURL)", url_);
  }
  return KMimeType::iconForURL(url_);
}

bool Manager::bundledScriptHasExecPath(const QString& specFile_, KConfig* config_) {
  // make sure ExecPath is set and executable
  // for the bundled scripts, either the exec name is not set, in which case it is the
  // name of the spec file, minus the .spec, or the exec is set, and is local to the dir
  // if not, look for it
  QString exec = config_->readPathEntry("ExecPath");
  QFileInfo specInfo(specFile_), execInfo(exec);
  if(exec.isEmpty() || !execInfo.exists()) {
    exec = specInfo.dirPath(true) + QDir::separator() + specInfo.baseName(true); // remove ".spec"
  } else if(execInfo.isRelative()) {
    exec = specInfo.dirPath(true) + exec;
  } else if(!execInfo.isExecutable()) {
    kdWarning() << "Fetch::Manager::execPathForBundledScript() - not executable: " << specFile_ << endl;
    return false;
  }
  execInfo.setFile(exec);
  if(!execInfo.exists() || !execInfo.isExecutable()) {
    kdWarning() << "Fetch::Manager::execPathForBundledScript() - no exec file for " << specFile_ << endl;
    kdWarning() << "exec = " << exec << endl;
    return false; // we're not ok
  }

  config_->writePathEntry("ExecPath", exec);
  config_->sync(); // might be readonly, but that's ok
  return true;
}

#include "fetchmanager.moc"
