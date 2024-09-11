/***************************************************************************
    Copyright (C) 2017-2020 Robby Stephenson <robby@periapsis.org>
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

#include "igdbfetcher.h"
#include "../collections/gamecollection.h"
#include "../images/imagefactory.h"
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
#include <QUrlQuery>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QThread>
#include <QTimer>

namespace {
  static const int IGDB_MAX_RETURNS_TOTAL = 20;
  static const char* IGDB_API_URL = "https://api.igdb.com/v4";
  static const char* IGDB_CLIENT_ID = "hc7jojgdmkcc6divxmz0mxzzt22ehr";
  static const char* IGDB_TOKEN_URL = "https://api.tellico-project.org/igdb/";
}

using namespace Tellico;
using Tellico::Fetch::IGDBFetcher;

IGDBFetcher::IGDBFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false) {
  m_requestTimer.start();
  // delay reading the platform names from the cache file
  QTimer::singleShot(0, this, &IGDBFetcher::populateHashes);
}

IGDBFetcher::~IGDBFetcher() {
}

QString IGDBFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString IGDBFetcher::attribution() const {
  return TC_I18N3(providedBy, QLatin1String("https://igdb.com"), QLatin1String("IGDB.com"));
}

bool IGDBFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Keyword;
}

bool IGDBFetcher::canFetch(int type) const {
  return type == Data::Collection::Game;
}

void IGDBFetcher::readConfigHook(const KConfigGroup& config_) {
  const QString k = config_.readEntry("Access Token");
  if(!k.isEmpty()) {
    m_accessToken = k;
  }
  m_accessTokenExpires = config_.readEntry("Access Token Expires", QDateTime());
}

void IGDBFetcher::saveConfigHook(KConfigGroup& config_) {
  config_.writeEntry("Access Token", m_accessToken);
  config_.writeEntry("Access Token Expires", m_accessTokenExpires);
}

void IGDBFetcher::search() {
  continueSearch();
}

void IGDBFetcher::continueSearch() {
  m_started = true;

  QUrl u(QString::fromLatin1(IGDB_API_URL));
  u.setPath(u.path() + QStringLiteral("/games"));

  QStringList clauseList;
  switch(request().key()) {
    case Keyword:
      clauseList += QString(QStringLiteral("search \"%1\";")).arg(request().value());
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  clauseList += QStringLiteral("fields *,cover.url,screenshots.url,age_ratings.*,involved_companies.*;");
  // exclude some of the bigger unused fields
  clauseList += QStringLiteral("exclude keywords,tags;");
  clauseList += QString(QStringLiteral("limit %1;")).arg(QString::number(IGDB_MAX_RETURNS_TOTAL));
//  myDebug() << u << clauseList.join(QStringLiteral(" "));

  m_job = igdbJob(u, clauseList.join(QStringLiteral(" ")));
  connect(m_job.data(), &KJob::result, this, &IGDBFetcher::slotComplete);
  markTime();
}

void IGDBFetcher::stop() {
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

Tellico::Data::EntryPtr IGDBFetcher::fetchEntryHook(uint uid_) {
  if(!m_entries.contains(uid_)) {
    myDebug() << "no entry ptr";
    return Data::EntryPtr();
  }

  Data::EntryPtr entry = m_entries.value(uid_);

  // image might still be a URL
  const QString coverString = QStringLiteral("cover");
  const QString image_id = entry->field(coverString);
  if(image_id.contains(QLatin1Char('/'))) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(image_id), true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(coverString, id);
  }

  const QString screenshotString = QStringLiteral("screenshot");
  const QString screenshot_id = entry->field(screenshotString);
  if(screenshot_id.contains(QLatin1Char('/'))) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(screenshot_id), true /* quiet */);
    entry->setField(screenshotString, id);
  }

  return entry;
}

Tellico::Fetch::FetchRequest IGDBFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Keyword, title);
  }
  return FetchRequest();
}

