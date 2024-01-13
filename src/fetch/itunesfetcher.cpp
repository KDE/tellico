/***************************************************************************
    Copyright (C) 2023 Robby Stephenson <robby@periapsis.org>
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
#include "../collections/bookcollection.h"
#include "../collections/musiccollection.h"
#include "../collections/videocollection.h"
#include "../images/imagefactory.h"
#include "../gui/combobox.h"
#include "../utils/guiproxy.h"
#include "../utils/isbnvalidator.h"
#include "../utils/string_utils.h"
#include "../utils/mapvalue.h"
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
#include <QGridLayout>
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
    , m_started(false)
    , m_isTV(false)
    , m_imageSize(LargeImage) {
}

ItunesFetcher::~ItunesFetcher() {
}

QString ItunesFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool ItunesFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Keyword || k == ISBN || k == UPC;
}

bool ItunesFetcher::canFetch(int type) const {
  return type == Data::Collection::Book
      || type == Data::Collection::Album
      || type == Data::Collection::Video;
}

void ItunesFetcher::readConfigHook(const KConfigGroup& config_) {
  const int imageSize = config_.readEntry("Image Size", -1);
  if(imageSize > -1) {
    m_imageSize = static_cast<ImageSize>(imageSize);
  }
}

void ItunesFetcher::saveConfigHook(KConfigGroup& config_) {
  Q_UNUSED(config_)
}

void ItunesFetcher::search() {
  m_started = true;
  m_isTV = false;

  QUrl u(QString::fromLatin1(ITUNES_API_URL));
  u = u.adjusted(QUrl::StripTrailingSlash);
  QUrlQuery q;
  switch(request().key()) {
    case Keyword:
      u.setPath(u.path() + QLatin1String("/search"));
      if(collectionType() == Data::Collection::Book) {
        q.addQueryItem(QStringLiteral("media"), QLatin1String("audiobook"));
        q.addQueryItem(QStringLiteral("entity"), QLatin1String("audiobook"));
      } else if(collectionType() == Data::Collection::Album) {
        q.addQueryItem(QStringLiteral("media"), QLatin1String("music"));
        q.addQueryItem(QStringLiteral("entity"), QLatin1String("album"));
      } else if(collectionType() == Data::Collection::Video) {
        q.addQueryItem(QStringLiteral("media"), QLatin1String("movie,tvShow"));
        q.addQueryItem(QStringLiteral("entity"), QLatin1String("movie,tvSeason"));
      }
      q.addQueryItem(QStringLiteral("limit"), QString::number(ITUNES_MAX_RETURNS_TOTAL));
      q.addQueryItem(QStringLiteral("term"),  QString::fromLatin1(QUrl::toPercentEncoding(request().value())));
      break;

    case ISBN:
      u.setPath(u.path() + QLatin1String("/lookup"));
      {
        QString isbn = ISBNValidator::isbn13(request().value());
        isbn.remove(QLatin1Char('-'));
        q.addQueryItem(QStringLiteral("isbn"), isbn);
      }
      break;

    case UPC:
      u.setPath(u.path() + QLatin1String("/lookup"));
      if(collectionType() == Data::Collection::Album) {
        // include songs
        q.addQueryItem(QStringLiteral("entity"), QLatin1String("song"));
      }
      q.addQueryItem(QStringLiteral("upc"), request().value());
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
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
  myWarning() << "Remove debug from itunesfetcher.cpp";
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
  if(collectionType() == Data::Collection::Book) {
    coll = new Data::BookCollection(true);
  } else if(collectionType() == Data::Collection::Album) {
    coll = new Data::MusicCollection(true);
  } else if(collectionType() == Data::Collection::Video) {
    coll = new Data::VideoCollection(true);
  }
  Q_ASSERT(coll);
  if(!coll) {
    stop();
    return;
  }

  // placeholder for collection id, to be removed later
  Data::FieldPtr f1(new Data::Field(QStringLiteral("collectionId"), QString(), Data::Field::Number));
  coll->addField(f1);
  if(optionalFields().contains(QStringLiteral("itunes"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("itunes"), i18n("iTunes Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  if(collectionType() == Data::Collection::Video &&
     optionalFields().contains(QStringLiteral("episode"))) {
    coll->addField(Data::Field::createDefaultField(Data::Field::EpisodeField));
  }

  QList<FetchResult*> fetchResults;
  foreach(const QJsonValue& result, results) {
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
#if 0
      myWarning() << "Remove debug from itunesfetcher.cpp";
      QFile f(QStringLiteral("/tmp/test-itunes-tracks.json"));
      if(f.open(QIODevice::WriteOnly)) {
        QTextStream t(&f);
        t.setCodec("UTF-8");
        t << job->data();
      }
      f.close();
#endif
      QJsonDocument doc = QJsonDocument::fromJson(job->data());
      QJsonArray results = doc.object().value(QLatin1String("results")).toArray();
      foreach(const QJsonValue& result, results) {
        auto obj = result.toObject();
        if(obj.value(QLatin1String("wrapperType")) == QLatin1String("track")) {
          readTrackInfo(obj.toVariantMap());
        }
      }
      auto discsInColl = m_trackList.value(collectionId.toInt());
      bool changeTrackTitle = true;
      for(int disc = 0; disc < discsInColl.count(); ++disc) {
        QString trackField = QStringLiteral("track");
        if(disc > 0) {
          trackField.append(QString::number(disc+1));
          Data::FieldPtr f2(new Data::Field(trackField,
                                            i18n("Tracks (Disc %1)", disc+1),
                                            Data::Field::Table));
          f2->setFormatType(FieldFormat::FormatTitle);
          f2->setProperty(QStringLiteral("columns"), QStringLiteral("3"));
          f2->setProperty(QStringLiteral("column1"), i18n("Title"));
          f2->setProperty(QStringLiteral("column2"), i18n("Artist"));
          f2->setProperty(QStringLiteral("column3"), i18n("Length"));
          entry->collection()->addField(f2);
          // also change the title of the first track field
          if(changeTrackTitle) {
            Data::FieldPtr f1 = entry->collection()->fieldByName(QStringLiteral("track"));
            f1->setTitle(i18n("Tracks (Disc %1)", 1));
            entry->collection()->modifyField(f1);
            changeTrackTitle = false;
          }
        }
        entry->setField(trackField, discsInColl.at(disc).join(FieldFormat::rowDelimiterString()));
      }
    }
  }

  if(m_isTV && optionalFields().contains(QStringLiteral("episode"))) {
    populateEpisodes(entry);
  }

  // image might still be a URL
  const QString image_id = entry->field(QStringLiteral("cover"));
  if(image_id.contains(QLatin1Char('/'))) {
    // default image is the 100x100 size and considered 'Small'
    QString newImage = image_id;
    if(m_imageSize == LargeImage) {
      newImage.replace(QLatin1String("100x100"), QLatin1String("600x600"));
    }
    QString id = ImageFactory::addImage(QUrl::fromUserInput(newImage), true /* quiet */);
    if(id.isEmpty() && newImage != image_id) {
      // fallback to original
      id = ImageFactory::addImage(QUrl::fromUserInput(image_id), true /* quiet */);
    }
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
  if(collectionType() == Data::Collection::Book) {
    QString title = mapValue(resultMap_, "collectionName");
    if(title.isEmpty()) title = mapValue(resultMap_, "trackName");
    entry_->setField(QStringLiteral("title"), title);
    entry_->setField(QStringLiteral("author"), mapValue(resultMap_, "artistName"));
    entry_->setField(QStringLiteral("plot"), mapValue(resultMap_, "description"));
    static const QRegularExpression publisherRx(QStringLiteral("^© \\d{4} (.+)$"));
    auto publisherMatch = publisherRx.match(mapValue(resultMap_, "copyright"));
    if(publisherMatch.hasMatch()) {
      entry_->setField(QStringLiteral("publisher"), publisherMatch.captured(1));
    }
  } else if(collectionType() == Data::Collection::Album) {
    entry_->setField(QStringLiteral("title"), mapValue(resultMap_, "collectionName"));
    entry_->setField(QStringLiteral("artist"), mapValue(resultMap_, "artistName"));
    static const QRegularExpression labelRx(QStringLiteral("^℗ \\d{4} ([^,]+)"));
    auto labelMatch = labelRx.match(mapValue(resultMap_, "copyright"));
    if(labelMatch.hasMatch()) {
      entry_->setField(QStringLiteral("label"), labelMatch.captured(1));
    }
  } else if(collectionType() == Data::Collection::Video) {
    if(mapValue(resultMap_, "collectionType") == QLatin1String("TV Season")) {
      m_isTV = true;
      // collection Name includes season
      entry_->setField(QStringLiteral("title"), mapValue(resultMap_, "collectionName"));
      // artistName is TV Show title
      entry_->setField(QStringLiteral("keyword"), mapValue(resultMap_, "artistName"));
    } else {
      entry_->setField(QStringLiteral("title"), mapValue(resultMap_, "trackName"));
      entry_->setField(QStringLiteral("director"), mapValue(resultMap_, "artistName"));
    }
    entry_->setField(QStringLiteral("nationality"), mapValue(resultMap_, "country"));
    QString cert = mapValue(resultMap_, "contentAdvisoryRating");
    if(cert == QStringLiteral("NR")) {
      cert = QLatin1Char('U');
    }
    if(!cert.isEmpty()) {
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
    }
    entry_->setField(QStringLiteral("plot"), mapValue(resultMap_, "longDescription"));
  }
  if(collectionType() == Data::Collection::Book) {
    entry_->setField(QStringLiteral("binding"), i18n("E-Book"));
    entry_->setField(QStringLiteral("pub_year"), mapValue(resultMap_, "releaseDate").left(4));
  } else {
    entry_->setField(QStringLiteral("year"), mapValue(resultMap_, "releaseDate").left(4));
  }

  QStringList genres;
  genres += mapValue(resultMap_, "primaryGenreName");
  const auto genreList = resultMap_.value(QLatin1String("genres")).toList();
  for(const auto& genre : genreList) {
    genres += genre.toString();
  }
  genres.removeDuplicates();
  genres.removeOne(QString()); // no empty genres
  genres.removeOne(QLatin1String("Books")); // too generic
  entry_->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));

  if(m_imageSize != NoImage) {
    entry_->setField(QStringLiteral("cover"), mapValue(resultMap_, "artworkUrl100"));
  }

  if(optionalFields().contains(QStringLiteral("itunes"))) {
    entry_->setField(QStringLiteral("itunes"), mapValue(resultMap_, "collectionViewUrl"));
  }

  m_collectionHash.insert(resultMap_.value(QLatin1String("collectionId")).toInt(), entry_);
}

