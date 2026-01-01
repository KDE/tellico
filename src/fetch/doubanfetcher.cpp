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
#include "../utils/objvalue.h"
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
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
  static const int DOUBAN_MAX_RETURNS_TOTAL = 20;
  static const char* DOUBAN_API_URL = "https://api.douban.com/v2/";
  static const char* DOUBAN_API_KEY = "93a5facb2118d2e0d1b32e4a2c1e497d01310d6c41715e3ba0c36c58f2c27e4c3705cdafe783261621180632ebdd8ebf55659efb97f6e0d42d192241f0914777";
//  static const char* DOUBAN_API_KEY = "cba90d6c2a48f6c097a1b1d2f2c682b36b0a8deb3505deeacbfddaeb6f5fab99b48d6754023bb681596e340d0637caa84e7686e793a600315664a9cbaccda393";
}

using namespace Tellico;
using Tellico::Fetch::DoubanFetcher;
using namespace Qt::Literals::StringLiterals;

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

  switch(request().collectionType()) {
    case Data::Collection::Book:
    case Data::Collection::Bibtex:
      u.setPath(u.path() + QLatin1String("book/"));
      break;

    case Data::Collection::Video:
      u.setPath(u.path() + QLatin1String("movie/"));
      break;

    case Data::Collection::Album:
      u.setPath(u.path() + QLatin1String("music/"));
      break;

    default:
      myWarning() << "bad collection type:" << request().collectionType();
  }


  QUrlQuery q;
  switch(request().key()) {
    case ISBN:
      u.setPath(u.path() + QLatin1String("isbn/") + ISBNValidator::cleanValue(term_));
      break;

    case Keyword:
      u.setPath(u.path() + QLatin1String("search"));
      q.addQueryItem(QStringLiteral("q"), term_);
      q.addQueryItem(QStringLiteral("count"), QString::number(DOUBAN_MAX_RETURNS_TOTAL));
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      return;
  }

  q.addQueryItem(QStringLiteral("start"), QString::number(0));
  q.addQueryItem(QStringLiteral("apiKey"), Tellico::reverseObfuscate(DOUBAN_API_KEY));
  u.setQuery(q);
  if(!m_testUrl1.isEmpty()) u = m_testUrl1;
//  myDebug() << "url:" << u.url();

  QPointer<KIO::StoredTransferJob> job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  job->addMetaData(QStringLiteral("referrer"), QStringLiteral("https://douban.com"));
  job->addMetaData(QStringLiteral("ConnectTimeout"), QStringLiteral("120"));
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
  Q_EMIT signalDone(this);
}

void DoubanFetcher::slotCompleteISBN(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    myDebug() << "job error -" << job->errorString();
    if(GUI::Proxy::widget()) job->uiDelegate()->showErrorMessage();
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
  const auto obj = doc.object();

  // code == 6000 for no result, 997 for provisioning error, 109 for invalid credential, 104 for invalid apikey, 105 is blocked apikey, 999 is invalid request
  const auto code = objValue(obj, "code");
  if(code == QLatin1String("6000") ||
     code == QLatin1String("997") ||
     code == QLatin1String("999") ||
     code == QLatin1String("109") ||
     code == QLatin1String("104") ||
     code == QLatin1String("105")) {
    const auto msg = objValue(obj, "msg");
    message(msg, MessageHandler::Error);
    myLog() << "DoubanFetcher -" << msg;
  } else {
    Data::EntryPtr entry = createEntry(obj);
    if(entry) {
      FetchResult* r = new FetchResult(this, entry);
      m_entries.insert(r->uid, entry);
      m_matches.insert(r->uid, QUrl(objValue(obj, "url")));
      Q_EMIT signalResultFound(r);
    }
  }

  endJob(job);
}

void DoubanFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    myLog() << job->errorText();
    job->uiDelegate()->showErrorMessage();
    endJob(job);
    return;
  }

  QByteArray data = job->data();
  if(data.isEmpty()) {
    myLog() << "No data received";
    endJob(job);
    return;
  }