void IGDBFetcher::slotComplete(KJob* job_) {
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
  myWarning() << "Remove debug from igdbfetcher.cpp";
  QFile file(QStringLiteral("/tmp/test.json"));
  if(file.open(QIODevice::WriteOnly)) {
    QTextStream t(&file);
    t << data;
  }
  file.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  if(doc.isObject()) {
    // probably an error message
    QJsonObject obj = doc.object();
    const QString msg = obj.value(QLatin1String("message")).toString();
    myDebug() << "IGDBFetcher -" << msg;
    message(msg, MessageHandler::Error);
    stop();
    return;
  }

  Data::CollPtr coll(new Data::GameCollection(true));

  foreach(const QVariant& result, doc.array().toVariantList()) {
    QVariantMap resultMap = result.toMap();
    Data::EntryPtr baseEntry(new Data::Entry(coll));
    populateEntry(baseEntry, resultMap);

    // for multiple platforms, return a result for each one
    QVariantList platforms = resultMap.value(QStringLiteral("platforms")).toList();
    foreach(const QVariant pVariant, platforms) {
      Data::EntryPtr entry(new Data::Entry(*baseEntry));
      const int pId = pVariant.toInt();
      if(!m_platformHash.contains(pId)) {
        readDataList(Platform);
      }
      const QString platform = Data::GameCollection::normalizePlatform(m_platformHash.value(pId));
      // make the assumption that if the platform name isn't already in the allowed list, it should be added
      Data::FieldPtr f = coll->fieldByName(QStringLiteral("platform"));
      if(f && !f->allowed().contains(platform)) {
        f->setAllowed(QStringList(f->allowed()) << platform);
      }
      entry->setField(QStringLiteral("platform"), platform);
      FetchResult* r = new FetchResult(this, entry);
      m_entries.insert(r->uid, entry);
      emit signalResultFound(r);
    }

    // also allow case of no platform
    if(platforms.isEmpty()) {
      FetchResult* r = new FetchResult(this, baseEntry);
      m_entries.insert(r->uid, baseEntry);
      emit signalResultFound(r);
    }
  }

  stop();
}

