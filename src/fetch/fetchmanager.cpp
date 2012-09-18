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

#include <config.h>

#include "fetchmanager.h"
#include "configwidget.h"
#include "messagehandler.h"
#include "../entry.h"
#include "../collection.h"
#include "../document.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"

#include "fetcherinitializer.h"
#ifdef HAVE_YAZ
#include "z3950fetcher.h"
#endif
#include "srufetcher.h"
#include "execexternalfetcher.h"

#include <kglobal.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>
#include <kio/job.h>
#include <kio/netaccess.h>

#include <QFileInfo>
#include <QDir>

#define LOAD_ICON(name, group, size) \
  KIconLoader::global()->loadIcon(name, static_cast<KIconLoader::Group>(group), size_)

using Tellico::Fetch::Manager;
Manager* Manager::s_self = 0;

Manager::Manager() : QObject(), m_currentFetcherIndex(-1), m_messager(new ManagerMessage()),
                     m_count(0), m_loadDefaults(false) {
  // must create static pointer first
  Q_ASSERT(!s_self);
  s_self = this;
  FetcherInitializer init;
  loadFetchers();

//  m_keyMap.insert(FetchFirst, QString());
  m_keyMap.insert(Title,      i18n("Title"));
  m_keyMap.insert(Person,     i18n("Person"));
  m_keyMap.insert(ISBN,       i18n("ISBN"));
  m_keyMap.insert(UPC,        i18n("UPC/EAN"));
  m_keyMap.insert(Keyword,    i18n("Keyword"));
  m_keyMap.insert(DOI,        i18n("DOI"));
  m_keyMap.insert(ArxivID,    i18n("arXiv ID"));
  m_keyMap.insert(PubmedID,   i18n("PubMed ID"));
  m_keyMap.insert(LCCN,       i18n("LCCN"));
  m_keyMap.insert(Raw,        i18n("Raw Query"));
//  m_keyMap.insert(FetchLast,  QString());
}

Manager::~Manager() {
  delete m_messager;
}

void Manager::registerFunction(int type_, const FetcherFunction& func_) {
  functionRegistry.insert(type_, func_);
}

void Manager::loadFetchers() {
  m_fetchers.clear();
  m_uuidHash.clear();

  KSharedConfigPtr config = KGlobal::config();
  if(config->hasGroup(QLatin1String("Data Sources"))) {
    KConfigGroup configGroup(config, QLatin1String("Data Sources"));
    int nSources = configGroup.readEntry("Sources Count", 0);
    for(int i = 0; i < nSources; ++i) {
      QString group = QString::fromLatin1("Data Source %1").arg(i);
      Fetcher::Ptr f = createFetcher(config, group);
      if(f) {
        m_fetchers.append(f);
        f->setMessageHandler(m_messager);
        m_uuidHash.insert(f->uuid(), f);
      }
    }
    m_loadDefaults = false;
  } else { // add default sources
    m_fetchers = defaultFetchers();
    m_loadDefaults = true;
  }
}

Tellico::Fetch::FetcherVec Manager::fetchers(int type_) {
  FetcherVec vec;
  foreach(Fetcher::Ptr fetcher, m_fetchers) {
    if(fetcher->canFetch(type_)) {
      vec.append(fetcher);
    }
  }
  return vec;
}

Tellico::Fetch::Fetcher::Ptr Manager::fetcherByUuid(const QString& uuid_) {
  return m_uuidHash.contains(uuid_) ? m_uuidHash[uuid_] : Fetcher::Ptr();
}

Tellico::Fetch::KeyMap Manager::keyMap(const QString& source_) const {
  // an empty string means return all
  if(source_.isEmpty()) {
    return m_keyMap;
  }

  // assume there's only one fetcher match
  Fetcher::Ptr foundFetcher;
  foreach(Fetcher::Ptr fetcher, m_fetchers) {
    if(source_ == fetcher->source()) {
      foundFetcher = fetcher;
      break;
    }
  }
  if(!foundFetcher) {
    myWarning() << "no fetcher found!";
    return KeyMap();
  }

  KeyMap map;
  for(KeyMap::ConstIterator it = m_keyMap.constBegin(); it != m_keyMap.constEnd(); ++it) {
    if(foundFetcher->canSearch(it.key())) {
      map.insert(it.key(), it.value());
    }
  }
  return map;
}