#if 0
  myWarning() << "Remove debug from doubanfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test-douban1.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  QJsonParseError error;
  QJsonDocument doc = QJsonDocument::fromJson(data, &error);
  const auto obj = doc.object();

  // code == 6000 for no result, 997 for provisioning error, 109 for invalid credential, 104 for invalid apikey, 105 is blocked apikey
  const auto code = objValue(obj, "code");
  if(code == QLatin1String("6000") ||
     code == QLatin1String("997") ||
     code == QLatin1String("999") ||
     code == QLatin1String("109") ||
     code == QLatin1String("104") ||
     code == QLatin1String("105")) {
    const auto msg = objValue(obj, "msg");
    message(msg, MessageHandler::Error);
    myLog() << "DoubanFetcher -" << msg;
    endJob(job);
    return;
  }

  const auto books = obj["books"_L1].toArray();
  const auto subjects = obj["subjects"_L1].toArray();
  const auto musics = obj["musics"_L1].toArray();
  switch(request().collectionType()) {
    case Data::Collection::Book:
    case Data::Collection::Bibtex:
      for(const auto& v : books) {
        const auto resObj = v.toObject();
        FetchResult* r = new FetchResult(this, objValue(resObj, "title"),
                                         objValue(resObj, "author") + QLatin1Char('/') +
                                         objValue(resObj, "publisher") + QLatin1Char('/') +
                                         objValue(resObj, "pubdate").left(4));
        m_matches.insert(r->uid, QUrl(objValue(resObj, "url")));
        Q_EMIT signalResultFound(r);
      }
      break;

    case Data::Collection::Video:
      for(const auto& v : subjects) {
        const auto resObj = v.toObject();
        FetchResult* r = new FetchResult(this, objValue(resObj, "title"),
                                         objValue(resObj, "directors", "name") + QLatin1Char('/') +
                                         objValue(resObj, "year"));
        // movie results don't appear to have a url field
        m_matches.insert(r->uid, QUrl(QLatin1String(DOUBAN_API_URL) +
                                      QLatin1String("movie/subject/") +
                                      objValue(resObj, "id")));
        Q_EMIT signalResultFound(r);
      }
      break;

    case Data::Collection::Album:
      for(const auto& v : musics) {
        const auto resObj = v.toObject();
        FetchResult* r = new FetchResult(this, objValue(resObj, "title"),
                                         objValue(resObj, "attrs", "singer") + QLatin1Char('/') +
                                         objValue(resObj, "attrs", "publisher") + QLatin1Char('/') +
                                         objValue(resObj, "attrs", "pubdate").left(4));
        // movie results don't appear to have a url field
        m_matches.insert(r->uid, QUrl(QLatin1String(DOUBAN_API_URL) +
                                      QLatin1String("music/") +
                                      objValue(resObj, "id")));
        Q_EMIT signalResultFound(r);
      }
      break;

    default:
      break;
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
  q.addQueryItem(QStringLiteral("apiKey"), Tellico::reverseObfuscate(DOUBAN_API_KEY));
  url.setQuery(q);
  if(!m_testUrl2.isEmpty()) url = m_testUrl2;
//  myDebug() << url.url();
  const QByteArray data = FileHandler::readDataFile(url, true);
#if 0
  myWarning() << "Remove output debug from doubanfetcher.cpp";
  QFile f(QLatin1String("/tmp/test-douban2.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  entry = createEntry(doc.object());

  if(entry) {
    m_entries.insert(uid_, entry);
  }
  return entry;
}

Tellico::Data::EntryPtr DoubanFetcher::createEntry(const QJsonObject& obj_) {
  Data::CollPtr coll;
  Data::EntryPtr entry;
  switch(request().collectionType()) {
    case Data::Collection::Book:
    case Data::Collection::Bibtex:
      coll = new Data::BookCollection(true);
      break;
    case Data::Collection::Video:
      coll = new Data::VideoCollection(true);
      break;
    case Data::Collection::Album:
      coll = new Data::MusicCollection(true);
      break;
    default:
      break;
  }

  if(optionalFields().contains(QStringLiteral("origtitle")) &&
     !objValue(obj_, "origin_title").isEmpty() &&
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
  switch(request().collectionType()) {
    case Data::Collection::Book:
    case Data::Collection::Bibtex:
      populateBookEntry(entry, obj_);
      break;
    case Data::Collection::Video:
      populateVideoEntry(entry, obj_);
      break;
    case Data::Collection::Album:
      populateMusicEntry(entry, obj_);
      break;
    default:
      break;
  }

  // Now read the info_url which is a table with additional info like binding and ISBN
  QString info_url = objValue(obj_, "info_url");
  if(info_url.isEmpty()) {
    info_url = objValue(obj_, "intro_url");
  }
  if(!info_url.isEmpty()) {
//    myDebug() << info_url;
    const QString infoHtml = Tellico::decodeHTML(FileHandler::readTextFile(QUrl::fromUserInput(info_url), true));
    QRegularExpression tableRowRx(QStringLiteral("<tr>.*?<td>(.+?)</td>.*?<td>(.+?)</td>.*?</tr>"),
                                  QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator i = tableRowRx.globalMatch(infoHtml);
    while(i.hasNext()) {
      QRegularExpressionMatch match = i.next();
      const QString m1 = match.captured(1).simplified();
      const QString m2 = match.captured(2).simplified();
      if(m1 == "原作名"_L1) {
        const QString origtitle = QStringLiteral("origtitle");
        if(entry->collection()->hasField(origtitle) && entry->field(origtitle).isEmpty()) {
          entry->setField(origtitle, m2);
        }
      } else if(m1 == "装帧"_L1) {
        if(m2 == "精装"_L1) {
          entry->setField(QStringLiteral("binding"), i18n("Hardback"));
        } else if(m2 == "平装"_L1) {
          entry->setField(QStringLiteral("binding"), i18n("Paperback"));
        }
      } else if(m1 == "ISBN"_L1) {
        entry->setField(QStringLiteral("isbn"), m2);
      } else if(m1 == "介质"_L1) {
        if(m2.contains("CD"_L1)) {
          entry->setField(QStringLiteral("medium"), i18n("Compact Disc"));
        }
      } else if(m1 == "流派"_L1) {
        static const QRegularExpression slashRx(QStringLiteral("\\s*/\\s*"));
        const QStringList genres = m2.split(slashRx);
        entry->setField(QStringLiteral("genre"), genres.join(FieldFormat::rowDelimiterString()));
      } else if(m1 == "片长"_L1) {
        static const QRegularExpression digits(QStringLiteral("\\d+"));
        auto digitsMatch = digits.match(m2);
        if(digitsMatch.hasMatch()) {
          entry->setField(QStringLiteral("running-time"), digitsMatch.captured());
        }
      } else if(m1 == "编剧"_L1) {
        entry->setField(QStringLiteral("writer"), m2);
      }
    }
  }

  const QString image_id = entry->field(QStringLiteral("cover"));
  // if it's still a url, we need to load it
  if(image_id.startsWith("http"_L1)) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(image_id), true, QUrl(info_url));
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
      entry->setField(QStringLiteral("cover"), QString());
    } else {
      entry->setField(QStringLiteral("cover"), id);
    }
  }
  return entry;
}

void DoubanFetcher::populateBookEntry(Data::EntryPtr entry, const QJsonObject& obj_) {
  entry->setField(QStringLiteral("title"), objValue(obj_, "title"));
  entry->setField(QStringLiteral("subtitle"), objValue(obj_, "subtitle"));
  entry->setField(QStringLiteral("author"), objValue(obj_, "author"));
  entry->setField(QStringLiteral("translator"), objValue(obj_, "translator"));
  if(obj_.contains("publisher"_L1)) {
    entry->setField(QStringLiteral("publisher"), objValue(obj_, "publisher"));
  } else {
    entry->setField(QStringLiteral("publisher"), objValue(obj_, "press"));
  }

  const QString binding = objValue(obj_, "binding");
  if(binding == "精装"_L1) {
    entry->setField(QStringLiteral("binding"), i18n("Hardback"));
  } else if(binding == "平装"_L1) {
    entry->setField(QStringLiteral("binding"), i18n("Paperback"));
  }

  entry->setField(QStringLiteral("pub_year"), objValue(obj_, "pubdate").left(4));
  QString isbn;
  if(obj_.contains("isbn10"_L1)) {
    isbn = objValue(obj_, "isbn10");
  } else if(request().key() == ISBN && !request().value().contains(QLatin1Char(';'))) {
    isbn = request().value();
  }
  entry->setField(QStringLiteral("isbn"), ISBNValidator::isbn10(isbn));
  entry->setField(QStringLiteral("pages"), objValue(obj_, "pages"));
  if(obj_.contains("cover_url"_L1)) {
    entry->setField(QStringLiteral("cover"), objValue(obj_, "cover_url"));
  } else {
    entry->setField(QStringLiteral("cover"), objValue(obj_, "image"));
  }

  QString keyword = objValue(obj_, "tags", "title");
  if(keyword.isEmpty()) {
    keyword = objValue(obj_, "tags", "name");
  }
  entry->setField(QStringLiteral("keyword"), keyword);

  if(entry->collection()->hasField(QStringLiteral("origtitle")) &&
     !objValue(obj_, "origin_title").isEmpty()) {
    entry->setField(QStringLiteral("origtitle"), objValue(obj_, "origin_title"));
  }
  if(entry->collection()->hasField(QStringLiteral("douban"))) {
    if(obj_.contains("alt"_L1)) {
      entry->setField(QStringLiteral("douban"), objValue(obj_, "alt"));
    } else {
      entry->setField(QStringLiteral("douban"), objValue(obj_, "url"));
    }
  }
  if(obj_.contains("summary"_L1)) {
    entry->setField(QStringLiteral("plot"), objValue(obj_, "summary"));
  } else {
    entry->setField(QStringLiteral("plot"), objValue(obj_, "intro"));
  }
}

void DoubanFetcher::populateVideoEntry(Data::EntryPtr entry, const QJsonObject& obj_) {
  entry->setField(QStringLiteral("title"), objValue(obj_, "title"));
  entry->setField(QStringLiteral("genre"), objValue(obj_, "genres"));
  entry->setField(QStringLiteral("director"), objValue(obj_, "directors", "name"));
  entry->setField(QStringLiteral("writer"), objValue(obj_, "writers", "name"));
  entry->setField(QStringLiteral("year"), objValue(obj_, "year"));
  if(obj_.contains("cover_url"_L1)) {
    entry->setField(QStringLiteral("cover"), objValue(obj_, "cover_url"));
  } else {
    entry->setField(QStringLiteral("cover"), objValue(obj_, "images", "medium"));
  }
  if(obj_.contains("summary"_L1)) {
    entry->setField(QStringLiteral("plot"), objValue(obj_, "summary"));
  } else {
    entry->setField(QStringLiteral("plot"), objValue(obj_, "intro"));
  }

  entry->setField(QStringLiteral("cast"), objValue(obj_, "casts", "name"));

  if(optionalFields().contains(QStringLiteral("origtitle")) &&
     !objValue(obj_, "original_title").isEmpty()) {
    entry->setField(QStringLiteral("origtitle"), objValue(obj_, "original_title"));
  }
  if(optionalFields().contains(QStringLiteral("douban"))) {
    entry->setField(QStringLiteral("douban"), objValue(obj_, "alt"));
  }
}

void DoubanFetcher::populateMusicEntry(Data::EntryPtr entry, const QJsonObject& obj_) {
  entry->setField(QStringLiteral("title"), objValue(obj_, "title"));
  if(obj_.contains("cover_url"_L1)) {
    entry->setField(QStringLiteral("cover"), objValue(obj_, "cover_url"));
  } else {
    entry->setField(QStringLiteral("cover"), objValue(obj_, "image"));
  }
  entry->setField(QStringLiteral("artist"), objValue(obj_, "attrs", "singer"));
  entry->setField(QStringLiteral("label"), objValue(obj_, "attrs", "publisher"));
  entry->setField(QStringLiteral("year"), objValue(obj_, "attrs", "pubdate").left(4));

  if(objValue(obj_, "attrs", "media") == "Audio CD"_L1 ||
     objValue(obj_, "attrs", "media") == "CD"_L1) {
    entry->setField(QStringLiteral("medium"), i18n("Compact Disc"));
  }

  QStringList tracks;
  const auto songList = obj_["songs"_L1].toArray();
  for(const auto& v : songList) {
    const auto trackMap = v.toObject();
    const QString title = objValue(trackMap, "title");
    QString artists = objValue(trackMap, "artist_names");
    if(artists.isEmpty()) artists = entry->field(QStringLiteral("artist"));
    tracks << title + FieldFormat::columnDelimiterString() +
              artists + FieldFormat::columnDelimiterString() +
              Tellico::minutes(trackMap["duration"_L1].toInt());
  }
  entry->setField(QStringLiteral("track"), tracks.join(FieldFormat::rowDelimiterString()));

  if(optionalFields().contains(QStringLiteral("origtitle")) &&
     !objValue(obj_, "original_title").isEmpty()) {
    entry->setField(QStringLiteral("origtitle"), objValue(obj_, "original_title"));
  }
  if(optionalFields().contains(QStringLiteral("douban"))) {
    entry->setField(QStringLiteral("douban"), objValue(obj_, "alt"));
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
  return favIcon(QUrl(QStringLiteral("http://www.douban.com")),
                 QUrl(QStringLiteral("https://tellico-project.org/img/douban-favicon.ico")));
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
