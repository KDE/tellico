/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#include "doubanfetcher.h"
#include "../collections/bookcollection.h"
#include "../collections/videocollection.h"
#include "../collections/musiccollection.h"
#include "../images/imagefactory.h"
#include "../core/filehandler.h"
#include "../utils/guiproxy.h"
#include "../utils/isbnvalidator.h"
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
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
  static const int DOUBAN_MAX_RETURNS_TOTAL = 20;
  static const char* DOUBAN_API_URL = "https://frodo.douban.com/api/v2/";
  // old and unused
  static const char* DOUBAN_API_KEY = "e3828ebc93a3d7e6794bbedfbc8c395a0232bcdf5c647345c4f4fb983a5c1b2b5237daea8dbd99fb20101f7a9afba7c20c6da1c4d5e7e4d60535685c44716555";
}

using namespace Tellico;
using Tellico::Fetch::DoubanFetcher;

DoubanFetcher::DoubanFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false) {
}

DoubanFetcher::~DoubanFetcher() {
}

QString DoubanFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool DoubanFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Keyword || k == ISBN;
}

bool DoubanFetcher::canFetch(int type) const {
  return type == Data::Collection::Book
      || type == Data::Collection::Bibtex
      || type == Data::Collection::Video
      || type == Data::Collection::Album;
}

void DoubanFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void DoubanFetcher::search() {
  m_started = true;
  // split ISBN values
  QStringList searchTerms;
  if(request().key() == ISBN) {
    searchTerms = FieldFormat::splitValue(request().value());
  } else  {
    searchTerms += request().value();
  }
  foreach(const QString& searchTerm, searchTerms) {
    doSearch(searchTerm);
  }
  if(m_jobs.isEmpty()) {
    stop();
  }
}

void DoubanFetcher::doSearch(const QString& term_) {
  QUrl u(QString::fromLatin1(DOUBAN_API_URL));

  QUrlQuery q;
  switch(request().key()) {
    case ISBN:
      u.setPath(u.path() + QLatin1String("book/isbn/") + ISBNValidator::cleanValue(term_));
      break;

    case Keyword:
      u.setPath(u.path() + QLatin1String("search"));
      q.addQueryItem(QStringLiteral("q"), term_);
      q.addQueryItem(QStringLiteral("count"), QString::number(DOUBAN_MAX_RETURNS_TOTAL));
      break;

    default:
      myWarning() << "key not recognized:" << request().key();
      return;
  }

  q.addQueryItem(QLatin1String("start"), QString::number(0));
  q.addQueryItem(QLatin1String("apiKey"), Tellico::reverseObfuscate(DOUBAN_API_KEY));
  u.setQuery(q);
//  myDebug() << "url:" << u.url();

  QPointer<KIO::StoredTransferJob> job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  if(request().key() == ISBN) {
    connect(job.data(), &KJob::result, this, &DoubanFetcher::slotCompleteISBN);
  } else {
    connect(job.data(), &KJob::result, this, &DoubanFetcher::slotComplete);
  }
  m_jobs << job;
}

void DoubanFetcher::endJob(KIO::StoredTransferJob* job_) {
  m_jobs.removeAll(job_);
  if(m_jobs.isEmpty())  {
    stop();
  }
}

void DoubanFetcher::stop() {
  if(!m_started) {
    return;
  }
  foreach(auto job, m_jobs) {
    if(job) {
      job->kill();
    }
  }
  m_jobs.clear();
  m_started = false;
  emit signalDone(this);
}

void DoubanFetcher::slotCompleteISBN(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    job->uiDelegate()->showErrorMessage();
    endJob(job);
    return;
  }

  QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    endJob(job);
    return;
  }

  QJsonDocument doc = QJsonDocument::fromJson(data);
  const QVariantMap resultMap = doc.object().toVariantMap();

  // code == 6000 for no result, 997 for provisioning error
  const auto code = mapValue(resultMap, "code");
  if(code == QLatin1String("6000") || code == QLatin1String("997")) {
    message(mapValue(resultMap, "msg"), MessageHandler::Error);
  } else {
    Data::EntryPtr entry = createEntry(resultMap);
    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    m_matches.insert(r->uid, QUrl(mapValue(resultMap, "url")));
    emit signalResultFound(r);
  }

  endJob(job);
}

void DoubanFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    job->uiDelegate()->showErrorMessage();
    endJob(job);
    return;
  }

  QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    endJob(job);
    return;
  }

#if 0
  myWarning() << "Remove debug from doubanfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test-douban1.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  const QVariantMap resultsMap = doc.object().toVariantMap();
  foreach(const QVariant& v, resultsMap.value(QLatin1String("subjects")).toList()) {
    const QVariantMap resultMap = v.toMap();
    const QString resultType = mapValue(resultMap, "type");
    const QString resultId = mapValue(resultMap, "id");
    FetchResult* r = nullptr;
    if(request().collectionType() == Data::Collection::Book && resultType == QLatin1String("book")) {
      r = new FetchResult(this, mapValue(resultMap, "title"),
                                        mapValue(resultMap, "author") + QLatin1Char('/') +
                                        mapValue(resultMap, "publisher") + QLatin1Char('/') +
                                        mapValue(resultMap, "pubdate").left(4));
      m_matches.insert(r->uid, QUrl(QLatin1String(DOUBAN_API_URL) +
                                    QLatin1String("book/") +
                                    resultId));
      emit signalResultFound(r);
    } else if(request().collectionType() == Data::Collection::Video &&
              (resultType == QLatin1String("movie") || resultType == QLatin1String("tv"))) {
      r = new FetchResult(this, mapValue(resultMap, "title"),
                                        mapValue(resultMap, "directors", "name") + QLatin1Char('/') +
                                        mapValue(resultMap, "year"));
      m_matches.insert(r->uid, QUrl(QLatin1String(DOUBAN_API_URL) +
                                    QLatin1String("movie/") +
                                    resultId));
      emit signalResultFound(r);
    } else if(request().collectionType() == Data::Collection::Album && resultType == QLatin1String("music")) {
      r = new FetchResult(this, mapValue(resultMap, "title"),
                                        mapValue(resultMap, "attrs", "singer") + QLatin1Char('/') +
                                        mapValue(resultMap, "attrs", "publisher") + QLatin1Char('/') +
                                        mapValue(resultMap, "attrs", "pubdate").left(4));
      m_matches.insert(r->uid, QUrl(QLatin1String(DOUBAN_API_URL) +
                                    QLatin1String("music/") +
                                    resultId));
      emit signalResultFound(r);
    }
  }

  endJob(job);
}

Tellico::Data::EntryPtr DoubanFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(entry) {
    return entry;
  }

  QUrl url = m_matches.value(uid_);
  QUrlQuery q;
  q.addQueryItem(QLatin1String("apiKey"), QString::fromLatin1(DOUBAN_API_KEY));
  url.setQuery(q);
  QByteArray data = FileHandler::readDataFile(url, true);
