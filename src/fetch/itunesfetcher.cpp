/***************************************************************************
    Copyright (C) Robby Stephenson <robby@periapsis.org>
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

#include "itunesfetcher.h"
#include "../collections/musiccollection.h"
#include "../collections/videocollection.h"
#include "../images/imagefactory.h"
#include "../gui/combobox.h"
#include "../core/filehandler.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KJob>
#include <KJobUiDelegate>
#include <KJobWidgets/KJobWidgets>
#include <KIO/StoredTransferJob>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QTextCodec>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

namespace {
  static const int ITUNES_MAX_RETURNS_TOTAL = 20;
  static const char* ITUNES_API_URL = "https://itunes.apple.com";
}

using namespace Tellico;
using Tellico::Fetch::ItunesFetcher;

ItunesFetcher::ItunesFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false) {
}

ItunesFetcher::~ItunesFetcher() {
}

QString ItunesFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool ItunesFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Keyword || k == UPC;
}

bool ItunesFetcher::canFetch(int type) const {
  return type == Data::Collection::Album ||
         type == Data::Collection::Video;
}

void ItunesFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_)
}

void ItunesFetcher::saveConfigHook(KConfigGroup& config_) {
  Q_UNUSED(config_)
}

void ItunesFetcher::search() {
  m_started = true;

  QUrl u(QString::fromLatin1(ITUNES_API_URL));
  u = u.adjusted(QUrl::StripTrailingSlash);
  QUrlQuery q;
  switch(request().key()) {
    case Keyword:
      u.setPath(u.path() + QLatin1String("/search"));
      if(collectionType() == Data::Collection::Album) {
        q.addQueryItem(QStringLiteral("media"), QLatin1String("music"));
        q.addQueryItem(QStringLiteral("entity"), QLatin1String("album"));
      } else if(collectionType() == Data::Collection::Video) {
        q.addQueryItem(QStringLiteral("media"), QLatin1String("movie"));
        q.addQueryItem(QStringLiteral("entity"), QLatin1String("movie"));
      }
      q.addQueryItem(QStringLiteral("limit"), QString::number(ITUNES_MAX_RETURNS_TOTAL));
      q.addQueryItem(QStringLiteral("term"), request().value());
      break;

    case UPC:
      u.setPath(u.path() + QLatin1String("/lookup"));
      q.addQueryItem(QStringLiteral("entity"), QLatin1String("song"));
      q.addQueryItem(QStringLiteral("upc"), request().value());
      break;

    default:
      myWarning() << "key not recognized:" << request().key();
      stop();
      return;
  }
  u.setQuery(q);

//  myDebug() << u;
  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result, this, &ItunesFetcher::slotComplete);
}

void ItunesFetcher::stop() {
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

Tellico::Fetch::FetchRequest ItunesFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString upc = entry_->field(QStringLiteral("upc"));
  if(!upc.isEmpty()) {
    return FetchRequest(UPC, upc);
  }

  const QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Keyword, title);
  }

  return FetchRequest();
}

void ItunesFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  const QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "iTunesFetcher: no data";
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = nullptr;

#if 0
  myWarning() << "Remove debug from ItunesFetcher.cpp";
  QFile f(QStringLiteral("/tmp/test-itunes.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  if(doc.isNull()) {
    myDebug() << "null JSON document";
    stop();
    return;
  }

  QJsonArray results = doc.object().value(QLatin1String("results")).toArray();
  if(results.isEmpty()) {
    myDebug() << "iTunesFetcher: no results";
    stop();
    return;
  }

  Data::CollPtr coll;
  if(collectionType() == Data::Collection::Album) {
    coll = new Data::MusicCollection(true);
  } else if(collectionType() == Data::Collection::Video) {
    coll = new Data::VideoCollection(true);
  }

  // placeholder for collection id, to be removed later
  Data::FieldPtr f1(new Data::Field(QStringLiteral("collectionId"), QString(), Data::Field::Number));
  coll->addField(f1);
  if(optionalFields().contains(QStringLiteral("itunes"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("itunes"), i18n("iTunes Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }

  QList<FetchResult*> fetchResults;
  foreach(const QJsonValue& result, results) {
    // result kind must be an album
    auto obj = result.toObject();
    if(obj.value(QLatin1String("kind")) == QLatin1String("song")) {
      readTrackInfo(obj.toVariantMap());
    } else {
      Data::EntryPtr entry(new Data::Entry(coll));
      populateEntry(entry, obj.toVariantMap());

      FetchResult* r = new FetchResult(this, entry);
      m_entries.insert(r->uid, entry);
      fetchResults.append(r);
    }
  }

  // check for tracks to add by iterating over all known track lists
  QHashIterator<int, QStringList> i(m_trackList);
  while(i.hasNext()) {
    i.next();
    Data::EntryPtr entry = m_collectionHash.value(i.key());
    if(!entry) continue;
    entry->setField(QStringLiteral("track"), i.value().join(FieldFormat::rowDelimiterString()));
  }

  // don't emit result until after adding tracks
  for(auto fetchResult : fetchResults) {
    emit signalResultFound(fetchResult);
  }
  stop();
}

Tellico::Data::EntryPtr ItunesFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  // check if tracks need to be downloaded
  const QString collectionId = entry->field(QLatin1String("collectionId"));
  if(!collectionId.isEmpty() && collectionType() == Data::Collection::Album &&
     entry->field(QLatin1String("track")).isEmpty()) {
    QUrl u(QString::fromLatin1(ITUNES_API_URL));
    u = u.adjusted(QUrl::StripTrailingSlash);
    u.setPath(u.path() + QLatin1String("/lookup"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("entity"), QLatin1String("song"));
    q.addQueryItem(QStringLiteral("id"), collectionId);
    u.setQuery(q);
    auto job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
    if(job->exec()) {
      QJsonDocument doc = QJsonDocument::fromJson(job->data());
      QJsonArray results = doc.object().value(QLatin1String("results")).toArray();
      foreach(const QJsonValue& result, results) {
        auto obj = result.toObject();
        if(obj.value(QLatin1String("wrapperType")) == QLatin1String("track")) {
          readTrackInfo(obj.toVariantMap());
        }
      }
      entry->setField(QStringLiteral("track"),
                      m_trackList.value(collectionId.toInt()).join(FieldFormat::rowDelimiterString()));
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

  // clear the placeholder field
  entry->setField(QStringLiteral("collectionId"), QString());

  return entry;
}

void ItunesFetcher::populateEntry(Data::EntryPtr entry_, const QVariantMap& resultMap_) {
  entry_->setField(QStringLiteral("collectionId"), mapValue(resultMap_, "collectionId"));
  if(collectionType() == Data::Collection::Album) {
    entry_->setField(QStringLiteral("title"), mapValue(resultMap_, "collectionName"));
    entry_->setField(QStringLiteral("artist"), mapValue(resultMap_, "artistName"));
  } else if(collectionType() == Data::Collection::Video) {
    entry_->setField(QStringLiteral("title"), mapValue(resultMap_, "trackName"));
    entry_->setField(QStringLiteral("director"), mapValue(resultMap_, "artistName"));
    entry_->setField(QStringLiteral("nationality"), mapValue(resultMap_, "country"));
    QString cert = mapValue(resultMap_, "contentAdvisoryRating");
    if(cert == QStringLiteral("NR")) {
      cert = QLatin1Char('U');
    }
    if(mapValue(resultMap_, "country") == QLatin1String("US")) {
      cert += QStringLiteral(" (USA)");
    } else {
      cert += QLatin1String(" (") + mapValue(resultMap_, "country") + QLatin1Char(')');
    }
    QStringList certsAllowed = entry_->collection()->fieldByName(QStringLiteral("certification"))->allowed();
    if(!certsAllowed.contains(cert)) {
      certsAllowed << cert;
      Data::FieldPtr f = entry_->collection()->fieldByName(QStringLiteral("certification"));
      f->setAllowed(certsAllowed);
    }
    entry_->setField(QStringLiteral("certification"), cert);
    entry_->setField(QStringLiteral("plot"), mapValue(resultMap_, "longDescription"));
  }
  entry_->setField(QStringLiteral("year"),  mapValue(resultMap_, "releaseDate").left(4));
  entry_->setField(QStringLiteral("genre"), mapValue(resultMap_, "primaryGenreName"));
  entry_->setField(QStringLiteral("cover"), mapValue(resultMap_, "artworkUrl100"));
  if(optionalFields().contains(QStringLiteral("itunes"))) {
    entry_->setField(QStringLiteral("itunes"), mapValue(resultMap_, "collectionViewUrl"));
  }
  m_collectionHash.insert(resultMap_.value(QLatin1String("collectionId")).toInt(), entry_);
}

void ItunesFetcher::readTrackInfo(const QVariantMap& resultMap_) {
  QStringList trackInfo;
  trackInfo << mapValue(resultMap_, "trackName")
            << mapValue(resultMap_, "artistName")
            << Tellico::minutes(mapValue(resultMap_, "trackTimeMillis").toInt() / 1000);

  const int collectionId = mapValue(resultMap_, "collectionId").toInt();
  const int idx = mapValue(resultMap_, "trackNumber").toInt();
  if(idx < 1) return;

  QStringList tracks = m_trackList.value(collectionId);
  while(tracks.size() < idx) tracks << QString();
  tracks[idx-1] = trackInfo.join(FieldFormat::columnDelimiterString());
  m_trackList.insert(collectionId, tracks);
}

Tellico::Fetch::ConfigWidget* ItunesFetcher::configWidget(QWidget* parent_) const {
  return new ItunesFetcher::ConfigWidget(parent_, this);
}

QString ItunesFetcher::defaultName() {
  return QStringLiteral("iTunes"); // this is the capitalization they use on their site
}

QString ItunesFetcher::defaultIcon() {
  return favIcon(ITUNES_API_URL);
}

Tellico::StringHash ItunesFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("itunes")] = i18n("iTunes Link");
  return hash;
}

ItunesFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const ItunesFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(ItunesFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString ItunesFetcher::ConfigWidget::preferredName() const {
  return ItunesFetcher::defaultName();
}