void Manager::startSearch(const QString& source_, Tellico::Fetch::FetchKey key_, const QString& value_) {
  if(value_.isEmpty()) {
    emit signalDone();
    return;
  }

  FetchRequest request(Data::Document::self()->collection()->type(), key_, value_);

  // assume there's only one fetcher match
  int i = 0;
  m_currentFetcherIndex = -1;
  foreach(Fetcher::Ptr fetcher, m_fetchers) {
    if(source_ == fetcher->source()) {
      ++m_count; // Fetcher::search() might emit done(), so increment before calling search()
      connect(fetcher.data(), SIGNAL(signalResultFound(Tellico::Fetch::FetchResult*)),
              SIGNAL(signalResultFound(Tellico::Fetch::FetchResult*)));
      connect(fetcher.data(), SIGNAL(signalDone(Tellico::Fetch::Fetcher*)),
              SLOT(slotFetcherDone(Tellico::Fetch::Fetcher*)));
      fetcher->startSearch(request);
      m_currentFetcherIndex = i;
      break;
    }
    ++i;
  }
}

void Manager::continueSearch() {
  if(m_currentFetcherIndex < 0 || m_currentFetcherIndex >= static_cast<int>(m_fetchers.count())) {
    myDebug() << "can't continue!";
    emit signalDone();
    return;
  }
  Fetcher::Ptr fetcher = m_fetchers[m_currentFetcherIndex];
  if(fetcher && fetcher->hasMoreResults()) {
    ++m_count;
    connect(fetcher.data(), SIGNAL(signalResultFound(Tellico::Fetch::FetchResult*)),
            SIGNAL(signalResultFound(Tellico::Fetch::FetchResult*)));
    connect(fetcher.data(), SIGNAL(signalDone(Tellico::Fetch::Fetcher*)),
            SLOT(slotFetcherDone(Tellico::Fetch::Fetcher*)));
    fetcher->continueSearch();
  } else {
    emit signalDone();
  }
}

bool Manager::hasMoreResults() const {
  if(m_currentFetcherIndex < 0 || m_currentFetcherIndex >= static_cast<int>(m_fetchers.count())) {
    return false;
  }
  Fetcher::Ptr fetcher = m_fetchers[m_currentFetcherIndex];
  return fetcher && fetcher->hasMoreResults();
}

void Manager::stop() {
//  DEBUG_LINE;
  foreach(Fetcher::Ptr fetcher, m_fetchers) {
    if(fetcher->isSearching()) {
      fetcher->stop();
    }
  }
#ifndef NDEBUG
  if(m_count != 0) {
    myDebug() << "count should be 0!";
  }
#endif
  m_count = 0;
}

void Manager::slotFetcherDone(Tellico::Fetch::Fetcher* fetcher_) {
//  myDebug() << (fetcher_ ? fetcher_->source() : QString()) << ":" << m_count;
  fetcher_->disconnect(); // disconnect all signals
  --m_count;
  if(m_count <= 0) {
    emit signalDone();
  }
}

bool Manager::canFetch() const {
  foreach(Fetcher::Ptr fetcher, m_fetchers) {
    if(fetcher->canFetch(Data::Document::self()->collection()->type())) {
      return true;
    }
  }
  return false;
}

Tellico::Fetch::Fetcher::Ptr Manager::createFetcher(KSharedConfigPtr config_, const QString& group_) {
  if(!config_->hasGroup(group_)) {
    myDebug() << "no config group for " << group_;
    return Fetcher::Ptr();
  }

  KConfigGroup config(config_, group_);

  int fetchType = config.readEntry("Type", int(Fetch::Unknown));
  if(fetchType == Fetch::Unknown) {
    myDebug() << "unknown type " << fetchType << ", skipping";
    return Fetcher::Ptr();
  }

  Fetcher::Ptr f;
  if(functionRegistry.contains(fetchType)) {
    f = functionRegistry.value(fetchType).create(this);
    f->readConfig(config, group_);
  }
  return f;
}

#define FETCHER_ADD(type) \
  do { \
    if(functionRegistry.contains(type)) { \
      vec.append(functionRegistry.value(type).create(this)); \
    } \
  } while(false)

// static
Tellico::Fetch::FetcherVec Manager::defaultFetchers() {
  FetcherVec vec;
#ifdef ENABLE_IMDB
  FETCHER_ADD(IMDB);
#endif
  vec.append(SRUFetcher::libraryOfCongress(this));
  FETCHER_ADD(ISBNdb);
//  FETCHER_ADD(Yahoo);
  FETCHER_ADD(AnimeNfo);
  FETCHER_ADD(Arxiv);
  FETCHER_ADD(GoogleScholar);
  FETCHER_ADD(Discogs);
  FETCHER_ADD(TheMovieDB);
  FETCHER_ADD(MusicBrainz);
  FETCHER_ADD(BiblioShare);
  FETCHER_ADD(TheGamesDB);
// only add IBS if user includes italian
  if(KGlobal::locale()->languageList().contains(QLatin1String("it"))) {
    FETCHER_ADD(IBS);
  }
#ifdef HAVE_QJSON
  FETCHER_ADD(OpenLibrary);
  FETCHER_ADD(Freebase);
  FETCHER_ADD(GoogleBook);
#endif
  const QStringList langs = KGlobal::locale()->languageList();
  if(langs.contains(QLatin1String("fr"))) {
    FETCHER_ADD(DVDFr);
    FETCHER_ADD(Allocine);
  }
  if(langs.contains(QLatin1String("de"))) {
    FETCHER_ADD(FilmStarts);
  }
  if(langs.contains(QLatin1String("es"))) {
    FETCHER_ADD(SensaCine);
  }
  if(langs.contains(QLatin1String("tr"))) {
    FETCHER_ADD(Beyazperde);
  }
  return vec;
}

