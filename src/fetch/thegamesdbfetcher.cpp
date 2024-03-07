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
#include "../gui/combobox.h"
#include "../core/filehandler.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../utils/mapvalue.h"
#include "../utils/tellico_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KJob>
#include <KJobUiDelegate>
#include <KJobWidgets>
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
#include <QTimer>

namespace {
  static const int THEGAMESDB_MAX_RETURNS_TOTAL = 20;
  static const char* THEGAMESDB_API_URL = "https://api.thegamesdb.net";
  static const char* THEGAMESDB_API_VERSION = "1"; // krazy:exclude=doublequote_chars
  static const char* THEGAMESDB_MAGIC_TOKEN = "f7c4fd9c5d6d4a2fcefe3157192f87e260038abe86b0f3977716596edaebdbb82315586e98fc88b0fb9ff4c01576e4d47b4e556d487a4325221abbddfac36f59d7e114753b5fa6c77a1e73423d5f72460f3b526bcbae4f2be0d86a5854600436784e3a5c5d6bc1a3e2d395f798fb35073051f2c232014023e9dda99edfea5767";
}

using namespace Tellico;
using Tellico::Fetch::TheGamesDBFetcher;

TheGamesDBFetcher::TheGamesDBFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false)
    , m_imageSize(SmallImage) {
  m_apiKey = Tellico::reverseObfuscate(THEGAMESDB_MAGIC_TOKEN);
  // delay reading the platform names from the cache file
  QTimer::singleShot(0, this, &TheGamesDBFetcher::loadCachedData);
}

TheGamesDBFetcher::~TheGamesDBFetcher() {
}

QString TheGamesDBFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool TheGamesDBFetcher::canSearch(Fetch::FetchKey k) const {
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
  const int imageSize = config_.readEntry("Image Size", -1);
  if(imageSize > -1) {
    m_imageSize = static_cast<ImageSize>(imageSize);
  }
}

void TheGamesDBFetcher::search() {
  m_started = true;

  QUrl u(QString::fromLatin1(THEGAMESDB_API_URL));
  u.setPath(QLatin1String("/v") + QLatin1String(THEGAMESDB_API_VERSION));

  switch(request().key()) {
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
        q.addQueryItem(QStringLiteral("name"), request().value());
        if(!request().data().isEmpty()) {
          q.addQueryItem(QStringLiteral("filter[platform]"), request().data());
        }
        u.setQuery(q);
      }
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }

  if(m_apiKey.isEmpty()) {
    myDebug() << source() << "- empty API key";
    message(i18n("An access key is required to use this data source.")
            + QLatin1Char(' ') +
            i18n("Those values must be entered in the data source settings."), MessageHandler::Error);
    stop();
    return;
  }
//  myDebug() << u;

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result, this, &TheGamesDBFetcher::slotComplete);
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

  const QString tgdb = QStringLiteral("tgdb-id");
  const QString screenshot = QStringLiteral("screenshot");
  if(optionalFields().contains(screenshot)) {
    if(!entry->collection()->hasField(screenshot)) {
      entry->collection()->addField(Data::Field::createDefaultField(Data::Field::ScreenshotField));
    }
    QUrl u(QString::fromLatin1(THEGAMESDB_API_URL));
    u.setPath(QLatin1String("/v") + QLatin1String(THEGAMESDB_API_VERSION) + QLatin1String("/Games/Images"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("apikey"), m_apiKey);
    q.addQueryItem(QStringLiteral("games_id"), entry->field(tgdb));
    q.addQueryItem(QStringLiteral("filter[type]"), screenshot);
    u.setQuery(q);

    QByteArray data = FileHandler::readDataFile(u, true);
    QVariantMap topLevelMap = QJsonDocument::fromJson(data).object().toVariantMap();
    readCoverList(topLevelMap.value(QStringLiteral("data")).toMap());

    const QString screenshot_key = QLatin1Char('s') + entry->field(tgdb);
    if(m_covers.contains(screenshot_key)) {
      const QString screenshot_url = m_covers.value(screenshot_key);
      const QString id = ImageFactory::addImage(QUrl::fromUserInput(screenshot_url), true /* quiet */);
      entry->setField(screenshot, id);
    }
  }

  // don't want to include TGDb ID field
  entry->setField(tgdb, QString());

  return entry;
}