void ItunesFetcher::populateEpisodes(Data::EntryPtr entry_) {
  const QString collectionId = entry_->field(QLatin1String("collectionId"));
  QUrl u(QString::fromLatin1(ITUNES_API_URL));
  u = u.adjusted(QUrl::StripTrailingSlash);
  u.setPath(u.path() + QLatin1String("/lookup"));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("entity"), QLatin1String("tvEpisode"));
  q.addQueryItem(QStringLiteral("id"), collectionId);
  u.setQuery(q);

  auto job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  if(!job->exec()) {
    myDebug() << "Failed downloa ditunes episodes";
    return;
   }

#if 0
  myWarning() << "Remove debug2 from ItunesFetcher.cpp";
  QFile f(QStringLiteral("/tmp/test-itunes-episodes.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << job->data();
  }
  f.close();
#endif

  static const QRegularExpression seasonRx(QStringLiteral("Season (\\d+)"));
  QMap<int, QString> episodeMap; // mapping episode number to episode string
  QJsonDocument doc = QJsonDocument::fromJson(job->data());
  QJsonArray results = doc.object().value(QLatin1String("results")).toArray();
  foreach(const QJsonValue& result, results) {
    auto map = result.toObject().toVariantMap();
    if(mapValue(map, "kind") != QStringLiteral("tv-episode")) continue;
    int seasonNumber = 1;
    // the season number is in the collection title
    auto match = seasonRx.match(mapValue(map, "collectionName"));
    if(match.hasMatch()) {
      seasonNumber = match.captured(1).toInt();
    }
    QString ep = mapValue(map, "trackName") + FieldFormat::columnDelimiterString() +
                 QString::number(seasonNumber) + FieldFormat::columnDelimiterString() +
                 mapValue(map, "trackNumber");
    episodeMap.insert(seasonNumber*1000 + mapValue(map, "trackNumber").toInt(), ep);
  }
  // the QMap sorts the values in ascending order by key
  const auto episodes = episodeMap.values();
  entry_->setField(QStringLiteral("episode"), episodes.join(FieldFormat::rowDelimiterString()));
}

