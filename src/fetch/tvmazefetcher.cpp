/***************************************************************************
    Copyright (C) 2020 Robby Stephenson <robby@periapsis.org>
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

#include "tvmazefetcher.h"
#include "../collections/videocollection.h"
#include "../images/imagefactory.h"
#include "../core/filehandler.h"
#include "../utils/guiproxy.h"
#include "../utils/mapvalue.h"
#include "../core/tellico_strings.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KJob>
#include <KJobUiDelegate>
#include <KJobWidgets>
#include <KIO/StoredTransferJob>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

namespace {
  static const int TVMAZE_MAX_RETURNS_TOTAL = 20;
  static const char* TVMAZE_API_URL = "https://api.tvmaze.com";
}

using namespace Tellico;
using Tellico::Fetch::TVmazeFetcher;

TVmazeFetcher::TVmazeFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false) {
}

TVmazeFetcher::~TVmazeFetcher() {
}

QString TVmazeFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

// https://www.tvmaze.com/api#licensing
QString TVmazeFetcher::attribution() const {
  return TC_I18N3(providedBy, QLatin1String("https://tvmaze.com"), QLatin1String("TVmaze"));
}

bool TVmazeFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title;
}

bool TVmazeFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

void TVmazeFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_)
}

void TVmazeFetcher::saveConfigHook(KConfigGroup& config_) {
  Q_UNUSED(config_)
}

void TVmazeFetcher::search() {
  continueSearch();
}

void TVmazeFetcher::continueSearch() {
  m_started = true;


  QUrl u(QString::fromLatin1(TVMAZE_API_URL));

  switch(request().key()) {
    case Title:
      u = u.adjusted(QUrl::StripTrailingSlash);
      u.setPath(u.path() + QLatin1String("/search/shows"));
      {
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("q"), request().value());
        u.setQuery(q);
      }
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result, this, &TVmazeFetcher::slotComplete);
}

void TVmazeFetcher::stop() {
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

Tellico::Fetch::FetchRequest TVmazeFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

void TVmazeFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  const QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "TVmaze: no data";
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = nullptr;

#if 0
  myWarning() << "Remove debug from tvmazefetcher..cpp";
  QFile f(QStringLiteral("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  Data::CollPtr coll(new Data::VideoCollection(true));
  // always add the tvmaze-id for fetchEntryHook
  Data::FieldPtr field(new Data::Field(QStringLiteral("tvmaze-id"), QString(), Data::Field::Line));
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
  if(optionalFields().contains(QStringLiteral("alttitle"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("alttitle"), i18n("Alternative Titles"), Data::Field::Table));
    field->setFormatType(FieldFormat::FormatTitle);
    coll->addField(field);
  }

  QJsonDocument doc = QJsonDocument::fromJson(data);
  const QJsonArray results = doc.array();

  if(results.isEmpty()) {
    stop();
    return;
  }

  int count = 0;
  foreach(const QJsonValue& result, results) {
    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toObject().value(QLatin1String("show"))
                               .toObject().toVariantMap(), false);

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
    ++count;
    if(count >= TVMAZE_MAX_RETURNS_TOTAL) {
      break;
    }
  }

  stop();
}

Tellico::Data::EntryPtr TVmazeFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  const QString id = entry->field(QStringLiteral("tvmaze-id"));
  if(!id.isEmpty()) {
    // quiet
    QUrl u(QString::fromLatin1(TVMAZE_API_URL));
    u.setPath(QStringLiteral("/shows/%1").arg(id));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("embed[]"), QLatin1String("cast"));
    q.addQueryItem(QStringLiteral("embed[]"), QLatin1String("crew"));
    if(optionalFields().contains(QStringLiteral("episode"))) {
      q.addQueryItem(QStringLiteral("embed[]"), QLatin1String("episodes"));
    }
    if(optionalFields().contains(QStringLiteral("alttitle"))) {
      q.addQueryItem(QStringLiteral("embed[]"), QLatin1String("akas"));
    }
    u.setQuery(q);
    QByteArray data = FileHandler::readDataFile(u, true);
#if 0
    myWarning() << "Remove debug2 from tvmazefetcher..cpp";
    QFile f(QStringLiteral("/tmp/test2.json"));
    if(f.open(QIODevice::WriteOnly)) {
      QTextStream t(&f);
      t << data;
    }
    f.close();
#endif
    QJsonDocument doc = QJsonDocument::fromJson(data);
    populateEntry(entry, doc.object().toVariantMap(), true);
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
  entry->setField(QStringLiteral("tvmaze-id"), QString());

  return entry;
}

void TVmazeFetcher::populateEntry(Data::EntryPtr entry_, const QVariantMap& resultMap_, bool fullData_) {
  entry_->setField(QStringLiteral("tvmaze-id"), mapValue(resultMap_, "id"));
  entry_->setField(QStringLiteral("title"), mapValue(resultMap_, "name"));
  entry_->setField(QStringLiteral("year"),  mapValue(resultMap_, "premiered").left(4));

  // if we only need cursory data, then we're done
  if(!fullData_) {
    return;
  }

  QStringList directors, producers, writers, composers;
  QVariantList crewList = resultMap_.value(QStringLiteral("_embedded")).toMap()
                                    .value(QStringLiteral("crew")).toList();
  foreach(const QVariant& crew, crewList) {
    const QVariantMap crewMap = crew.toMap();
    const QString job = mapValue(crewMap, "type");
    // going to get a lot of producers
    if(job.contains(QLatin1String("Director"))) {
      directors += mapValue(crewMap, "person", "name");
    } else if(job.contains(QLatin1String("Producer"))) {
      producers += mapValue(crewMap, "person", "name");
    } else if(job.contains(QLatin1String("Creator"))) {
      writers += mapValue(crewMap, "person", "name");
    } else if(job.contains(QLatin1String("Composer"))) {
      composers += mapValue(crewMap, "person", "name");
    }
  }
  entry_->setField(QStringLiteral("director"), directors.join(FieldFormat::delimiterString()));
  entry_->setField(QStringLiteral("producer"), producers.join(FieldFormat::delimiterString()));
  entry_->setField(QStringLiteral("writer"),     writers.join(FieldFormat::delimiterString()));
  entry_->setField(QStringLiteral("composer"), composers.join(FieldFormat::delimiterString()));

  const QString network(QStringLiteral("network"));
  if(entry_->collection()->hasField(network)) {
    entry_->setField(network, mapValue(resultMap_, "network", "name"));
  }

  const QString imdb(QStringLiteral("imdb"));
  if(entry_->collection()->hasField(imdb)) {
    entry_->setField(imdb, QLatin1String("https://www.imdb.com/title/") + mapValue(resultMap_, "externals", "imdb"));
  }

  QStringList actors;
  QVariantList castList = resultMap_.value(QStringLiteral("_embedded")).toMap()
                                    .value(QStringLiteral("cast")).toList();
  foreach(const QVariant& cast, castList) {
    QVariantMap castMap = cast.toMap();
    actors << mapValue(castMap, "person", "name") + FieldFormat::columnDelimiterString() + mapValue(castMap, "character", "name");
  }
  entry_->setField(QStringLiteral("cast"), actors.join(FieldFormat::rowDelimiterString()));

  QStringList genres;
  foreach(const QVariant& genre, resultMap_.value(QLatin1String("genres")).toList()) {
    genres << genre.toString();
  }
  entry_->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));

  const QString episode(QStringLiteral("episode"));
  if(entry_->collection()->hasField(episode)) {
    QStringList episodes;
    QVariantList episodeList = resultMap_.value(QStringLiteral("_embedded")).toMap()
                                         .value(QStringLiteral("episodes")).toList();
    foreach(const QVariant& row, episodeList) {
      QVariantMap map = row.toMap();
      episodes << mapValue(map, "name") + FieldFormat::columnDelimiterString() +
                  mapValue(map, "season") + FieldFormat::columnDelimiterString() +
                  mapValue(map, "number");
    }
    entry_->setField(episode, episodes.join(FieldFormat::rowDelimiterString()));
  }

  const QString alttitle(QStringLiteral("alttitle"));
  if(entry_->collection()->hasField(alttitle)) {
    QStringList alttitles;
    QVariantList titleList = resultMap_.value(QStringLiteral("_embedded")).toMap()
                                       .value(QStringLiteral("akas")).toList();
    foreach(const QVariant& title, titleList) {
      alttitles << mapValue(title.toMap(), "name");
    }
    entry_->setField(alttitle, alttitles.join(FieldFormat::rowDelimiterString()));
  }

  entry_->setField(QStringLiteral("cover"), mapValue(resultMap_, "image", "original"));
  entry_->setField(QStringLiteral("language"), mapValue(resultMap_, "language"));
  entry_->setField(QStringLiteral("plot"), mapValue(resultMap_, "summary"));
}

Tellico::Fetch::ConfigWidget* TVmazeFetcher::configWidget(QWidget* parent_) const {
  return new TVmazeFetcher::ConfigWidget(parent_, this);
}

QString TVmazeFetcher::defaultName() {
  return QStringLiteral("TVmaze");
}

QString TVmazeFetcher::defaultIcon() {
  return favIcon("https://static.tvmaze.com/images/favico/favicon.ico");
}

Tellico::StringHash TVmazeFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("imdb")] = i18n("IMDb Link");
  hash[QStringLiteral("episode")] = i18n("Episodes");
  hash[QStringLiteral("network")] = i18n("Network");
  hash[QStringLiteral("alttitle")] = i18n("Alternative Titles");
  // TODO: network
  return hash;
}

TVmazeFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const TVmazeFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(TVmazeFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString TVmazeFetcher::ConfigWidget::preferredName() const {
  return TVmazeFetcher::defaultName();
}