void IGDBFetcher::populateEntry(Data::EntryPtr entry_, const QVariantMap& resultMap_) {
  entry_->setField(QStringLiteral("title"), mapValue(resultMap_, "name"));
  entry_->setField(QStringLiteral("description"), mapValue(resultMap_, "summary"));

  QString cover = mapValue(resultMap_, "cover", "url");
  if(cover.startsWith(QLatin1Char('/'))) {
    cover.prepend(QStringLiteral("https:"));
  }
  entry_->setField(QStringLiteral("cover"), cover);

  const QString screenshotString = QStringLiteral("screenshot");
  if(optionalFields().contains(screenshotString)) {
    if(!entry_->collection()->hasField(screenshotString)) {
      entry_->collection()->addField(Data::Field::createDefaultField(Data::Field::ScreenshotField));
    }
    auto screenshotList = resultMap_.value(QStringLiteral("screenshots")).toList();
    if(!screenshotList.isEmpty()) {
      QString screenshot = mapValue(screenshotList.at(0).toMap(), "url");
      if(screenshot.startsWith(QLatin1Char('/'))) {
        screenshot.prepend(QStringLiteral("https:"));
      }
      entry_->setField(screenshotString, screenshot);
    }
  }

  QVariantList genreIDs = resultMap_.value(QStringLiteral("genres")).toList();
  QStringList genres;
  foreach(const QVariant& id, genreIDs) {
    const int genreId = id.toInt();
    if(!m_genreHash.contains(genreId)) {
      readDataList(Genre);
    }
    const QString genre = m_genreHash.value(genreId);
    if(!genre.isEmpty()) {
      genres << genre;
    }
  }
  entry_->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));

  qlonglong release_t = mapValue(resultMap_, "first_release_date").toLongLong();
  if(release_t > 0) {
    // could use QDateTime::fromSecsSinceEpoch but that was introduced in Qt 5.8
    // while I still support Qt 5.6, in theory...
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(release_t * 1000);
    entry_->setField(QStringLiteral("year"), QString::number(dt.date().year()));
  }

  const QString pegiString = QStringLiteral("pegi");
  const QVariantList ageRatingList = resultMap_.value(QStringLiteral("age_ratings")).toList();
  foreach(const QVariant& ageRating, ageRatingList) {
    const QVariantMap ratingMap = ageRating.toMap();
    // per Age Rating Enums, ESRB==1, PEGI==2
    const int category = ratingMap.value(QStringLiteral("category")).toInt();
    const int rating = ratingMap.value(QStringLiteral("rating")).toInt();
    if(category == 1) {
      if(m_esrbHash.contains(rating)) {
        entry_->setField(QStringLiteral("certification"), m_esrbHash.value(rating));
      } else {
        myDebug() << "No ESRB rating for value =" << rating;
      }
    } else if(category == 2 && optionalFields().contains(pegiString)) {
      if(!entry_->collection()->hasField(pegiString)) {
        entry_->collection()->addField(Data::Field::createDefaultField(Data::Field::PegiField));
      }
      entry_->setField(pegiString, m_pegiHash.value(rating));
    }
  }

  const QVariantList companyList = resultMap_.value(QStringLiteral("involved_companies")).toList();

  QList<int>companyIdList;
  foreach(const QVariant& company, companyList) {
    const QVariantMap companyMap = company.toMap();
    const int companyId = companyMap.value(QStringLiteral("company")).toInt();
    if(!m_companyHash.contains(companyId)) {
      companyIdList += companyId;
    }
  }
  if(!companyIdList.isEmpty()) {
    readDataList(Company, companyIdList);
  }

  QStringList pubs, devs;
  foreach(const QVariant& company, companyList) {
    const QVariantMap companyMap = company.toMap();
    const int companyId = companyMap.value(QStringLiteral("company")).toInt();
    const QString companyName = m_companyHash.value(companyId);
    if(companyName.isEmpty()) {
      continue;
    }
    if(companyMap.value(QStringLiteral("publisher")).toBool()) {
      pubs += companyName;
    } else if(companyMap.value(QStringLiteral("developer")).toBool()) {
      devs += companyName;
    }
  }
  entry_->setField(QStringLiteral("publisher"), pubs.join(FieldFormat::delimiterString()));
  entry_->setField(QStringLiteral("developer"), devs.join(FieldFormat::delimiterString()));

  const QString igdbString = QStringLiteral("igdb");
  if(optionalFields().contains(igdbString)) {
    if(!entry_->collection()->hasField(igdbString)) {
      Data::FieldPtr field(new Data::Field(igdbString, i18n("IGDB Link"), Data::Field::URL));
      field->setCategory(i18n("General"));
      entry_->collection()->addField(field);
    }
    entry_->setField(igdbString, mapValue(resultMap_, "url"));
  }
}

Tellico::Fetch::ConfigWidget* IGDBFetcher::configWidget(QWidget* parent_) const {
  return new IGDBFetcher::ConfigWidget(parent_, this);
}

// Use member hash for certain field names for now.
// Don't expect IGDB values to change. This avoids exponentially multiplying the number of API calls
void IGDBFetcher::populateHashes() {
  QFile genreFile(dataFileName(Genre));
  if(genreFile.open(QIODevice::ReadOnly)) {
    updateData(Genre, genreFile.readAll());
  } else if(genreFile.exists()) { // don't want errors for non-existing file
    myDebug() << "Failed to read genres from" << genreFile.fileName() << genreFile.errorString();
  }

  QFile platformFile(dataFileName(Platform));
  if(platformFile.open(QIODevice::ReadOnly)) {
    updateData(Platform, platformFile.readAll());
  } else if(platformFile.exists()) { // don't want errors for non-existing file
    myDebug() << "Failed to read from" << platformFile.fileName() << platformFile.errorString();
  }

  QFile companyFile(dataFileName(Company));
  if(companyFile.open(QIODevice::ReadOnly)) {
    updateData(Company, companyFile.readAll());
  } else if(companyFile.exists()) { // don't want errors for non-existing file
    myDebug() << "Failed to read from" << companyFile.fileName() << companyFile.errorString();
  }

  // grab i18n values for ESRB from default collection
  Data::CollPtr c(new Data::GameCollection(true));
  QStringList esrb = c->fieldByName(QStringLiteral("certification"))->allowed();
  if(esrb.size() < 8) {
    myWarning() << "ESRB rating list is badly translated";
  }
  while(esrb.size() < 8) {
    esrb << QString();
  }
  // see https://api-docs.igdb.com/#age-rating
  m_esrbHash.insert(12, esrb.at(1)); // adults only
  m_esrbHash.insert(11, esrb.at(2)); // mature
  m_esrbHash.insert(10, esrb.at(3)); // teen
  m_esrbHash.insert(9,  esrb.at(4)); // e10
  m_esrbHash.insert(8,  esrb.at(5)); // everyone
  m_esrbHash.insert(7,  esrb.at(6)); // early childhood
  m_esrbHash.insert(6,  esrb.at(7)); // pending

  m_pegiHash.insert(1, QStringLiteral("PEGI 3"));
  m_pegiHash.insert(2, QStringLiteral("PEGI 7"));
  m_pegiHash.insert(3, QStringLiteral("PEGI 12"));
  m_pegiHash.insert(4, QStringLiteral("PEGI 16"));
  m_pegiHash.insert(5, QStringLiteral("PEGI 18"));
}