#undef FETCHER_ADD

Tellico::Fetch::FetcherVec Manager::createUpdateFetchers(int collType_) {
  if(m_loadDefaults) {
    return defaultFetchers();
  }

  FetcherVec vec;
  KConfigGroup config(KGlobal::config(), "Data Sources");
  int nSources = config.readEntry("Sources Count", 0);
  for(int i = 0; i < nSources; ++i) {
    QString group = QString::fromLatin1("Data Source %1").arg(i);
    // needs the KConfig*
    Fetcher::Ptr fetcher = createFetcher(KGlobal::config(), group);
    if(fetcher && fetcher->canFetch(collType_) && fetcher->canUpdate()) {
      vec.append(fetcher);
    }
  }
  return vec;
}

Tellico::Fetch::FetcherVec Manager::createUpdateFetchers(int collType_, Tellico::Fetch::FetchKey key_) {
  FetcherVec fetchers;
  // creates new fetchers
  FetcherVec allFetchers = createUpdateFetchers(collType_);
  foreach(Fetcher::Ptr fetcher, allFetchers) {
    if(fetcher->canSearch(key_)) {
      fetchers.append(fetcher);
    }
  }
  return fetchers;
}

Tellico::Fetch::Fetcher::Ptr Manager::createUpdateFetcher(int collType_, const QString& source_) {
  Fetcher::Ptr newFetcher;
  // creates new fetchers
  FetcherVec fetchers = createUpdateFetchers(collType_);
  foreach(Fetcher::Ptr fetcher, fetchers) {
    if(fetcher->source() == source_) {
      newFetcher = fetcher;
      break;
    }
  }
  return newFetcher;
}

void Manager::updateStatus(const QString& message_) {
  emit signalStatus(message_);
}

Tellico::Fetch::NameTypeMap Manager::nameTypeMap() {
  Fetch::NameTypeMap map;
  FunctionRegistry::const_iterator it = functionRegistry.constBegin();
  while(it != functionRegistry.constEnd()) {
    map.insert(functionRegistry.value(it.key()).name(), static_cast<Type>(it.key()));
    ++it;
  }

  // now find all the scripts distributed with tellico
  QStringList files = KGlobal::dirs()->findAllResources("appdata", QLatin1String("data-sources/*.spec"),
                                                        KStandardDirs::NoDuplicates);
  foreach(const QString& file, files) {
    KConfig spec(file, KConfig::SimpleConfig);
    KConfigGroup specConfig(&spec, QString());
    QString name = specConfig.readEntry("Name");
    if(name.isEmpty()) {
      myDebug() << "no name for" << file;
      continue;
    }

    if(!bundledScriptHasExecPath(file, specConfig)) { // no available exec
      continue;
    }

    map.insert(name, ExecExternal);
    m_scriptMap.insert(name, file);
  }
  return map;
}


// called when creating a new fetcher
Tellico::Fetch::ConfigWidget* Manager::configWidget(QWidget* parent_, Tellico::Fetch::Type type_, const QString& name_) {
  ConfigWidget* w = 0;
  if(functionRegistry.contains(type_)) {
    w = functionRegistry.value(type_).configWidget(parent_);
  } else {
    myWarning() << "no widget defined for type =" << type_;
  }
  if(type_ == ExecExternal) {
    if(!name_.isEmpty() && m_scriptMap.contains(name_)) {
      // bundledScriptHasExecPath() actually needs to write the exec path
      // back to the config so the configWidget can read it. But if the spec file
      // is not readable, that doesn't work. So work around it with a copy to a temp file
      KTemporaryFile tmpFile;
      tmpFile.setAutoRemove(true);
      tmpFile.open();
      KUrl from, to;
      from.setPath(m_scriptMap[name_]);
      to.setPath(tmpFile.fileName());
      // have to overwrite since KTemporaryFile already created it
      KIO::Job* job = KIO::file_copy(from, to, -1, KIO::Overwrite);
      if(!KIO::NetAccess::synchronousRun(job, 0)) {
        myDebug() << KIO::NetAccess::lastErrorString();
      }
      KConfig spec(to.path(), KConfig::SimpleConfig);
      KConfigGroup specConfig(&spec, QString());
      // pass actual location of spec file
      if(name_ == specConfig.readEntry("Name") && bundledScriptHasExecPath(m_scriptMap[name_], specConfig)) {
        static_cast<ExecExternalFetcher::ConfigWidget*>(w)->readConfig(specConfig);
      } else {
        myWarning() << "Can't read config file for " << to.path();
      }
    }
  }

  return w;
}

