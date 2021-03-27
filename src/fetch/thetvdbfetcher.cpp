/***************************************************************************
    Copyright (C) 2021 Robby Stephenson <robby@periapsis.org>
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

#include "thetvdbfetcher.h"
#include "../collections/videocollection.h"
#include "../images/imagefactory.h"
#include "../gui/combobox.h"
#include "../core/filehandler.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../core/tellico_strings.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KJob>
#include <KJobUiDelegate>
#include <KJobWidgets/KJobWidgets>
#include <KIO/StoredTransferJob>
#include <kwidgetsaddons_version.h>
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5,55,0)
#include <KLanguageName>
#endif

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QTextCodec>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QLineEdit>

#define THETVDB_LOG 0

namespace {
  static const int THETVDB_MAX_RETURNS_TOTAL = 20;
  static const char* THETVDB_API_URL = "https://api.thetvdb.com";
  static const char* THETVDB_API_KEY = "6656bb823a0dd79390a4efd91725dfee00373b0d4f7ed4e36d549aae59183405";
  static const int THETVDB_TOKEN_EXPIRES = 24*60*60; // expires in 24 hours
  static const char* THETVDB_ART_PREFIX = "https://thetvdb.com/banners/";
}

using namespace Tellico;
using Tellico::Fetch::TheTVDBFetcher;

TheTVDBFetcher::TheTVDBFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false) {
  m_apiKey = Tellico::reverseObfuscate(THETVDB_API_KEY);
}

TheTVDBFetcher::~TheTVDBFetcher() {
}

QString TheTVDBFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString TheTVDBFetcher::attribution() const {
  return i18n(providedBy).arg(QLatin1String("https://thetvdb.com"), defaultName());
}

bool TheTVDBFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title;
}

bool TheTVDBFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

void TheTVDBFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key");
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
  k = config_.readEntry("Access Token");
  if(!k.isEmpty()) {
    m_accessToken = k;
  }
  if(!m_accessToken.isEmpty()) {
    m_accessTokenExpires = config_.readEntry("Access Token Expires", QDateTime());
  }
}

void TheTVDBFetcher::saveConfigHook(KConfigGroup& config_) {
  config_.writeEntry("Access Token", m_accessToken);
  config_.writeEntry("Access Token Expires", m_accessTokenExpires);
}

void TheTVDBFetcher::search() {
  continueSearch();
}

void TheTVDBFetcher::continueSearch() {
  m_started = true;

  QUrl u(QString::fromLatin1(THETVDB_API_URL));
  switch(request().key()) {
    case Title:
      u = u.adjusted(QUrl::StripTrailingSlash);
      u.setPath(u.path() + QLatin1String("/search/series"));
      {
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("name"), request().value());
        u.setQuery(q);
      }
      break;

    default:
      myWarning() << "key not recognized:" << request().key();
      stop();
      return;
  }
//  myDebug() << u;

  m_job = getJob(u);
  connect(m_job.data(), &KJob::result, this, &TheTVDBFetcher::slotComplete);
}

void TheTVDBFetcher::stop() {
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

Tellico::Fetch::FetchRequest TheTVDBFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

void TheTVDBFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  const QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "TheTVDB: no data";
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = nullptr;

#if THETVDB_LOG
  myWarning() << "Remove debug from tvtvdbfetcher..cpp";
  QFile f(QStringLiteral("/tmp/test-thetvdb.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  Data::CollPtr coll(new Data::VideoCollection(true));
  // always add the thetvdb-id for fetchEntryHook
  Data::FieldPtr field(new Data::Field(QStringLiteral("thetvdb-id"), QString(), Data::Field::Line));
  field->setCategory(i18n("General"));
  coll->addField(field);

  if(optionalFields().contains(QStringLiteral("network"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("network"), i18n("Network"), Data::Field::Line));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  if(optionalFields().contains(QStringLiteral("imdb"))) {
    coll->addField(Data::Field::createDefaultField(Data::Field::ImdbField));
  }
  if(optionalFields().contains(QStringLiteral("episode"))) {
    coll->addField(Data::Field::createDefaultField(Data::Field::EpisodeField));
  }

  QJsonDocument doc = QJsonDocument::fromJson(data);
  const QJsonArray results = doc.object().value(QLatin1String("data")).toArray();

  if(results.isEmpty()) {
    myDebug() << "no results";
    stop();
    return;
  }

  int count = 0;
  foreach(const QJsonValue& result, results) {
    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toObject().toVariantMap(), false);

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
    ++count;
    if(count >= THETVDB_MAX_RETURNS_TOTAL) {
      break;
    }
  }

  stop();
}

Tellico::Data::EntryPtr TheTVDBFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  const QString id = entry->field(QStringLiteral("thetvdb-id"));
  if(!id.isEmpty()) {
    QUrl url(QString::fromLatin1(THETVDB_API_URL));
    url.setPath(QStringLiteral("/series/%1").arg(id));
//    myDebug() << url;
    auto job = getJob(url);
    if(!job->exec()) {
      myDebug() << job->errorString() << url;
      return Data::EntryPtr();
    }
    QByteArray data = job->data();
    if(data.isEmpty()) {
      myDebug() << "no data for" << url;
      return Data::EntryPtr();
    }
#if THETVDB_LOG
    myWarning() << "Remove debug2 from thetvdbfetcher.cpp";
    QFile f(QStringLiteral("/tmp/test2-thetvdb.json"));
    if(f.open(QIODevice::WriteOnly)) {
      QTextStream t(&f);
      t.setCodec("UTF-8");
      t << data;
    }
    f.close();
#endif
    QJsonDocument doc = QJsonDocument::fromJson(data);
    populateEntry(entry, doc.object().value(QLatin1String("data")).toObject().toVariantMap(), true);

    // now grab cast info
    url.setPath(QStringLiteral("/series/%1/actors").arg(id));
    job = getJob(url);
    if(job->exec()) {
      data = job->data();
      if(!data.isEmpty()) {
#if THETVDB_LOG
        myWarning() << "Remove debug3 from thetvdbfetcher.cpp";
        QFile f(QStringLiteral("/tmp/test3-thetvdb.json"));
        if(f.open(QIODevice::WriteOnly)) {
          QTextStream t(&f);
          t.setCodec("UTF-8");
          t << data;
        }
        f.close();
#endif
        doc = QJsonDocument::fromJson(data);
        populateCast(entry, doc.object().value(QLatin1String("data")).toArray());
      }
    } else {
      myDebug() << job->errorString() << url;
    }

    // now episode info
    if(optionalFields().contains(QStringLiteral("episode"))) {
      url.setPath(QStringLiteral("/series/%1/episodes").arg(id));
      job = getJob(url);
      if(job->exec()) {
        data = job->data();
        if(!data.isEmpty()) {
#if THETVDB_LOG
          myWarning() << "Remove debug4 from thetvdbfetcher.cpp";
          QFile f(QStringLiteral("/tmp/test4-thetvdb.json"));
          if(f.open(QIODevice::WriteOnly)) {
            QTextStream t(&f);
            t.setCodec("UTF-8");
            t << data;
          }
          f.close();
#endif
          doc = QJsonDocument::fromJson(data);
          populateEpisodes(entry, doc.object().value(QLatin1String("data")).toArray());
        }
      }
    }
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

  // don't want to include ID field - absence indicates entry is fully populated
  entry->setField(QStringLiteral("thetvdb-id"), QString());

  return entry;
}

void TheTVDBFetcher::populateEntry(Data::EntryPtr entry_, const QVariantMap& resultMap_, bool fullData_) {
  entry_->setField(QStringLiteral("thetvdb-id"), mapValue(resultMap_, "id"));
  entry_->setField(QStringLiteral("title"), mapValue(resultMap_, "seriesName"));
  entry_->setField(QStringLiteral("year"),  mapValue(resultMap_, "firstAired").left(4));

  // if we only need cursory data, then we're done
  if(!fullData_) {
    return;
  }

  const QString network(QStringLiteral("network"));
  if(entry_->collection()->hasField(network)) {
    entry_->setField(network, mapValue(resultMap_, "network"));
  }

  const QString imdb(QStringLiteral("imdb"));
  if(entry_->collection()->hasField(imdb)) {
    entry_->setField(imdb, QLatin1String("https://www.imdb.com/title/") + mapValue(resultMap_, "imdbId"));
  }

  QStringList genres;
  foreach(const QVariant& genre, resultMap_.value(QLatin1String("genre")).toList()) {
    genres << genre.toString();
  }
  entry_->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));

  const QString cert = QStringLiteral("certification");
  const QString rating = mapValue(resultMap_, "rating");
  QStringList allowed = entry_->collection()->fieldByName(cert)->allowed();
  if(!rating.isEmpty() && !allowed.contains(rating)) {
    allowed << rating;
    entry_->collection()->fieldByName(cert)->setAllowed(allowed);
    entry_->setField(cert, rating);
  }

  entry_->setField(QStringLiteral("cover"), QLatin1String(THETVDB_ART_PREFIX) + mapValue(resultMap_, "poster"));

  QString lang = mapValue(resultMap_, "language");
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5,55,0)
  const QString langName = KLanguageName::nameForCode(lang);
  if(!langName.isEmpty()) lang = langName;
  if(lang == QLatin1String("US English")) lang = QLatin1String("English");
#else
  if(lang == QLatin1String("en")) lang = QStringLiteral("English");
#endif
  entry_->setField(QStringLiteral("language"), lang);
  entry_->setField(QStringLiteral("plot"), mapValue(resultMap_, "overview"));
}

void TheTVDBFetcher::populateCast(Data::EntryPtr entry_, const QJsonArray& castArray_) {
  QStringList actors;
  foreach(const QJsonValue& cast, castArray_) {
    const QVariantMap castMap = cast.toObject().toVariantMap();
    actors << mapValue(castMap, "name") + FieldFormat::columnDelimiterString() + mapValue(castMap, "role");
  }
  entry_->setField(QStringLiteral("cast"), actors.join(FieldFormat::rowDelimiterString()));
}

void TheTVDBFetcher::populateEpisodes(Data::EntryPtr entry_, const QJsonArray& episodeArray_) {
  QStringList episodes, directors, writers;
  foreach(const QJsonValue& episode, episodeArray_) {
    const QVariantMap map = episode.toObject().toVariantMap();
    episodes << mapValue(map, "episodeName") + FieldFormat::columnDelimiterString() +
                mapValue(map, "airedSeason") + FieldFormat::columnDelimiterString() +
                mapValue(map, "airedEpisodeNumber");

    foreach(const auto& director, map.value(QStringLiteral("directors")).toList()) {
      directors << director.toString();
    }
    foreach(const auto& writer, map.value(QStringLiteral("writers")).toList()) {
      writers << writer.toString();
    }
  }
  directors.removeDuplicates();
  writers.removeDuplicates();
  entry_->setField(QStringLiteral("episode"), episodes.join(FieldFormat::rowDelimiterString()));
  entry_->setField(QStringLiteral("director"), directors.join(FieldFormat::delimiterString()));
  entry_->setField(QStringLiteral("writer"), writers.join(FieldFormat::delimiterString()));
}

void TheTVDBFetcher::checkAccessToken() {
  const QDateTime now = QDateTime::currentDateTimeUtc();
  if(m_accessToken.isEmpty() || m_accessTokenExpires < now) {
    requestToken();
  } else if(now.secsTo(m_accessTokenExpires) < 12*60*60) {
    // refresh the token if it expires within 12 hours
    refreshToken();
  }
}

void TheTVDBFetcher::requestToken() {
  QUrl u(QString::fromLatin1(THETVDB_API_URL));
  u.setPath(u.path() + QLatin1String("/login"));
  QJsonObject obj;
  obj.insert(QLatin1String("apikey"), m_apiKey);
  const QByteArray loginPayload = QJsonDocument(obj).toJson();

  QPointer<KIO::StoredTransferJob> job = KIO::storedHttpPost(loginPayload, u, KIO::HideProgressInfo);
  job->addMetaData(QStringLiteral("content-type"), QStringLiteral("Content-Type: application/json"));
  job->addMetaData(QStringLiteral("accept"), QStringLiteral("application/json"));
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  if(!job->exec()) {
    myDebug() << "TheTVDB: access token request failed";
    myDebug() << job->errorString() << u;
    return;
  }

  myDebug() << job->data();
  QJsonDocument doc = QJsonDocument::fromJson(job->data());
  if(doc.isNull()) {
    myDebug() << "TheTVDB: Invalid JSON in login response";
    return;
  }
  QJsonObject response = doc.object();
  if(response.contains(QLatin1String("Error"))) {
    myDebug() << "TheTVDB:" << response.value(QLatin1String("Error")).toString();
  }
  m_accessToken = response.value(QLatin1String("token")).toString();
  if(!m_accessToken.isEmpty()) {
    m_accessTokenExpires = QDateTime::currentDateTimeUtc().addSecs(THETVDB_TOKEN_EXPIRES);
  }
}

void TheTVDBFetcher::refreshToken() {
  Q_ASSERT(!m_accessToken.isEmpty());
  QUrl refreshUrl(QString::fromLatin1(THETVDB_API_URL));
  refreshUrl.setPath(refreshUrl.path() + QLatin1String("/refresh_token"));
  auto job = getJob(refreshUrl, false /* check token */);
  if(!job->exec()) {
    myDebug() << "TheTVDB: access token refresh failed";
    myDebug() << job->errorString() << refreshUrl;
    return;
  }

  QJsonDocument doc = QJsonDocument::fromJson(job->data());
  if(doc.isNull()) {
    myDebug() << "TheTVDB: Invalid JSON in refresh_token response";
    return;
  }
  QJsonObject response = doc.object();
  if(response.contains(QLatin1String("Error"))) {
    myDebug() << "TheTVDB:" << response.value(QLatin1String("Error")).toString();
  }
  m_accessToken = response.value(QLatin1String("token")).toString();
  if(!m_accessToken.isEmpty()) {
    m_accessTokenExpires = QDateTime::currentDateTimeUtc().addSecs(THETVDB_TOKEN_EXPIRES);
  }
}