#if 0
  myWarning() << "Remove output debug from doubanfetcher.cpp";
  QFile f(QLatin1String("/tmp/test-douban2.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  entry = createEntry(doc.object().toVariantMap());

  if(entry) {
    m_entries.insert(uid_, entry);
  }
  return entry;
}

Tellico::Data::EntryPtr DoubanFetcher::createEntry(const QVariantMap& resultMap_) {
  Data::CollPtr coll;
  Data::EntryPtr entry;
  switch(request().collectionType()) {
    case Data::Collection::Book:
      coll = new Data::BookCollection(true);
      if(optionalFields().contains(QStringLiteral("origtitle")) &&
        !mapValue(resultMap_, "origin_title").isEmpty() &&
        !coll->hasField(QStringLiteral("origtitle"))) {
        Data::FieldPtr f(new Data::Field(QStringLiteral("origtitle"), i18n("Original Title")));
        f->setFormatType(FieldFormat::FormatTitle);
        coll->addField(f);
      }
      if(optionalFields().contains(QStringLiteral("douban")) &&
        !coll->hasField(QStringLiteral("douban"))) {
        Data::FieldPtr f(new Data::Field(QStringLiteral("douban"), i18n("Douban Link"), Data::Field::URL));
        f->setCategory(i18n("General"));
        coll->addField(f);
      }
      entry = new Data::Entry(coll);
      populateBookEntry(entry, resultMap_);
      break;
    case Data::Collection::Video:
      coll = new Data::VideoCollection(true);
      if(optionalFields().contains(QStringLiteral("origtitle")) &&
        !mapValue(resultMap_, "original_title").isEmpty() &&
        !coll->hasField(QStringLiteral("origtitle"))) {
        Data::FieldPtr f(new Data::Field(QStringLiteral("origtitle"), i18n("Original Title")));
        f->setFormatType(FieldFormat::FormatTitle);
        coll->addField(f);
      }
      if(optionalFields().contains(QStringLiteral("douban")) &&
        !coll->hasField(QStringLiteral("douban"))) {
        Data::FieldPtr f(new Data::Field(QStringLiteral("douban"), i18n("Douban Link"), Data::Field::URL));
        f->setCategory(i18n("General"));
        coll->addField(f);
      }
      entry = new Data::Entry(coll);
      populateVideoEntry(entry, resultMap_);
      break;
    case Data::Collection::Album:
      coll = new Data::MusicCollection(true);
      if(optionalFields().contains(QStringLiteral("origtitle")) &&
        !mapValue(resultMap_, "original_title").isEmpty() &&
        !coll->hasField(QStringLiteral("origtitle"))) {
        Data::FieldPtr f(new Data::Field(QStringLiteral("origtitle"), i18n("Original Title")));
        f->setFormatType(FieldFormat::FormatTitle);
        coll->addField(f);
      }
      if(optionalFields().contains(QStringLiteral("douban")) &&
        !coll->hasField(QStringLiteral("douban"))) {
        Data::FieldPtr f(new Data::Field(QStringLiteral("douban"), i18n("Douban Link"), Data::Field::URL));
        f->setCategory(i18n("General"));
        coll->addField(f);
      }
      entry = new Data::Entry(coll);
      populateMusicEntry(entry, resultMap_);
      break;
    default:
      break;
  }

  // Now read the info_url which is a table with additional info like binding and ISBN
  QString info_url = mapValue(resultMap_, "info_url");
  if(info_url.isEmpty()) {
    info_url = mapValue(resultMap_, "intro_url");
  }
  if(!info_url.isEmpty()) {
    const QString infoHtml = Tellico::decodeHTML(FileHandler::readTextFile(QUrl::fromUserInput(info_url), true));
    QRegularExpression tableRowRx(QStringLiteral("<tr>.*?<td>(.+?)</td>.*?<td>(.+?)</td>.*?</tr>"),
                                  QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator i = tableRowRx.globalMatch(infoHtml);
    while(i.hasNext()) {
      QRegularExpressionMatch match = i.next();
      const QString m1 = match.captured(1).simplified();
      const QString m2 = match.captured(2).simplified();
      if(m1 == QString::fromUtf8("原作名")) {
        const QString origtitle = QStringLiteral("origtitle");
        if(entry->collection()->hasField(origtitle) && entry->field(origtitle).isEmpty()) {
          entry->setField(origtitle, m2);
        }
      } else if(m1 == QString::fromUtf8("装帧")) {
        if(m2 == QStringLiteral("精装")) {
          entry->setField(QStringLiteral("binding"), i18n("Hardback"));
        } else if(m2 == QStringLiteral("平装")) {
          entry->setField(QStringLiteral("binding"), i18n("Paperback"));
        }
      } else if(m1 == QLatin1String("ISBN")) {
        entry->setField(QStringLiteral("isbn"), m2);
      } else if(m1 == QString::fromUtf8("介质")) {
        if(m2.contains(QLatin1String("CD"))) {
          entry->setField(QStringLiteral("medium"), i18n("Compact Disc"));
        }
      } else if(m1 == QString::fromUtf8("流派")) {
        const QStringList genres = m2.split(QRegularExpression(QLatin1String("\\s*/\\s*")));
        entry->setField(QStringLiteral("genre"), genres.join(FieldFormat::rowDelimiterString()));
      } else if(m1 == QString::fromUtf8("片长")) {
        QRegularExpression digits(QLatin1String("\\d+"));
        QRegularExpressionMatch digitsMatch = digits.match(m2);
        if(digitsMatch.hasMatch()) {
          entry->setField(QStringLiteral("running-time"), digitsMatch.captured());
        }
      } else if(m1 == QString::fromUtf8("编剧")) {
        entry->setField(QStringLiteral("writer"), m2);
      }
    }
  }

  const QString image_id = entry->field(QStringLiteral("cover"));
  // if it's still a url, we need to load it
  if(image_id.startsWith(QLatin1String("http"))) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(image_id), true);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
      entry->setField(QStringLiteral("cover"), QString());
    } else {
      entry->setField(QStringLiteral("cover"), id);
    }
  }
  return entry;
}

