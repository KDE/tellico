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
#include "../utils/string_utils.h"
#include "../utils/tellico_utils.h"
#include "../tellico_debug.h"

#ifdef HAVE_YAZ
#include "z3950fetcher.h"
#endif
#include "srufetcher.h"
#include "execexternalfetcher.h"

#include <KLocalizedString>
#include <KIconLoader>
#include <KIO/Job>
#include <KSharedConfig>

#include <QFileInfo>
#include <QDir>
#include <QTemporaryFile>

#define LOAD_ICON(name, group, size) \
  KIconLoader::global()->loadIcon(name, static_cast<KIconLoader::Group>(group), size_)

using Tellico::Fetch::Manager;
Manager* Manager::s_self = nullptr;

Manager::Manager() : QObject(), m_currentFetcherIndex(-1), m_messager(new ManagerMessage()),
                     m_count(0), m_loadDefaults(false) {
  // must create static pointer first
  Q_ASSERT(!s_self);
  s_self = this;
  // no need to load fetchers since the initializer does it for us

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

  KSharedConfigPtr config = KSharedConfig::openConfig();
  if(config->hasGroup(QStringLiteral("Data Sources"))) {
    KConfigGroup configGroup(config, QStringLiteral("Data Sources"));
    int nSources = configGroup.readEntry("Sources Count", 0);
    for(int i = 0; i < nSources; ++i) {
      QString group = QStringLiteral("Data Source %1").arg(i);
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

const Tellico::Fetch::FetcherVec& Manager::fetchers() const {
  return m_fetchers;
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
      fetcher->saveConfig();
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
  fetcher_->saveConfig();
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

  // special case: the BoardGameGeek fetcher was originally implemented as a Ruby script
  // now, it's available with an XML API, so prefer the new version
  // so check for fetcher version and switch to the XML if version is missing or lower
  if(fetchType == Fetch::ExecExternal &&
     config.readPathEntry("ExecPath", QString()).endsWith(QLatin1String("boardgamegeek.rb"))) {
    KConfigGroup generalConfig(config_, QStringLiteral("General Options"));
    if(generalConfig.readEntry("FetchVersion", 0) < 1) {
      fetchType = Fetch::BoardGameGeek;
      generalConfig.writeEntry("FetchVersion", 1);
    }
  }
  // special case: the Bedetheque fetcher was originally implemented as a Python script
  // now, it's available as a builtin data source, so prefer the new version
  // so check for fetcher version and switch to the newer if version is missing or lower
  if(fetchType == Fetch::ExecExternal &&
     config.readPathEntry("ExecPath", QString()).endsWith(QStringLiteral("bedetheque.py"))) {
    KConfigGroup generalConfig(config_, QStringLiteral("General Options"));
    if(generalConfig.readEntry("FetchVersion", 0) < 2) {
      fetchType = Fetch::Bedetheque;
      generalConfig.writeEntry("FetchVersion", 2);
    }
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
  vec.append(SRUFetcher::libraryOfCongress(this));
// books
  FETCHER_ADD(ISBNdb);
  FETCHER_ADD(OpenLibrary);
  FETCHER_ADD(GoogleBook);
// comic books
  FETCHER_ADD(AnimeNfo);
  FETCHER_ADD(Bedetheque);
// bibliographic
  FETCHER_ADD(Arxiv);
  FETCHER_ADD(GoogleScholar);
  FETCHER_ADD(BiblioShare);
  FETCHER_ADD(DBLP);
  FETCHER_ADD(HathiTrust);
// music
  FETCHER_ADD(MusicBrainz);
// video games
  FETCHER_ADD(TheGamesDB);
  FETCHER_ADD(IGDB);
  FETCHER_ADD(VNDB);
  FETCHER_ADD(VideoGameGeek);
// board games
  FETCHER_ADD(BoardGameGeek);
// movies
  FETCHER_ADD(TheMovieDB);
#ifdef ENABLE_IMDB
  FETCHER_ADD(IMDB);
#endif
  QStringList langs = QLocale().uiLanguages();
  if(langs.first().contains(QLatin1Char('-'))) {
    // I'm not sure QT always include two-letter locale codes
    langs << langs.first().section(QLatin1Char('-'), 0, 0);
  }
// only add IBS if user includes italian
  if(langs.contains(QStringLiteral("it"))) {
    FETCHER_ADD(IBS);
  }
  if(langs.contains(QStringLiteral("fr"))) {
    FETCHER_ADD(DVDFr);
    FETCHER_ADD(Allocine);
  }
  if(langs.contains(QStringLiteral("ru"))) {
    FETCHER_ADD(KinoPoisk);
  }
  if(langs.contains(QStringLiteral("ru"))) {
    FETCHER_ADD(KinoPoisk);
  }
  if(langs.contains(QStringLiteral("de"))) {
    FETCHER_ADD(Kino);
  }
  if(langs.contains(QStringLiteral("cn"))) {
    FETCHER_ADD(Douban);
  }
  if(langs.contains(QStringLiteral("dk"))) {
    FETCHER_ADD(DBC);
  }
  return vec;
}

#undef FETCHER_ADD

Tellico::Fetch::FetcherVec Manager::createUpdateFetchers(int collType_) {
  if(m_loadDefaults) {
    return defaultFetchers();
  }

  FetcherVec vec;
  KConfigGroup config(KSharedConfig::openConfig(), "Data Sources");
  int nSources = config.readEntry("Sources Count", 0);
  for(int i = 0; i < nSources; ++i) {
    QString group = QStringLiteral("Data Source %1").arg(i);
    // needs the KConfig*
    Fetcher::Ptr fetcher = createFetcher(KSharedConfig::openConfig(), group);
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
  QStringList files = Tellico::locateAllFiles(QStringLiteral("tellico/data-sources/*.spec"));
  foreach(const QString& file, files) {
    KConfig spec(file, KConfig::SimpleConfig);
    KConfigGroup specConfig(&spec, QString());
    QString name = specConfig.readEntry("Name");
    if(name.isEmpty()) {
      myDebug() << "no name for" << file;
      continue;
    }

    bool enabled = specConfig.readEntry("Enabled", true);
    if(!enabled || !bundledScriptHasExecPath(file, specConfig)) { // no available exec
      continue;
    }

    map.insert(name, ExecExternal);
    m_scriptMap.insert(name, file);
  }
  return map;
}

// called when creating a new fetcher
Tellico::Fetch::ConfigWidget* Manager::configWidget(QWidget* parent_, Tellico::Fetch::Type type_, const QString& name_) {
  ConfigWidget* w = nullptr;
  if(functionRegistry.contains(type_)) {
    w = functionRegistry.value(type_).configWidget(parent_);
  } else {
    myWarning() << "no widget defined for type =" << type_;
  }
  if(w && type_ == ExecExternal) {
    if(!name_.isEmpty() && m_scriptMap.contains(name_)) {
      // bundledScriptHasExecPath() actually needs to write the exec path
      // back to the config so the configWidget can read it. But if the spec file
      // is not readable, that doesn't work. So work around it with a copy to a temp file
      QTemporaryFile tmpFile;
      tmpFile.open();
      QUrl from = QUrl::fromLocalFile(m_scriptMap[name_]);
      QUrl to = QUrl::fromLocalFile(tmpFile.fileName());
      // have to overwrite since QTemporaryFile already created it
      KIO::Job* job = KIO::file_copy(from, to, -1, KIO::Overwrite);
      if(!job->exec()) {
        myDebug() << job->errorString();
      }
      KConfig spec(to.path(), KConfig::SimpleConfig);
      KConfigGroup specConfig(&spec, QString());
      // pass actual location of spec file
      if(name_ == specConfig.readEntry("Name") && bundledScriptHasExecPath(m_scriptMap[name_], specConfig)) {
        w->readConfig(specConfig);
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
  if(fetcher_->type() == Fetch::Z3950) {
#ifdef HAVE_YAZ
    const Fetch::Z3950Fetcher* f = static_cast<const Fetch::Z3950Fetcher*>(fetcher_.data());
    QUrl u;
    u.setScheme(QStringLiteral("http"));
    u.setHost(f->host());
    QString icon = Fetcher::favIcon(u);
    if(!icon.isEmpty()) {
      return LOAD_ICON(icon, group_, size_);
    }
#endif
  } else
  if(fetcher_->type() == Fetch::ExecExternal) {
    const Fetch::ExecExternalFetcher* f = static_cast<const Fetch::ExecExternalFetcher*>(fetcher_.data());
    const QString p = f->execPath();
    QUrl u;
    if(p.contains(QStringLiteral("allocine"))) {
      u = QUrl(QStringLiteral("http://www.allocine.fr"));
    } else if(p.contains(QStringLiteral("ministerio_de_cultura"))) {
      u = QUrl(QStringLiteral("http://www.mcu.es"));
    } else if(p.contains(QStringLiteral("dark_horse_comics"))) {
      u = QUrl(QStringLiteral("http://www.darkhorse.com"));
    } else if(p.contains(QStringLiteral("boardgamegeek"))) {
      u = QUrl(QStringLiteral("http://www.boardgamegeek.com"));
    } else if(p.contains(QStringLiteral("supercat"))) {
      u = QUrl(QStringLiteral("https://evergreen-ils.org"));
    } else if(f->source().contains(QStringLiteral("amarok"), Qt::CaseInsensitive)) {
      return LOAD_ICON(QStringLiteral("amarok"), group_, size_);
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
    // use default tellico application icon
    name = QStringLiteral("tellico");
  }

  QPixmap pix = KIconLoader::global()->loadIcon(name, static_cast<KIconLoader::Group>(group_),
                                                size_, KIconLoader::DefaultState,
                                                QStringList(), nullptr, true);
  if(pix.isNull()) {
    QIcon icon = QIcon::fromTheme(name);
    const int groupSize = KIconLoader::global()->currentSize(static_cast<KIconLoader::Group>(group_));
    size_ = size_ == 0 ? groupSize : size_;
    pix = icon.pixmap(size_, size_);
  }
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