QPointer<KIO::StoredTransferJob> TheTVDBFetcher::getJob(const QUrl& url_, bool checkToken_) {
  if(checkToken_) checkAccessToken();
  QPointer<KIO::StoredTransferJob> job = KIO::storedGet(url_, KIO::NoReload, KIO::HideProgressInfo);
  job->addMetaData(QStringLiteral("accept"), QStringLiteral("application/json"));
  job->addMetaData(QStringLiteral("customHTTPHeader"), QStringLiteral("Authorization: Bearer ") + m_accessToken);
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  return job;
}

Tellico::Fetch::ConfigWidget* TheTVDBFetcher::configWidget(QWidget* parent_) const {
  return new TheTVDBFetcher::ConfigWidget(parent_, this);
}

QString TheTVDBFetcher::defaultName() {
  return QStringLiteral("The TVDB");
}

QString TheTVDBFetcher::defaultIcon() {
  return favIcon("https://thetvdb.com/images/icon.png");
}

Tellico::StringHash TheTVDBFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("imdb")] = i18n("IMDb Link");
  hash[QStringLiteral("episode")] = i18n("Episodes");
  hash[QStringLiteral("network")] = i18n("Network");
  return hash;
}

TheTVDBFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const TheTVDBFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* al = new QLabel(i18n("Registration is required for accessing this data source. "
                               "If you agree to the terms and conditions, <a href='%1'>sign "
                               "up for an account</a>, and enter your information below.",
                                QLatin1String("https://thetvdb.com/api-information")),
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
  QString w = i18n("The default Tellico key may be used, but searching may fail due to reaching access limits.");
  label->setWhatsThis(w);
  m_apiKeyEdit->setWhatsThis(w);
  label->setBuddy(m_apiKeyEdit);

  l->setRowStretch(++row, 10);

  // now add additional fields widget
  addFieldsWidget(TheTVDBFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    // only show the key if it is not the default Tellico one...
    // that way the user is prompted to apply for their own
    if(fetcher_->m_apiKey != Tellico::reverseObfuscate(THETVDB_API_KEY)) {
      m_apiKeyEdit->setText(fetcher_->m_apiKey);
    }
  }
}

QString TheTVDBFetcher::ConfigWidget::preferredName() const {
  return TheTVDBFetcher::defaultName();
}

void TheTVDBFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
}