void DoubanFetcher::populateBookEntry(Data::EntryPtr entry, const QVariantMap& resultMap_) {
  entry->setField(QStringLiteral("title"), mapValue(resultMap_, "title"));
  entry->setField(QStringLiteral("subtitle"), mapValue(resultMap_, "subtitle"));
  entry->setField(QStringLiteral("author"), mapValue(resultMap_, "author"));
  entry->setField(QStringLiteral("translator"), mapValue(resultMap_, "translator"));
  if(resultMap_.contains(QLatin1String("publisher"))) {
    entry->setField(QStringLiteral("publisher"), mapValue(resultMap_, "publisher"));
  } else {
    entry->setField(QStringLiteral("publisher"), mapValue(resultMap_, "press"));
  }

  const QString binding = mapValue(resultMap_, "binding");
  if(binding == QStringLiteral("精装")) {
    entry->setField(QStringLiteral("binding"), i18n("Hardback"));
  } else if(binding == QStringLiteral("平装")) {
    entry->setField(QStringLiteral("binding"), i18n("Paperback"));
  }

  entry->setField(QStringLiteral("pub_year"), mapValue(resultMap_, "pubdate").left(4));
  QString isbn;
  if(resultMap_.contains(QLatin1String("isbn10"))) {
    isbn = mapValue(resultMap_, "isbn10");
  } else if(request().key() == ISBN && !request().value().contains(QLatin1Char(';'))) {
    isbn = request().value();
  }
  entry->setField(QStringLiteral("isbn"), ISBNValidator::isbn10(isbn));
  entry->setField(QStringLiteral("pages"), mapValue(resultMap_, "pages"));
  if(resultMap_.contains(QLatin1String("cover_url"))) {
    entry->setField(QStringLiteral("cover"), mapValue(resultMap_, "cover_url"));
  } else {
    entry->setField(QStringLiteral("cover"), mapValue(resultMap_, "image"));
  }

  QString keyword = mapValue(resultMap_, "tags", "title");
  if(keyword.isEmpty()) {
    keyword = mapValue(resultMap_, "tags", "name");
  }
  entry->setField(QStringLiteral("keyword"), keyword);

  if(entry->collection()->hasField(QStringLiteral("origtitle")) &&
     !mapValue(resultMap_, "origin_title").isEmpty()) {
    entry->setField(QStringLiteral("origtitle"), mapValue(resultMap_, "origin_title"));
  }
  if(entry->collection()->hasField(QStringLiteral("douban"))) {
    if(resultMap_.contains(QLatin1String("alt"))) {
      entry->setField(QStringLiteral("douban"), mapValue(resultMap_, "alt"));
    } else {
      entry->setField(QStringLiteral("douban"), mapValue(resultMap_, "url"));
    }
  }
  if(resultMap_.contains(QLatin1String("summary"))) {
    entry->setField(QStringLiteral("plot"), mapValue(resultMap_, "summary"));
  } else {
    entry->setField(QStringLiteral("plot"), mapValue(resultMap_, "intro"));
  }
}