void ItunesFetcher::readTrackInfo(const QVariantMap& resultMap_) {
  QStringList trackInfo;
  trackInfo << mapValue(resultMap_, "trackName")
            << mapValue(resultMap_, "artistName")
            << Tellico::minutes(mapValue(resultMap_, "trackTimeMillis").toInt() / 1000);

  const int collectionId = mapValue(resultMap_, "collectionId").toInt();
  const int discNum = mapValue(resultMap_, "discNumber").toInt();
  const int trackNum = mapValue(resultMap_, "trackNumber").toInt();
  if(trackNum < 1) return;

  auto discsInColl = m_trackList.value(collectionId);
  while(discsInColl.size() < discNum) discsInColl << QStringList();

  auto tracks = discsInColl.at(discNum-1);
  while(tracks.size() < trackNum) tracks << QString();

  tracks[trackNum-1] = trackInfo.join(FieldFormat::columnDelimiterString());
  discsInColl[discNum-1] = tracks;
  m_trackList.insert(collectionId, discsInColl);
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
  hash[QStringLiteral("episode")] = i18n("Episodes");
  return hash;
}

ItunesFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const ItunesFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* label = new QLabel(i18n("&Image size: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_imageCombo = new GUI::ComboBox(optionsWidget());
  m_imageCombo->addItem(i18n("No Image"), NoImage);
  m_imageCombo->addItem(i18n("Small Image"), SmallImage);
  m_imageCombo->addItem(i18n("Large Image"), LargeImage);
  void (GUI::ComboBox::* activatedInt)(int) = &GUI::ComboBox::activated;
  connect(m_imageCombo, activatedInt, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_imageCombo, row, 1);
  label->setBuddy(m_imageCombo);

  l->setRowStretch(++row, 10);

  // now add additional fields widget
  addFieldsWidget(ItunesFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    m_imageCombo->setCurrentData(fetcher_->m_imageSize);
  } else { // defaults
    m_imageCombo->setCurrentData(SmallImage);
  }
}

void ItunesFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const int n = m_imageCombo->currentData().toInt();
  config_.writeEntry("Image Size", n);
}

QString ItunesFetcher::ConfigWidget::preferredName() const {
  return ItunesFetcher::defaultName();
}