void IGDBFetcher::updateData(IgdbDataType dataType_, const QByteArray& jsonData_) {
  const QString idString(QStringLiteral("id"));
  const QString nmString(QStringLiteral("name"));
  QHash<int, QString> dataHash;
  const QJsonArray array = QJsonDocument::fromJson(jsonData_).array();
  for(int i = 0; i < array.size(); ++i) {
    QJsonObject obj = array.at(i).toObject();
    dataHash.insert(obj.value(idString).toInt(),
                    obj.value(nmString).toString());
  }

  // transfer read data into the correct local variable
  switch(dataType_) {
    case Genre:
      m_genreHash = dataHash;
      break;
    case Platform:
      m_platformHash = dataHash;
      break;
    case Company:
      // company list is bigger than request size, so rather than downloading all names
      // have to do it in chunks and then merge
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
      m_companyHash.unite(dataHash);
#else
      m_companyHash.insert(dataHash);
#endif
      break;
  }
}

void IGDBFetcher::readDataList(IgdbDataType dataType_, const QList<int>& idList_) {
  QUrl u(QString::fromLatin1(IGDB_API_URL));
  switch(dataType_) {
    case Genre:
      u.setPath(u.path() + QStringLiteral("/genres"));
      break;
    case Platform:
      u.setPath(u.path() + QStringLiteral("/platforms"));
      break;
    case Company:
      u.setPath(u.path() + QStringLiteral("/companies"));
      break;
  }

  QStringList clauseList;
  clauseList += QStringLiteral("fields id,name;");
  // if the id list is not empty, seach for specific data id values
  if(!idList_.isEmpty()) {
    // where id = (8,9,11);
    QString clause = QStringLiteral("where id = (") + QString::number(idList_.at(0));
    for(int i = 1; i < idList_.size(); ++i) {
      clause += (QLatin1String(",") + QString::number(idList_.at(i)));
    }
    clause += QLatin1String(");");
    clauseList += clause;
  }
  clauseList += QStringLiteral("limit 500;"); // biggest limit is 500 which should be enough for all

  QPointer<KIO::StoredTransferJob> job = igdbJob(u, clauseList.join(QStringLiteral(" ")));
  markTime();
  if(!job->exec()) {
    myDebug() << "IGDB: data request failed";
    myDebug() << job->errorString() << u;
    return;
  }

  const QByteArray data = job->data();
  updateData(dataType_, data);

  // now save the date, but instead of just writing the job->data() to the file
  // since the company data may have been merged, write the full set of hash values
  QByteArray dataToWrite;
  if(dataType_ == Company) {
    QJsonArray array;
    const QString idString(QStringLiteral("id"));
    const QString nmString(QStringLiteral("name"));
    QHashIterator<int, QString> it(m_companyHash);
    while(it.hasNext()) {
      it.next();
      QJsonObject obj;
      obj.insert(idString, it.key());
      obj.insert(nmString, it.value());
      array.append(obj);
    }
    dataToWrite = QJsonDocument(array).toJson();
  } else {
    dataToWrite = data;
  }

  QFile file(dataFileName(dataType_));
  if(!file.open(QIODevice::WriteOnly) || file.write(dataToWrite) == -1) {
    myDebug() << "unable to write to" << file.fileName() << file.errorString();
    return;
  }
  file.close();
}