Tellico::Fetch::FetchRequest TheGamesDBFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString platform = entry_->field(QStringLiteral("platform"));
  int platformId = -1;
  // if the platform id is available, it can be used to filter the update search
  if(!platform.isEmpty()) {
    for(auto i = m_platforms.constBegin(); i != m_platforms.constEnd(); ++i) {
      if(i.value() == platform) {
        platformId = i.key();
        break;
      }
    }
  }

  const QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    FetchRequest req(Title, title);
    if(platformId > -1) {
      req.setData(QString::number(platformId));
    }
    return req;
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
                              .value(QStringLiteral("platform")).toMap());
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

    FetchResult* r = new FetchResult(this, entry);
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

  const int platformId = mapValue(resultMap_, "platform").toInt();
  if(m_platforms.contains(platformId)) {
    const QString platform = m_platforms[platformId];
    // make the assumption that if the platform name isn't already in the allowed list, it should be added
    Data::FieldPtr f = entry_->collection()->fieldByName(QStringLiteral("platform"));
    if(f && !f->allowed().contains(platform)) {
      f->setAllowed(QStringList(f->allowed()) << platform);
    }
    entry_->setField(QStringLiteral("platform"), platform);
  }

  const QString esrb = mapValue(resultMap_, "rating")
                       .section(QLatin1Char('-'), 0, 0)
                       .trimmed(); // value is like "T - Teen"
  Data::GameCollection::EsrbRating rating = Data::GameCollection::UnknownEsrb;
  if(esrb == QLatin1String("U"))         rating = Data::GameCollection::Unrated;
  else if(esrb == QLatin1String("T"))    rating = Data::GameCollection::Teen;
  else if(esrb == QLatin1String("E"))    rating = Data::GameCollection::Everyone;
  else if(esrb == QLatin1String("E10+")) rating = Data::GameCollection::Everyone10;
  else if(esrb == QLatin1String("EC"))   rating = Data::GameCollection::EarlyChildhood;
  else if(esrb == QLatin1String("A"))    rating = Data::GameCollection::Adults;
  else if(esrb == QLatin1String("M"))    rating = Data::GameCollection::Mature;
  else if(esrb == QLatin1String("RP"))   rating = Data::GameCollection::Pending;
  if(rating != Data::GameCollection::UnknownEsrb) {
    entry_->setField(QStringLiteral("certification"), Data::GameCollection::esrbRating(rating));
  }

  if(m_imageSize != NoImage) {
    const QString coverUrl = m_covers.value(mapValue(resultMap_, "id"));
    entry_->setField(QStringLiteral("cover"), coverUrl);
  }

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
    const QVariantMap map = i.value().toMap();
    const QString name = map.value(QStringLiteral("name")).toString();
    m_platforms.insert(i.key().toInt(), Data::GameCollection::normalizePlatform(name));
  }

  // now write it to cache again
  const QString id = QStringLiteral("id");
  const QString name = QStringLiteral("name");
  QJsonObject platformObj;
  for(auto ii = m_platforms.constBegin(); ii != m_platforms.constEnd(); ++ii) {
    QJsonObject iObj;
    iObj.insert(id, ii.key());
    iObj.insert(name, ii.value());
    platformObj.insert(QString::number(ii.key()), iObj);
  }
  QJsonObject dataObj;
  dataObj.insert(QStringLiteral("platforms"), platformObj);
  QJsonObject docObj;
  docObj.insert(QStringLiteral("data"), dataObj);
  QJsonDocument doc;
  doc.setObject(docObj);
  writeDataList(Platform, doc.toJson());
}

void TheGamesDBFetcher::readCoverList(const QVariantMap& coverDataMap_) {
  // first, get the base url
  QString imageBase;
  switch(m_imageSize) {
    case SmallImage:
      // this is the default size, using the thumb. Not the small size
      imageBase = QStringLiteral("thumb");
      break;
    case MediumImage:
      imageBase = QStringLiteral("medium");
      break;
    case LargeImage:
      imageBase = QStringLiteral("large");
      break;
    case NoImage:
      m_covers.clear();
      return; // no need to read anything
      break;
  }

  QString baseUrl =  coverDataMap_.value(QStringLiteral("base_url")).toMap()
                                  .value(imageBase).toString();

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

  // these are probably screenshots
  QVariantMap imagesMap = coverDataMap_.value(QStringLiteral("images")).toMap();
  QMapIterator<QString, QVariant> i2(imagesMap);
  while(i2.hasNext()) {
    i2.next();
    foreach(QVariant v, i2.value().toList()) {
      QVariantMap map = v.toMap();
      if(map.value(QStringLiteral("type")) == QLatin1String("screenshot")) {
        m_covers.insert(QLatin1Char('s') + i2.key(), baseUrl + mapValue(map, "filename"));
        break;
      }
    }
  }
}

void TheGamesDBFetcher::loadCachedData() {
  // The lists of genres, publishers, and developers are separate, with TGDB requesting that
  // the data be cached heavily and only updated when necessary
  // read the three cached JSON data file for genres, publishers, and developers
  // the platform info is sent with each request response, so it doesn't necessarily need
  // to be cache. But if an update request is used, having the cached platform id is helpful

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

  QFile platformFile(dataFileName(Platform));
  if(platformFile.open(QIODevice::ReadOnly)) {
    updateData(Platform, platformFile.readAll());
  }
}