// static
QString Manager::typeName(Tellico::Fetch::Type type_) {
  if(self()->functionRegistry.contains(type_)) {
    return self()->functionRegistry.value(type_).name();
  }
  myWarning() << "none found for" << type_;
  return QString();
}

QPixmap Manager::fetcherIcon(Tellico::Fetch::Fetcher::Ptr fetcher_, int group_, int size_) {
#ifdef HAVE_YAZ
  if(fetcher_->type() == Fetch::Z3950) {
    const Fetch::Z3950Fetcher* f = static_cast<const Fetch::Z3950Fetcher*>(fetcher_.data());
    KUrl u;
    u.setProtocol(QLatin1String("http"));
    u.setHost(f->host());
    QString icon = Fetcher::favIcon(u);
    if(u.isValid() && !icon.isEmpty()) {
      return LOAD_ICON(icon, group_, size_);
    }
  } else
#endif
  if(fetcher_->type() == Fetch::ExecExternal) {
    const Fetch::ExecExternalFetcher* f = static_cast<const Fetch::ExecExternalFetcher*>(fetcher_.data());
    const QString p = f->execPath();
    KUrl u;
    if(p.contains(QLatin1String("allocine"))) {
      u = QLatin1String("http://www.allocine.fr");
    } else if(p.contains(QLatin1String("ministerio_de_cultura"))) {
      u = QLatin1String("http://www.mcu.es");
    } else if(p.contains(QLatin1String("dark_horse_comics"))) {
      u = QLatin1String("http://www.darkhorse.com");
    } else if(p.contains(QLatin1String("boardgamegeek"))) {
      u = QLatin1String("http://www.boardgamegeek.com");
    } else if(f->source().contains(QLatin1String("amarok"), Qt::CaseInsensitive)) {
      return LOAD_ICON(QLatin1String("amarok"), group_, size_);
    }
    if(!u.isEmpty() && u.isValid()) {
      QString icon = Fetcher::favIcon(u);
      if(!icon.isEmpty()) {
        return LOAD_ICON(icon, group_, size_);
      }
    }
  }
  return fetcherIcon(fetcher_->type(), group_, size_);
}

QPixmap Manager::fetcherIcon(Tellico::Fetch::Type type_, int group_, int size_) {
  QString name;
  if(self()->functionRegistry.contains(type_)) {
    name = self()->functionRegistry.value(type_).icon();
  } else {
    myWarning() << "no pixmap defined for type =" << type_;
  }

  if(name.isEmpty()) {
    return QPixmap();
  }

  QPixmap pix = KIconLoader::global()->loadIcon(name, static_cast<KIconLoader::Group>(group_),
                                                size_, KIconLoader::DefaultState,
                                                QStringList(), 0L, true);
  if(pix.isNull()) {
    pix = BarIcon(name);
  }
  return pix;
}

Tellico::StringHash Manager::optionalFields(Type type_) {
  if(self()->functionRegistry.contains(type_)) {
    return self()->functionRegistry.value(type_).optionalFields();
  }
  return StringHash();
}

bool Manager::bundledScriptHasExecPath(const QString& specFile_, KConfigGroup& config_) {
  // make sure ExecPath is set and executable
  // for the bundled scripts, either the exec name is not set, in which case it is the
  // name of the spec file, minus the .spec, or the exec is set, and is local to the dir
  // if not, look for it
  QFileInfo specInfo(specFile_);
  QString exec = config_.readPathEntry("ExecPath", QString());
  QFileInfo execInfo(exec);
  if(exec.isEmpty() || !execInfo.exists()) {
    exec = specInfo.canonicalPath() + QDir::separator() + specInfo.completeBaseName(); // remove ".spec"
  } else if(execInfo.isRelative()) {
    exec = specInfo.canonicalPath() + QDir::separator() + exec;
  } else if(!execInfo.isExecutable()) {
    myWarning() << "not executable:" << specFile_;
    return false;
  }
  execInfo.setFile(exec);
  if(!execInfo.exists() || !execInfo.isExecutable()) {
    myWarning() << "no exec file for" << specFile_;
    myWarning() << "exec =" << exec;
    return false; // we're not ok
  }

  config_.writePathEntry("ExecPath", exec);
  config_.sync(); // might be readonly, but that's ok
  return true;
}

#include "fetchmanager.moc"