void IGDBFetcher::markTime() const {
  // rate limit is 4 requests per second
  if(m_requestTimer.elapsed() < 250) QThread::msleep(250);
  m_requestTimer.restart();
}

void IGDBFetcher::checkAccessToken() {
  const QDateTime now = QDateTime::currentDateTimeUtc();
  if(!m_accessToken.isEmpty() && m_accessTokenExpires > now) {
    // access token should be fine, nothing to do
    return;
  }

  QUrl u(QString::fromLatin1(IGDB_TOKEN_URL));
//  myDebug() << "Downloading IGDN token from" << u.toString();
  QPointer<KIO::StoredTransferJob> job = KIO::storedHttpPost(QByteArray(), u, KIO::HideProgressInfo);
  job->addMetaData(QStringLiteral("accept"), QStringLiteral("application/json"));
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  if(!job->exec()) {
    myDebug() << "IGDB: access token request failed";
    myDebug() << job->errorString() << u;
    return;
  }

  QJsonDocument doc = QJsonDocument::fromJson(job->data());
  if(doc.isNull()) {
    myDebug() << "IGDB: Invalid JSON";
    return;
  }
  QJsonObject response = doc.object();
  if(response.contains(QLatin1String("message"))) {
    myDebug() << "IGDB:" << response.value(QLatin1String("message")).toString();
  }
  m_accessToken = response.value(QLatin1String("access_token")).toString();
  const int expires = response.value(QLatin1String("expires_in")).toInt();
  if(expires > 0) {
    m_accessTokenExpires = now.addSecs(expires);
  }
//  myDebug() << "Received access token" << m_accessToken << m_accessTokenExpires;
}

QString IGDBFetcher::defaultName() {
  return i18n("Internet Game Database (IGDB.com)");
}

QString IGDBFetcher::defaultIcon() {
  // IGDB blocks favicon requests without a referer seemingly
  return favIcon(QUrl(QLatin1String("https://www.igdb.com")),
                 QUrl(QLatin1String("https://tellico-project.org/img/igdb-favicon.ico")));
  }

Tellico::StringHash IGDBFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("pegi")] = i18n("PEGI Rating");
  hash[QStringLiteral("igdb")] = i18n("IGDB Link");
  hash[QStringLiteral("screenshot")] = i18n("Screenshot");
  return hash;
}

QString IGDBFetcher::dataFileName(IgdbDataType dataType_) {
  const QString dataDir = Tellico::saveLocation(QStringLiteral("igdb-data/"));
  QString fileName;
  switch(dataType_) {
    case Genre:
      fileName = dataDir + QLatin1String("genres.json");
      break;
    case Platform:
      fileName = dataDir + QLatin1String("platforms.json");
      break;
    case Company:
      fileName = dataDir + QLatin1String("companies.json");
      break;
  }
  return fileName;
}

QPointer<KIO::StoredTransferJob> IGDBFetcher::igdbJob(const QUrl& url_, const QString& query_) {
  checkAccessToken();
  QPointer<KIO::StoredTransferJob> job = KIO::storedHttpPost(query_.toUtf8(), url_, KIO::HideProgressInfo);
  QStringList customHeaders;
  customHeaders += (QStringLiteral("Client-ID: ") + QString::fromLatin1(IGDB_CLIENT_ID));
  customHeaders += (QStringLiteral("Authorization: ") + QLatin1String("Bearer ") + m_accessToken);
  job->addMetaData(QStringLiteral("customHTTPHeader"), customHeaders.join(QLatin1String("\r\n")));
  job->addMetaData(QStringLiteral("accept"), QStringLiteral("application/json"));
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  return job;
}

IGDBFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const IGDBFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(IGDBFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void IGDBFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString IGDBFetcher::ConfigWidget::preferredName() const {
  return IGDBFetcher::defaultName();
}
