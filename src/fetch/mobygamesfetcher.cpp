/***************************************************************************
    Copyright (C) 2019-2020 Robby Stephenson <robby@periapsis.org>
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

#include "mobygamesfetcher.h"
#include "../collections/gamecollection.h"
#include "../images/imagefactory.h"
#include "../gui/combobox.h"
#include "../core/filehandler.h"
#include "../utils/guiproxy.h"
#include "../utils/mapvalue.h"
#include "../utils/tellico_utils.h"
#include "../core/tellico_strings.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KJob>
#include <KJobUiDelegate>
#include <KJobWidgets>
#include <KIO/StoredTransferJob>

#include <QUrl>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <QThread>
#include <QTimer>

namespace {
  static const int MOBYGAMES_MAX_RETURNS_TOTAL = 10;
  static const char* MOBYGAMES_API_URL = "https://api.mobygames.com/v1";
}

using namespace Tellico;
using Tellico::Fetch::MobyGamesFetcher;

MobyGamesFetcher::MobyGamesFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false)
    , m_imageSize(SmallImage)
    , m_requestPlatformId(0) {
  //  setLimit(MOBYGAMES_MAX_RETURNS_TOTAL);
  m_idleTime.start();
  // delay reading the platform names from the cache file
  QTimer::singleShot(0, this, &MobyGamesFetcher::populateHashes);
}

MobyGamesFetcher::~MobyGamesFetcher() {
}

QString MobyGamesFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString MobyGamesFetcher::attribution() const {
  return TC_I18N3(providedBy, QLatin1String("https://mobygames.com"), QLatin1String("MobyGames"));
}

bool MobyGamesFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == Keyword;
}

bool MobyGamesFetcher::canFetch(int type) const {
  return type == Data::Collection::Game;
}

void MobyGamesFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key");
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
  const int imageSize = config_.readEntry("Image Size", -1);
  if(imageSize > -1) {
    m_imageSize = static_cast<ImageSize>(imageSize);
  }
}

void MobyGamesFetcher::search() {
  continueSearch();
}

void MobyGamesFetcher::continueSearch() {
  m_started = true;
  m_requestPlatformId = 0;

  QUrl u(QString::fromLatin1(MOBYGAMES_API_URL));
  u.setPath(u.path() + QStringLiteral("/games"));

  QUrlQuery q;
  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("title"), request().value());
      break;

    case Keyword:
      {
      // figure out if the platform is part of the search string
      int pId = 0;
      QString value = request().value(); // resulting value
      QString matchedPlatform;
      // iterate over all known platforms; this doesn't seem to be too much of a performance hit
      QHash<int, QString>::const_iterator i = m_platforms.constBegin();
      while(i != m_platforms.constEnd()) {
        // don't forget that some platform names are substrings of others, like Wii and WiiU
        if(i.value().length() > matchedPlatform.length() && request().value().contains(i.value())) {
          pId = i.key();
          matchedPlatform = i.value();
          QString v = request().value(); // reset search value
          v.remove(matchedPlatform); // remove platform from search value
          value = v.simplified();
          // can't break, because of potential substring platform name
        }
        ++i;
      }
      q.addQueryItem(QStringLiteral("title"), value);
      if(pId > 0) {
        m_requestPlatformId = pId;
        q.addQueryItem(QStringLiteral("platform"), QString::number(pId));
      }
      }
      break;

    case Raw:
      q.setQuery(request().value());
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

  q.addQueryItem(QStringLiteral("api_key"), m_apiKey);
  q.addQueryItem(QStringLiteral("limit"), QString::number(MOBYGAMES_MAX_RETURNS_TOTAL));
  q.addQueryItem(QStringLiteral("format"), QStringLiteral("normal"));
  u.setQuery(q);
//  u = QUrl::fromLocalFile(QStringLiteral("/home/robby/games.json"));
//  myDebug() << u;

  markTime();
  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result, this, &MobyGamesFetcher::slotComplete);
}

void MobyGamesFetcher::stop() {
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

Tellico::Data::EntryPtr MobyGamesFetcher::fetchEntryHook(uint uid_) {
  if(!m_entries.contains(uid_)) {
    myDebug() << "no entry ptr";
    return Data::EntryPtr();
  }

  Data::EntryPtr entry = m_entries.value(uid_);

  QUrl u(QString::fromLatin1(MOBYGAMES_API_URL));
  u.setPath(u.path() + QStringLiteral("/games/%1/platforms/%2")
                       .arg(entry->field(QStringLiteral("moby-id")),
                            entry->field(QStringLiteral("platform-id"))));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("api_key"), m_apiKey);
  u.setQuery(q);
//  myDebug() << u;

  markTime();
  QPointer<KIO::StoredTransferJob> job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  if(!job->exec()) {
    myDebug() << job->errorString() << u;
    return entry;
  }
  QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data for" << u;
    return entry;
  }
#if 0
  myWarning() << "Remove platforms debug from mobygamesfetcher.cpp";
  QFile file(QStringLiteral("/tmp/moby-game-info.json"));
  if(file.open(QIODevice::WriteOnly)) {
    QTextStream t(&file);
    t << data;
  }
  file.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  QVariantMap map = doc.object().toVariantMap();
  foreach(const QVariant& rating, map.value(QStringLiteral("ratings")).toList()) {
    const QVariantMap ratingMap = rating.toMap();
    const QString ratingSystem = ratingMap.value(QStringLiteral("rating_system_name")).toString();
    if(ratingSystem == QStringLiteral("PEGI Rating")) {
      QString rating = ratingMap.value(QStringLiteral("rating_name")).toString();
      if(!rating.startsWith(QStringLiteral("PEGI"))) {
        rating.prepend(QStringLiteral("PEGI "));
      }
      entry->setField(QStringLiteral("pegi"), rating);
    } else if(ratingSystem == QStringLiteral("ESRB Rating")) {
      const int esrb = ratingMap.value(QStringLiteral("rating_id")).toInt();
      if(m_esrbHash.contains(esrb)) {
        entry->setField(QStringLiteral("certification"), m_esrbHash.value(esrb));
      }
    }
  }
  // just use the first release
  const QVariantList releaseList = map.value(QStringLiteral("releases")).toList();
  if(!releaseList.isEmpty()) {
    const QVariantMap releaseMap = releaseList.at(0).toMap();
    QStringList pubs, devs;
    foreach(const QVariant& company, releaseMap.value(QStringLiteral("companies")).toList()) {
      const QVariantMap companyMap = company.toMap();
      if(companyMap.value(QStringLiteral("role")) == QStringLiteral("Developed by")) {
        devs += companyMap.value(QStringLiteral("company_name")).toString();
      } else if(companyMap.value(QStringLiteral("role")) == QStringLiteral("Published by")) {
        pubs += companyMap.value(QStringLiteral("company_name")).toString();
      }
    }
//    myDebug() << pubs << devs;
    entry->setField(QStringLiteral("publisher"), pubs.join(FieldFormat::delimiterString()));
    entry->setField(QStringLiteral("developer"), devs.join(FieldFormat::delimiterString()));
  }

  if(m_imageSize == NoImage) {
    entry->setField(QStringLiteral("moby-id"), QString());
    entry->setField(QStringLiteral("platform-id"), QString());
    return entry;
  }

  // check for empty cover
  const QString image_id = entry->field(QStringLiteral("cover"));
  if(!image_id.isEmpty()) {
    return entry;
  }

  u = QUrl(QString::fromLatin1(MOBYGAMES_API_URL));
  u.setPath(u.path() + QStringLiteral("/games/%1/platforms/%2/covers")
                       .arg(entry->field(QStringLiteral("moby-id")),
                            entry->field(QStringLiteral("platform-id"))));
  u.setQuery(q);
//  myDebug() << u;

  markTime();
  job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  if(!job->exec()) {
    myDebug() << job->errorString() << u;
    return entry;
  }
  data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data for" << u;
    return entry;
  }
#if 0
  myWarning() << "Remove covers debug from mobygamesfetcher.cpp";
  QFile file2(QStringLiteral("/tmp/moby-covers.json"));
  if(file2.open(QIODevice::WriteOnly)) {
    QTextStream t(&file2);
    t << data;
  }
  file2.close();
#endif

  QString coverUrl;
  doc = QJsonDocument::fromJson(data);
  map = doc.object().toVariantMap();
  // prefer "Front Cover" but fall back to "Media"
  QString front, media;
  QVariantList coverGroupList = map.value(QStringLiteral("cover_groups")).toList();
  foreach(const QVariant& coverGroup, coverGroupList) {
    // just take the cover from the first group with front cover, appear to be grouped by country
    QVariantList coverList = coverGroup.toMap().value(QStringLiteral("covers")).toList();
    foreach(const QVariant& coverVariant, coverList) {
      const QVariantMap coverMap = coverVariant.toMap();
      if(media.isEmpty() &&
         coverMap.value(QStringLiteral("scan_of")) == QStringLiteral("Media")) {
        media = m_imageSize == SmallImage ?
                coverMap.value(QStringLiteral("thumbnail_image")).toString() :
                coverMap.value(QStringLiteral("image")).toString();
      } else if(coverMap.value(QStringLiteral("scan_of")) == QStringLiteral("Front Cover")) {
        front = m_imageSize == SmallImage ?
                coverMap.value(QStringLiteral("thumbnail_image")).toString() :
                coverMap.value(QStringLiteral("image")).toString();
        break;
      }
    }
    if(!front.isEmpty()) {
      // no need to continue iteration through cover groups
      break;
    }
  }

  coverUrl = front.isEmpty() ? media : front; // fall back to media image

  if(!coverUrl.isEmpty()) {
//    myDebug() << coverUrl;
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(coverUrl), true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QStringLiteral("cover"), id);
  }

  const QString screenshot = QStringLiteral("screenshot");
  if(optionalFields().contains(screenshot)) {
    if(!entry->collection()->hasField(screenshot)) {
      entry->collection()->addField(Data::Field::createDefaultField(Data::Field::ScreenshotField));
    }
    u = QUrl(QString::fromLatin1(MOBYGAMES_API_URL));
    u.setPath(u.path() + QStringLiteral("/games/%1/platforms/%2/screenshots")
                         .arg(entry->field(QStringLiteral("moby-id")),
                              entry->field(QStringLiteral("platform-id"))));
    u.setQuery(q);
    markTime();
    job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
    KJobWidgets::setWindow(job, GUI::Proxy::widget());
    if(!job->exec()) {
      myDebug() << job->errorString() << u;
      return entry;
    }
#if 0
    myWarning() << "Remove screenshots debug from mobygamesfetcher.cpp";
    QFile file3(QStringLiteral("/tmp/moby-screenshots.json"));
    if(file3.open(QIODevice::WriteOnly)) {
      QTextStream t(&file3);
      t << job->data();
    }
    file3.close();
#endif
    QString screenshotUrl;
    doc = QJsonDocument::fromJson(job->data());
    map = doc.object().toVariantMap();
    auto list = map.value(QStringLiteral("screenshots")).toList();
    if(!list.isEmpty()) {
      screenshotUrl = mapValue(list.at(0).toMap(), "image");
    }
    if(!screenshotUrl.isEmpty()) {
//      myDebug() << screenshotUrl;
      const QString id = ImageFactory::addImage(QUrl::fromUserInput(screenshotUrl), true /* quiet */);
      entry->setField(screenshot, id);
    }
  }

  // clear the placeholder fields
  entry->setField(QStringLiteral("moby-id"), QString());
  entry->setField(QStringLiteral("platform-id"), QString());
  return entry;
}