void DoubanFetcher::populateVideoEntry(Data::EntryPtr entry, const QVariantMap& resultMap_) {
  entry->setField(QStringLiteral("title"), mapValue(resultMap_, "title"));
  entry->setField(QStringLiteral("genre"), mapValue(resultMap_, "genres"));
  entry->setField(QStringLiteral("director"), mapValue(resultMap_, "directors", "name"));
  entry->setField(QStringLiteral("writer"), mapValue(resultMap_, "writers", "name"));
  entry->setField(QStringLiteral("year"), mapValue(resultMap_, "year"));
  if(resultMap_.contains(QLatin1String("cover_url"))) {
    entry->setField(QStringLiteral("cover"), mapValue(resultMap_, "cover_url"));
  } else {
    entry->setField(QStringLiteral("cover"), mapValue(resultMap_, "images", "medium"));
  }
  if(resultMap_.contains(QLatin1String("summary"))) {
    entry->setField(QStringLiteral("plot"), mapValue(resultMap_, "summary"));
  } else {
    entry->setField(QStringLiteral("plot"), mapValue(resultMap_, "intro"));
  }

  QStringList actors;
  foreach(const QVariant& v, resultMap_.value(QLatin1String("actors")).toList()) {
    actors << v.toMap().value(QStringLiteral("name")).toString();
  }
  entry->setField(QStringLiteral("cast"), actors.join(FieldFormat::rowDelimiterString()));

  if(optionalFields().contains(QStringLiteral("origtitle")) &&
     !mapValue(resultMap_, "original_title").isEmpty()) {
    entry->setField(QStringLiteral("origtitle"), mapValue(resultMap_, "original_title"));
  }
  if(optionalFields().contains(QStringLiteral("douban"))) {
    entry->setField(QStringLiteral("douban"), mapValue(resultMap_, "alt"));
  }
}

void DoubanFetcher::populateMusicEntry(Data::EntryPtr entry, const QVariantMap& resultMap_) {
  entry->setField(QStringLiteral("title"), mapValue(resultMap_, "title"));
  if(resultMap_.contains(QLatin1String("cover_url"))) {
    entry->setField(QStringLiteral("cover"), mapValue(resultMap_, "cover_url"));
  } else {
    entry->setField(QStringLiteral("cover"), mapValue(resultMap_, "image"));
  }
  entry->setField(QStringLiteral("artist"), mapValue(resultMap_, "singer", "name"));
  entry->setField(QStringLiteral("label"), mapValue(resultMap_, "publisher"));
  entry->setField(QStringLiteral("year"), mapValue(resultMap_, "pubdate").left(4));

  if(mapValue(resultMap_, "media") == QLatin1String("Audio CD") ||
     mapValue(resultMap_, "media") == QLatin1String("CD")) {
    entry->setField(QStringLiteral("medium"), i18n("Compact Disc"));
  }

  QStringList values, tracks;
  foreach(const QVariant& v, resultMap_.value(QLatin1String("songs")).toList()) {
    const QVariantMap trackMap = v.toMap();
    QString title = mapValue(trackMap, "title");
    QString artists = mapValue(trackMap, "artist_names");
    if(artists.isEmpty()) artists = entry->field(QStringLiteral("artist"));
    QString duration = mapValue(trackMap, "duration");
    tracks << title + FieldFormat::columnDelimiterString() +
              artists + FieldFormat::columnDelimiterString() +
              Tellico::minutes(duration.toInt());
  }
  entry->setField(QStringLiteral("track"), tracks.join(FieldFormat::rowDelimiterString()));

  if(optionalFields().contains(QStringLiteral("origtitle")) &&
     !mapValue(resultMap_, "original_title").isEmpty()) {
    entry->setField(QStringLiteral("origtitle"), mapValue(resultMap_, "original_title"));
  }
  if(optionalFields().contains(QStringLiteral("douban"))) {
    entry->setField(QStringLiteral("douban"), mapValue(resultMap_, "alt"));
  }
}

Tellico::Fetch::FetchRequest DoubanFetcher::updateRequest(Data::EntryPtr entry_) {
  QString isbn = entry_->field(QStringLiteral("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }

  QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* DoubanFetcher::configWidget(QWidget* parent_) const {
  return new DoubanFetcher::ConfigWidget(parent_, this);
}

QString DoubanFetcher::defaultName() {
  return QStringLiteral("Douban.com");
}

QString DoubanFetcher::defaultIcon() {
  return favIcon("http://www.douban.com");
}

Tellico::StringHash DoubanFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("origtitle")] = i18n("Original Title");
  hash[QStringLiteral("douban")] = i18n("Douban Link");
  return hash;
}

DoubanFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const DoubanFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(DoubanFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void DoubanFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  Q_UNUSED(config_);
}

QString DoubanFetcher::ConfigWidget::preferredName() const {
  return DoubanFetcher::defaultName();
}