void TheGamesDBFetcher::updateData(TgdbDataType dataType_, const QByteArray& jsonData_) {
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
    case Platform:
      dataName = QStringLiteral("platforms");
      break;
  }

  QHash<int, QString> dataHash;
  const QVariantMap topMap = QJsonDocument::fromJson(jsonData_).object().toVariantMap();
  const QVariantMap resultMap = topMap.value(QStringLiteral("data")).toMap()
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
    case Platform:
      m_platforms = dataHash;
      break;
  }
}

void TheGamesDBFetcher::readDataList(TgdbDataType dataType_) {
  QUrl u(QString::fromLatin1(THEGAMESDB_API_URL));
  u.setPath(QLatin1String("/v") + QLatin1String(THEGAMESDB_API_VERSION));
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
    case Platform:
      myDebug() << "not trying to read platforms";
      // platforms are not read independently, and are only cached
      return;
  }
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("apikey"), m_apiKey);
  u.setQuery(q);

//  u = QUrl::fromLocalFile(dataFileName(dataType_)); // for testing
//  myDebug() << "Reading" << u;
  const QByteArray data = FileHandler::readDataFile(u, true);
  writeDataList(dataType_, data);
  updateData(dataType_, data);
}

void TheGamesDBFetcher::writeDataList(TgdbDataType dataType_, const QByteArray& data_) {
  QFile file(dataFileName(dataType_));
  if(!file.open(QIODevice::WriteOnly) || file.write(data_) == -1) {
    myDebug() << "unable to write to" << file.fileName() << file.errorString();
    return;
  }
  file.close();
}

Tellico::Fetch::ConfigWidget* TheGamesDBFetcher::configWidget(QWidget* parent_) const {
  return new TheGamesDBFetcher::ConfigWidget(parent_, this);
}

QString TheGamesDBFetcher::defaultName() {
  return QStringLiteral("TheGamesDB");
}

QString TheGamesDBFetcher::defaultIcon() {
  // favicon is too big for the KIO job to download
  return favIcon(QUrl(QLatin1String("https://thegamesdb.net")),
                 QUrl(QLatin1String("https://tellico-project.org/img/thegamesdb-favicon.ico")));
}

Tellico::StringHash TheGamesDBFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("num-player")] = i18n("Number of Players");
  hash[QStringLiteral("screenshot")] = i18n("Screenshot");
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
    case Platform:
      fileName = dataDir + QLatin1String("platforms.json");
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
  QLabel* al = new QLabel(i18n("Registration is required for accessing this data source. "
                               "If you agree to the terms and conditions, <a href='%1'>sign "
                               "up for an account</a>, and enter your information below.",
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
  connect(m_apiKeyEdit, &QLineEdit::textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_apiKeyEdit, row, 1);
  label->setBuddy(m_apiKeyEdit);

  label = new QLabel(i18n("&Image size: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_imageCombo = new GUI::ComboBox(optionsWidget());
  m_imageCombo->addItem(i18n("Small Image"), SmallImage);
  m_imageCombo->addItem(i18n("Medium Image"), MediumImage);
  m_imageCombo->addItem(i18n("Large Image"), LargeImage);
  m_imageCombo->addItem(i18n("No Image"), NoImage);
  void (GUI::ComboBox::* activatedInt)(int) = &GUI::ComboBox::activated;
  connect(m_imageCombo, activatedInt, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_imageCombo, row, 1);
  QString w = i18n("The cover image may be downloaded as well. However, too many large images in the "
                   "collection may degrade performance.");
  label->setWhatsThis(w);
  m_imageCombo->setWhatsThis(w);
  label->setBuddy(m_imageCombo);

  l->setRowStretch(++row, 10);

  if(fetcher_) {
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
    m_imageCombo->setCurrentData(fetcher_->m_imageSize);
  } else { // defaults
    m_apiKeyEdit->setText(Tellico::reverseObfuscate(THEGAMESDB_MAGIC_TOKEN));
    m_imageCombo->setCurrentData(SmallImage);
  }

  // now add additional fields widget
  addFieldsWidget(TheGamesDBFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void TheGamesDBFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty() && apiKey != Tellico::reverseObfuscate(THEGAMESDB_MAGIC_TOKEN)) {
    config_.writeEntry("API Key", apiKey);
  }
  const int n = m_imageCombo->currentData().toInt();
  config_.writeEntry("Image Size", n);
}

QString TheGamesDBFetcher::ConfigWidget::preferredName() const {
  return TheGamesDBFetcher::defaultName();
}