Tellico::Fetch::FetchRequest MobyGamesFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString title = entry_->field(QStringLiteral("title"));
  const QString platform = entry_->field(QStringLiteral("platform"));
  // if the platform name is not empty, we can use that to limit the title search
  if(!platform.isEmpty()) {
    // iterate through platform map to potentially match name
    // would ultimately be faster to have a second hash to map name to id or a bidirectional
    // could assume the platform name is already normalized, but allow user to have entered something
    // not quite the same
    if(m_platforms.isEmpty()) {
      updatePlatforms();
    }
    const int pId = m_platforms.key(Data::GameCollection::normalizePlatform(platform));
    if(pId > 0) {
      return FetchRequest(Raw, QString::fromLatin1("title=%1&platform=%2").arg(title, QString::number(pId)));
    }
  }

  // fallback to pure title search
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

void MobyGamesFetcher::slotComplete(KJob* job_) {
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
  myWarning() << "Remove debug from mobygamesfetcher.cpp";
  QFile file(QStringLiteral("/tmp/moby-results.json"));
  if(file.open(QIODevice::WriteOnly)) {
    QTextStream t(&file);
    t << data;
  }
  file.close();
#endif

  Data::CollPtr coll(new Data::GameCollection(true));
  if(optionalFields().contains(QStringLiteral("pegi"))) {
    coll->addField(Data::Field::createDefaultField(Data::Field::PegiField));
  }
  if(optionalFields().contains(QStringLiteral("mobygames"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("mobygames"), i18n("MobyGames Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  // placeholder for mobygames id, to be removed later
  Data::FieldPtr f1(new Data::Field(QStringLiteral("moby-id"), QString(), Data::Field::Number));
  coll->addField(f1);
  Data::FieldPtr f2(new Data::Field(QStringLiteral("platform-id"), QString(), Data::Field::Number));
  coll->addField(f2);

  QJsonDocument doc = QJsonDocument::fromJson(data);
  QVariantMap map = doc.object().toVariantMap();

  // check for error
  if(map.contains(QStringLiteral("error"))) {
    const QString msg = map.value(QStringLiteral("message")).toString();
    message(msg, MessageHandler::Error);
    myDebug() << "MobyGamesFetcher -" << msg;
    stop();
    return;
  }

  if(m_platforms.isEmpty()) {
    updatePlatforms();
  }

  foreach(const QVariant& result, map.value(QStringLiteral("games")).toList()) {
    QVariantMap resultMap = result.toMap();
    Data::EntryList entries = createEntries(coll, resultMap);
    foreach(const Data::EntryPtr& entry, entries) {
      FetchResult* r = new FetchResult(this, entry);
      m_entries.insert(r->uid, entry);
      emit signalResultFound(r);
    }
  }

  stop();
}

Tellico::Data::EntryList MobyGamesFetcher::createEntries(Data::CollPtr coll_, const QVariantMap& resultMap_) {
  Data::EntryPtr entry(new Data::Entry(coll_));
  entry->setField(QStringLiteral("title"), mapValue(resultMap_, "title"));
  entry->setField(QStringLiteral("description"), mapValue(resultMap_, "description"));
  entry->setField(QStringLiteral("moby-id"), mapValue(resultMap_, "game_id"));

  QStringList genres;
  foreach(const QVariant& genreMap, resultMap_.value(QStringLiteral("genres")).toList()) {
    const QString g = genreMap.toMap().value(QStringLiteral("genre_name")).toString();
    if(!g.isEmpty()) {
      genres << g;
    }
  }
  entry->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));

  if(optionalFields().contains(QStringLiteral("mobygames"))) {
    entry->setField(QStringLiteral("mobygames"), mapValue(resultMap_, "moby_url"));
  }

  const QString platformS(QStringLiteral("platform"));

  // for efficiency, check if the search includes a platform
  // since the results will include all the platforms, not just the searched one
  if(request().key() == Raw &&
     request().value().contains(platformS)) {
    QUrlQuery q(request().value());
    m_requestPlatformId = q.queryItemValue(platformS).toInt();
  }

  Data::EntryList entries;
  // return a new entry for every platform
  foreach(const QVariant& platformMapV, resultMap_.value(QStringLiteral("platforms")).toList()) {
    Data::EntryPtr newEntry(new Data::Entry(*entry));

    const QVariantMap platformMap = platformMapV.toMap();
    const int platformId = platformMap.value(QStringLiteral("platform_id")).toInt();
    if(m_platforms.contains(platformId)) {
      const QString platform = m_platforms[platformId];
      // make the assumption that if the platform name isn't already in the allowed list, it should be added
      Data::FieldPtr f = newEntry->collection()->fieldByName(platformS);
      if(f && !f->allowed().contains(platform)) {
        f->setAllowed(QStringList(f->allowed()) << platform);
      }
      newEntry->setField(platformS, platform);
    } else {
      myDebug() << "platform list does not contain" << platformId << mapValue(platformMap, "platform_name");
    }

    newEntry->setField(QStringLiteral("platform-id"),
                       platformMap.value(QStringLiteral("platform_id")).toString());
    newEntry->setField(QStringLiteral("year"),
                       platformMap.value(QStringLiteral("first_release_date")).toString().left(4));
    if(m_requestPlatformId == 0 || m_requestPlatformId == platformId) entries << newEntry;
  }
  return entries;
}

void MobyGamesFetcher::markTime() {
  // need to wait a bit after previous query, Moby error message say 1 sec
  if(m_idleTime.elapsed() < 1000) QThread::msleep(1000);
  m_idleTime.restart();
}

void MobyGamesFetcher::populateHashes() {
  // cheat by grabbing i18n values from default collection
  Data::CollPtr c(new Data::GameCollection(true));
  QStringList esrb = c->fieldByName(QStringLiteral("certification"))->allowed();
  Q_ASSERT(esrb.size() == 8);
  while(esrb.size() < 8) {
    esrb << QString();
  }
  // 89 == EC
  // 90 == Everyone
  // etc.
  // 95 == Pending
  // https://www.mobygames.com/attribute/sheet/attributeId,89
  m_esrbHash.insert(89, esrb.at(6));
  m_esrbHash.insert(90, esrb.at(5));
  m_esrbHash.insert(91, esrb.at(4));
  m_esrbHash.insert(92, esrb.at(3));
  m_esrbHash.insert(93, esrb.at(2));
  m_esrbHash.insert(94, esrb.at(1));
  m_esrbHash.insert(95, esrb.at(7));

  // Read the cached data for the platform list
  QFile file(Tellico::saveLocation(QStringLiteral("mobygames-data/")) + QLatin1String("platforms.json"));
  if(file.open(QIODevice::ReadOnly)) {
    m_platforms.clear();
    const QVariantMap topMap = QJsonDocument::fromJson(file.readAll()).object().toVariantMap();
    foreach(const QVariant& platform, topMap.value(QStringLiteral("platforms")).toList()) {
      const QVariantMap m = platform.toMap();
      Data::GameCollection::GamePlatform pId = Data::GameCollection::guessPlatform(mapValue(m, "platform_name"));
      if(pId == Data::GameCollection::UnknownPlatform) {
        // platform is not in the default list, just keep it as is
        m_platforms.insert(m.value(QStringLiteral("platform_id")).toInt(),
                           mapValue(m, "platform_name"));
      } else {
        m_platforms.insert(m.value(QStringLiteral("platform_id")).toInt(),
                           Data::GameCollection::platformName(pId));
      }
    }
  } else if(file.exists()) { // don't want errors for non-existing file
    myDebug() << "Failed to read from" << file.fileName() << file.errorString();
  }
}

void MobyGamesFetcher::updatePlatforms() {
  QUrl u(QString::fromLatin1(MOBYGAMES_API_URL));
  u.setPath(u.path() + QStringLiteral("/platforms"));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("api_key"), m_apiKey);
  u.setQuery(q);

//  u = QUrl::fromLocalFile(QStringLiteral("/home/robby/platforms.json")); // for testing
//  myDebug() << "Reading platforms from" << u;
  markTime();
  const QByteArray data = FileHandler::readDataFile(u, true);
  QFile file(Tellico::saveLocation(QStringLiteral("mobygames-data/")) + QLatin1String("platforms.json"));
  if(!file.open(QIODevice::WriteOnly) || file.write(data) == -1) {
    myDebug() << "unable to write to" << file.fileName() << file.errorString();
    return;
  }
  file.close();
  populateHashes();
}

Tellico::Fetch::ConfigWidget* MobyGamesFetcher::configWidget(QWidget* parent_) const {
  return new MobyGamesFetcher::ConfigWidget(parent_, this);
}

QString MobyGamesFetcher::defaultName() {
  return QStringLiteral("MobyGames");
}

QString MobyGamesFetcher::defaultIcon() {
  return favIcon("https://www.mobygames.com/static/img/favicon.ico");
}

Tellico::StringHash MobyGamesFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("pegi")] = i18n("PEGI Rating");
  hash[QStringLiteral("mobygames")] = i18n("MobyGames Link");
  hash[QStringLiteral("screenshot")] = i18n("Screenshot");
  return hash;
}

MobyGamesFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const MobyGamesFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* al = new QLabel(i18n("Registration is required for accessing this data source. "
                               "If you agree to the terms and conditions, <a href='%1'>sign "
                               "up for an account</a>, and enter your information below.",
                                QStringLiteral("https://www.mobygames.com/info/api")),
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
//  m_imageCombo->addItem(i18n("Medium Image"), MediumImage); // no medium right now, either thumbnail (small) or large
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

  // now add additional fields widget
  addFieldsWidget(MobyGamesFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
    m_imageCombo->setCurrentData(fetcher_->m_imageSize);
  } else { // defaults
    m_imageCombo->setCurrentData(SmallImage);
  }
}

void MobyGamesFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
  const int n = m_imageCombo->currentData().toInt();
  config_.writeEntry("Image Size", n);
}

QString MobyGamesFetcher::ConfigWidget::preferredName() const {
  return MobyGamesFetcher::defaultName();
}
