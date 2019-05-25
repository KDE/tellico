/***************************************************************************
    Copyright (C) 2012-2019 Robby Stephenson <robby@periapsis.org>
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

#include "thegamesdbfetcher.h"
#include "../collections/gamecollection.h"
#include "../images/imagefactory.h"
#include "../core/filehandler.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../utils/tellico_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KJob>
#include <KJobUiDelegate>
#include <KJobWidgets/KJobWidgets>
#include <KIO/StoredTransferJob>

#include <QLabel>
#include <QLineEdit>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QTextCodec>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrlQuery>

namespace {
  static const int THEGAMESDB_MAX_RETURNS_TOTAL = 20;
  static const char* THEGAMESDB_API_URL = "https://api.thegamesdb.net";
  static const char* THEGAMESDB_API_VERSION = "1.1"; // krazy:exclude=doublequote_chars
  static const char* THEGAMESDB_MAGIC_TOKEN = "f7c4fd9c5d6d4a2fcefe3157192f87e260038abe86b0f3977716596edaebdbb82315586e98fc88b0fb9ff4c01576e4d47b4e556d487a4325221abbddfac36f59d7e114753b5fa6c77a1e73423d5f72460f3b526bcbae4f2be0d86a5854600436784e3a5c5d6bc1a3e2d395f798fb35073051f2c232014023e9dda99edfea5767";
}

using namespace Tellico;
using Tellico::Fetch::TheGamesDBFetcher;

TheGamesDBFetcher::TheGamesDBFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false) {
  m_apiKey = Tellico::reverseObfuscate(THEGAMESDB_MAGIC_TOKEN);
  loadCachedData();
}

TheGamesDBFetcher::~TheGamesDBFetcher() {
}

QString TheGamesDBFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool TheGamesDBFetcher::canSearch(FetchKey k) const {
  return k == Title;
}

bool TheGamesDBFetcher::canFetch(int type) const {
  return type == Data::Collection::Game;
}

void TheGamesDBFetcher::readConfigHook(const KConfigGroup& config_) {
  const QString k = config_.readEntry("API Key");
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
}

void TheGamesDBFetcher::search() {
  m_started = true;

  if(m_apiKey.isEmpty()) {
    myDebug() << "empty API key";
    message(i18n("An access key is required to use this data source.")
            + QLatin1Char(' ') +
            i18n("Those values must be entered in the data source settings."), MessageHandler::Error);
    stop();
    return;
  }

  QUrl u(QString::fromLatin1(THEGAMESDB_API_URL));
  u.setPath(QLatin1String("/v") + QLatin1String(THEGAMESDB_API_VERSION));

  switch(request().key) {
    case Title:
      u = u.adjusted(QUrl::StripTrailingSlash);
      u.setPath(u.path() + QLatin1String("/Games/ByGameName"));
      {
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("apikey"), m_apiKey);
        if(optionalFields().contains(QStringLiteral("num-player"))) {
          q.addQueryItem(QStringLiteral("fields"), QStringLiteral("players,rating,publishers,genres,overview,platform"));
        } else {
          q.addQueryItem(QStringLiteral("fields"), QStringLiteral("rating,publishers,genres,overview,platform"));
        }
        q.addQueryItem(QStringLiteral("include"), QStringLiteral("platform,boxart"));
        q.addQueryItem(QStringLiteral("name"), request().value);
        u.setQuery(q);
      }
      break;

    default:
      myWarning() << "key not recognized:" << request().key;
      stop();
      return;
  }
//  u = QUrl::fromLocalFile(QStringLiteral("/tmp/test-tgdb.json"));
//  myDebug() << u;

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
}

void TheGamesDBFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
    m_job = nullptr;
  }
  m_started = false;
  emit signalDone(this);
}

Tellico::Data::EntryPtr TheGamesDBFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  // image might still be a URL
  const QString image_id = entry->field(QStringLiteral("cover"));
  if(image_id.contains(QLatin1Char('/'))) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(image_id), true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QStringLiteral("cover"), id);
  }

  // don't want to include TGDb ID field
  entry->setField(QStringLiteral("tgdb-id"), QString());

  return entry;
}

Tellico::Fetch::FetchRequest TheGamesDBFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

void TheGamesDBFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  const QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = nullptr;

#if 0
  myWarning() << "Remove debug from thegamesdbfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test-tgdb.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  Data::CollPtr coll(new Data::GameCollection(true));
  // always add the tgdb-id for fetchEntryHook
  Data::FieldPtr field(new Data::Field(QStringLiteral("tgdb-id"), QStringLiteral("TGDb ID"), Data::Field::Line));
  field->setCategory(i18n("General"));
  coll->addField(field);

  if(optionalFields().contains(QStringLiteral("num-player"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("num-player"), i18n("Number of Players"), Data::Field::Number));
    field->setCategory(i18n("General"));
    field->setFlags(Data::Field::AllowMultiple | Data::Field::AllowGrouped);
    coll->addField(field);
  }

  QVariantMap topLevelMap = QJsonDocument::fromJson(data).object().toVariantMap();
  if(!topLevelMap.contains(QStringLiteral("data"))) {
    myDebug() << "No data in result!";
  }
  readPlatformList(topLevelMap.value(QStringLiteral("include")).toMap()
                              .value(QStringLiteral("platform")).toMap()
                              .value(QStringLiteral("data")).toMap());
  readCoverList(topLevelMap.value(QStringLiteral("include")).toMap()
                           .value(QStringLiteral("boxart")).toMap());

  QVariantList resultList = topLevelMap.value(QStringLiteral("data")).toMap()
                                       .value(QStringLiteral("games")).toList();
  if(resultList.isEmpty()) {
    myDebug() << "no results";
    stop();
    return;
  }

  int count = 0;
  foreach(const QVariant& result, resultList) {
    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toMap());

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
    ++count;
    if(count >= THEGAMESDB_MAX_RETURNS_TOTAL) {
      break;
    }
  }

  stop();
}

void TheGamesDBFetcher::populateEntry(Data::EntryPtr entry_, const QVariantMap& resultMap_) {
  entry_->setField(QStringLiteral("tgdb-id"), mapValue(resultMap_, "id"));
  entry_->setField(QStringLiteral("title"), mapValue(resultMap_, "game_title"));
  entry_->setField(QStringLiteral("year"),  mapValue(resultMap_, "release_date").left(4));
  entry_->setField(QStringLiteral("description"), mapValue(resultMap_, "overview"));

  const QString platformId = mapValue(resultMap_, "platform");
  if(m_platforms.contains(platformId)) {
    entry_->setField(QStringLiteral("platform"), m_platforms[platformId]);
  }

  const QString esrb = mapValue(resultMap_, "rating")
                       .section(QLatin1Char('-'), 1, 1)
                       .trimmed(); // value is like "T - Teen"
  if(!esrb.isEmpty()) {
    entry_->setField(QStringLiteral("certification"), i18n(esrb.toUtf8().constData()));
  }
  const QString coverUrl = m_covers.value(mapValue(resultMap_, "id"));
  entry_->setField(QStringLiteral("cover"), coverUrl);

  QStringList genres, pubs, devs;

  bool alreadyAttemptedLoad = false;
  QVariantList genreIdList = resultMap_.value(QStringLiteral("genres")).toList();
  foreach(const QVariant& v, genreIdList) {
    const int id = v.toInt();
    if(!m_genres.contains(id) && !alreadyAttemptedLoad) {
      readDataList(Genre);
      alreadyAttemptedLoad = true;
    }
    if(m_genres.contains(id)) {
      genres << m_genres[id];
    }
  }

  alreadyAttemptedLoad = false;
  QVariantList pubList = resultMap_.value(QStringLiteral("publishers")).toList();
  foreach(const QVariant& v, pubList) {
    const int id = v.toInt();
    if(!m_publishers.contains(id) && !alreadyAttemptedLoad) {
      readDataList(Publisher);
      alreadyAttemptedLoad = true;
    }
    if(m_publishers.contains(id)) {
      pubs << m_publishers[id];
    }
  }

  alreadyAttemptedLoad = false;
  QVariantList devList = resultMap_.value(QStringLiteral("developers")).toList();
  foreach(const QVariant& v, devList) {
    const int id = v.toInt();
    if(!m_developers.contains(id) && !alreadyAttemptedLoad) {
      readDataList(Developer);
      alreadyAttemptedLoad = true;
    }
    if(m_developers.contains(id)) {
      devs << m_developers[id];
    }
  }

  entry_->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));
  entry_->setField(QStringLiteral("publisher"), pubs.join(FieldFormat::delimiterString()));
  entry_->setField(QStringLiteral("developer"), devs.join(FieldFormat::delimiterString()));

if(entry_->collection()->hasField(QStringLiteral("num-player"))) {
    entry_->setField(QStringLiteral("num-player"), mapValue(resultMap_, "players"));
  }
}

void TheGamesDBFetcher::readPlatformList(const QVariantMap& platformMap_) {
  QMapIterator<QString, QVariant> i(platformMap_);
  while(i.hasNext()) {
    i.next();
    QVariantMap map = i.value().toMap();
    QString name = map.value(QStringLiteral("name")).toString();
    if(name.startsWith(QLatin1String("Microsoft"))) {
      name = name.mid(sizeof("Microsoft"));
    } else if(name.startsWith(QLatin1String("Sony Playstation"))) {
      // default video game collection has no space between 'PlayStation' and #
      name = QLatin1String("PlayStation") + name.mid(sizeof("Sony Playstation"));
    } else if(name == QLatin1String("Nintendo GameCube")) {
      name = i18n("GameCube");
    } else if(name == QLatin1String("Nintendo Game Boy Advance")) {
      name = i18n("Game Boy Advance");
    } else if(name.startsWith(QLatin1String("Nintendo Wii"))) {
      name = i18n("Nintendo Wii");
    } else if(name == QLatin1String("Super Nintendo (SNES)")) {
      name = i18n("Super Nintendo");
    } else if(name == QLatin1String("PC")) {
      name = i18nc("Windows Platform", "Windows");
    }
    m_platforms.insert(i.key(), name);
  }
}

void TheGamesDBFetcher::readCoverList(const QVariantMap& coverDataMap_) {
  // first, get the base url
  QString baseUrl =  coverDataMap_.value(QStringLiteral("base_url")).toMap()
                                  .value(QStringLiteral("thumb")).toString();

  QVariantMap coverMap = coverDataMap_.value(QStringLiteral("data")).toMap();
  QMapIterator<QString, QVariant> i(coverMap);
  while(i.hasNext()) {
    i.next();
    foreach(QVariant v, i.value().toList()) {
      QVariantMap map = v.toMap();
      if(map.value(QStringLiteral("type")) == QLatin1String("boxart") &&
         map.value(QStringLiteral("side")) == QLatin1String("front")) {
        m_covers.insert(i.key(), baseUrl + mapValue(map, "filename"));
        break;
      }
    }
  }
}

void TheGamesDBFetcher::loadCachedData() {
  // The lists of genres, publishers, and developers are separate, with TGDB requesting that
  // the data be cached heavily and only updated when necessary
  // read the three cached JSON data file for genres, publishers, and developers

  QFile genreFile(dataFileName(Genre));
  if(genreFile.open(QIODevice::ReadOnly)) {
    updateData(Genre, genreFile.readAll());
  }

  QFile publisherFile(dataFileName(Publisher));
  if(publisherFile.open(QIODevice::ReadOnly)) {
    updateData(Publisher, publisherFile.readAll());
  }

  QFile developerFile(dataFileName(Developer));
  if(developerFile.open(QIODevice::ReadOnly)) {
    updateData(Developer, developerFile.readAll());
  }
}

void TheGamesDBFetcher::updateData(TgdbDataType dataType_, const QByteArray& jsonData_) {
  const QString dataString = QStringLiteral("data");
  QString dataName;
  switch(dataType_) {
    case Genre:
      dataName = QStringLiteral("genres");
      break;
    case Publisher:
      dataName = QStringLiteral("publishers");
      break;
    case Developer:
      dataName = QStringLiteral("developers");
      break;
  }

  QHash<int, QString> dataHash;
  const QVariantMap topMap = QJsonDocument::fromJson(jsonData_).object().toVariantMap();
  const QVariantMap resultMap = topMap.value(dataString).toMap()
                                        .value(dataName).toMap();
  for(QMapIterator<QString, QVariant> i(resultMap); i.hasNext(); ) {
    i.next();
    const QVariantMap m = i.value().toMap();
    dataHash.insert(m.value(QStringLiteral("id")).toInt(), mapValue(m, "name"));
  }

  // transfer read data into the correct local variable
  switch(dataType_) {
    case Genre:
      m_genres = dataHash;
      break;
    case Publisher:
      m_publishers = dataHash;
      break;
    case Developer:
      m_developers = dataHash;
      break;
  }
}

void TheGamesDBFetcher::readDataList(TgdbDataType dataType_) {
  QUrl u(QString::fromLatin1(THEGAMESDB_API_URL));
  switch(dataType_) {
    case Genre:
      u.setPath(u.path() + QLatin1String("/Genres"));
      break;
    case Publisher:
      u.setPath(u.path() + QLatin1String("/Publishers"));
      break;
    case Developer:
      u.setPath(u.path() + QLatin1String("/Developers"));
      break;
  }
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("apikey"), m_apiKey);
  u.setQuery(q);

//  u = QUrl::fromLocalFile(dataFileName(dataType_)); // for testing
//  myDebug() << "Reading" << u;
  const QByteArray data = FileHandler::readDataFile(u, true);
  QFile file(dataFileName(dataType_));
  if(!file.open(QIODevice::WriteOnly) || file.write(data) == -1) {
    myDebug() << "unable to write to" << dataFileName(dataType_);
    return;
  }
  file.close();
  updateData(dataType_, data);
}


Tellico::Fetch::ConfigWidget* TheGamesDBFetcher::configWidget(QWidget* parent_) const {
  return new TheGamesDBFetcher::ConfigWidget(parent_, this);
}

QString TheGamesDBFetcher::defaultName() {
  return QStringLiteral("TheGamesDB");
}

QString TheGamesDBFetcher::defaultIcon() {
  return favIcon("https://thegamesdb.net");
}

Tellico::StringHash TheGamesDBFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("num-player")] = i18n("Number of Players");
  return hash;
}

QString TheGamesDBFetcher::dataFileName(TgdbDataType dataType_) {
  const QString dataDir = Tellico::saveLocation(QStringLiteral("thegamesdb-data/"));
  QString fileName;
  switch(dataType_) {
    case Genre:
      fileName = dataDir + QLatin1String("genres.json");
      break;
    case Publisher:
      fileName = dataDir + QLatin1String("publishers.json");
      break;
    case Developer:
      fileName = dataDir + QLatin1String("developers.json");
      break;
  }
  return fileName;
}

TheGamesDBFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const TheGamesDBFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;
  QLabel* al = new QLabel(i18n("Registration is required for accessing the %1 data source. "
                               "If you agree to the terms and conditions, <a href='%2'>sign "
                               "up for an account</a>, and enter your information below.",
                                preferredName(),
                                QLatin1String("https://forums.thegamesdb.net/viewforum.php?f=10")),
                          optionsWidget());
  al->setOpenExternalLinks(true);
  al->setWordWrap(true);
  ++row;
  l->addWidget(al, row, 0, 1, 2);
  // richtext gets weird with size
  al->setMinimumWidth(al->sizeHint().width());

  QLabel* label = new QLabel(i18n("Access key: "), optionsWidget());
  l->addWidget(label, ++row, 0);

  m_apiKeyEdit = new QLineEdit(optionsWidget());
  connect(m_apiKeyEdit, SIGNAL(textChanged(QString)), SLOT(slotSetModified()));
  l->addWidget(m_apiKeyEdit, row, 1);
  label->setBuddy(m_apiKeyEdit);

  l->setRowStretch(++row, 10);

  if(fetcher_) {
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
  }

  // now add additional fields widget
  addFieldsWidget(TheGamesDBFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void TheGamesDBFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty() && apiKey != Tellico::reverseObfuscate(THEGAMESDB_MAGIC_TOKEN)) {
    config_.writeEntry("API Key", apiKey);
  }
}

QString TheGamesDBFetcher::ConfigWidget::preferredName() const {
  return TheGamesDBFetcher::defaultName();
}
