/***************************************************************************
    Copyright (C) 2017-2021 Robby Stephenson <robby@periapsis.org>
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

#include "kinopoiskfetcher.h"
#include "../utils/guiproxy.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../images/imagefactory.h"
#include "../utils/string_utils.h"
#include "../utils/objvalue.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KJobUiDelegate>
#include <KJobWidgets>

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QSpinBox>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>

namespace {
  static const char* KINOPOISK_SEARCH_URL = "https://www.kinopoisk.ru/index.php";
  static const char* KINOPOISK_IMAGE_SIZE = "300x450";
  static const char* KINOPOISK_API_FILM_URL  = "https://kinopoiskapiunofficial.tech/api/v2.2/films/";
  static const char* KINOPOISK_API_STAFF_URL = "https://kinopoiskapiunofficial.tech/api/v1/staff";
  static const char* KINOPOISK_API_KEY = "9ca8395a794fb28b82e01120a6968bbf03651271fd9ce5d5371a096d4f7dc7a3caa8361ba8914425a1c5c0f4f5d88dbd3d0fccaa781ca18cd4b2b587ebdeaac89cfa771622162a12";
  static const int KINOPOISK_DEFAULT_CAST_SIZE = 10;
}

using namespace Tellico;
using Tellico::Fetch::KinoPoiskFetcher;

KinoPoiskFetcher::KinoPoiskFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false), m_redirected(false), m_numCast(KINOPOISK_DEFAULT_CAST_SIZE) {
  m_apiKey = Tellico::reverseObfuscate(KINOPOISK_API_KEY);
}

KinoPoiskFetcher::~KinoPoiskFetcher() {
}

QString KinoPoiskFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool KinoPoiskFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

bool KinoPoiskFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title;
}

void KinoPoiskFetcher::readConfigHook(const KConfigGroup& config_) {
  m_numCast = config_.readEntry("Max Cast", KINOPOISK_DEFAULT_CAST_SIZE);
}

void KinoPoiskFetcher::search() {
  m_started = true;
  m_redirected = false;
  m_redirectUrl.clear();
  m_matches.clear();

  QUrl u(QString::fromLatin1(KINOPOISK_SEARCH_URL));
  QUrlQuery q;

  switch(request().key()) {
    case Title:
      // first means return first result only
      //q.addQueryItem(QStringLiteral("first"), QStringLiteral("yes"));
      q.addQueryItem(QStringLiteral("kp_query"), request().value());
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  u.setQuery(q);
//  myDebug() << "url: " << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result, this, &KinoPoiskFetcher::slotComplete);
  connect(m_job.data(), &KIO::TransferJob::redirection,
          this, &KinoPoiskFetcher::slotRedirection);
}

void KinoPoiskFetcher::stop() {
  if(!m_started) {
    return;
  }

  if(m_job) {
    m_job->kill();
    m_job = nullptr;
  }
  m_started = false;
  Q_EMIT signalDone(this);
}

void KinoPoiskFetcher::slotRedirection(KIO::Job*, const QUrl& toUrl_) {
  if(m_redirectUrl.isEmpty()) {
    myDebug() << "Redirected to" << toUrl_;
    m_redirectUrl = toUrl_;
  }
  m_redirected = true;
}

void KinoPoiskFetcher::slotComplete(KJob*) {
  if(m_job->error()) {
    m_job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }

  const QString output = Tellico::decodeHTML(data);
#if 0
  myWarning() << "Remove debug from kinopoiskfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test1.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << output;
  }
  f.close();
#endif

  if(m_started && m_redirected) {
    // don't pull the data here, just add it to a single response
    auto res = new FetchResult(this, request().value(), QString());
    m_matches.insert(res->uid, m_redirectUrl);
    Q_EMIT signalResultFound(res);
  }

  // look for a paragraph, class=",", with an internal /ink to "/level/1/film..."
  QRegularExpression resultRx(QStringLiteral("<p class=\"name\">\\s*"
                                             "<a href=\"/film[^\"]+\".*? data-url=\"([^\"]*)\".*?>(.*?)</a>\\s*"
                                             "<span class=\"year\">(.*?)</span"));

  QString href, title, year;
  QRegularExpressionMatchIterator i = resultRx.globalMatch(output);
  while(m_started && !m_redirected && i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    href = match.captured(1);
    title = match.captured(2);
    year = match.captured(3);
    if(!href.isEmpty()) {
      QUrl url(QString::fromLatin1(KINOPOISK_SEARCH_URL));
      url = url.resolved(QUrl(href));
//      myDebug() << url << title << year;
      auto res = new FetchResult(this, title, year);
      m_matches.insert(res->uid, url);
      Q_EMIT signalResultFound(res);
    }
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = nullptr;
  stop();
}

Tellico::Data::EntryPtr KinoPoiskFetcher::fetchEntryHook(uint uid_) {
  // if we already grabbed this one, then just pull it out of the dict
  Data::EntryPtr entry = m_entries[uid_];
  if(entry) {
    return entry;
  }

  QUrl url = m_matches[uid_];
  if(url.isEmpty()) {
    myWarning() << "no url in map";
    return Data::EntryPtr();
  }

  QPointer<KIO::StoredTransferJob> getJob = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
  getJob->addMetaData(QStringLiteral("referrer"), QString::fromLatin1(KINOPOISK_SEARCH_URL));
  KJobWidgets::setWindow(getJob, GUI::Proxy::widget());
  if(!getJob->exec()) {
    myWarning() << "unable to read" << url;
    return Data::EntryPtr();
  }

// the HTML response has the character encoding after the first 1024 characters and Qt doesn't seem to detect that
// and potentially falls back to iso-8859-1. Enforce UTF-8
//  const QByteArray data = FileHandler::readDataFile(url, true);
  const QByteArray data = getJob->data();
  const QString results = Tellico::decodeHTML(Tellico::fromHtmlData(data, "UTF-8"));
  if(results.isEmpty()) {
    myDebug() << "KinoPoiskFetcher: no text results";
    return Data::EntryPtr();
  }

#if 0
  myDebug() << url.url();
  myWarning() << "Remove debug from kinopoiskfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test2.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << results;
  }
  f.close();
#endif

  if(results.contains(QLatin1StringView("captcha")) || results.endsWith(QLatin1StringView("</script>"))) {
//    myDebug() << "KinoPoiskFetcher: captcha triggered";
    static const QRegularExpression re(QStringLiteral("/(\\d+)"));
    QRegularExpressionMatch match = re.match(url.url());
    if(match.hasMatch()) {
      entry = requestEntry(match.captured(1));
    }
  } else {
    entry = parseEntry(results);
    if(!entry) {
      // might want to check LD+JSON format
//      myDebug() << "...trying Linked Data";
      entry = parseEntryLinkedData(results);
    }
  }
  if(!entry) {
//    myDebug() << "No discernible entry data";
    return Data::EntryPtr();
  }

  QString cover = entry->field(QStringLiteral("cover"));
  if(!cover.isEmpty()) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(cover), true /* quiet */,
                                              QUrl(QString::fromLatin1(KINOPOISK_SEARCH_URL)) /* referer */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QStringLiteral("cover"), id);
  }

  if(optionalFields().contains(QStringLiteral("kinopoisk"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("kinopoisk"), i18n("KinoPoisk Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    entry->collection()->addField(field);
    entry->setField(QStringLiteral("kinopoisk"), url.url());
  }

  m_entries.insert(uid_, entry); // keep for later
  return entry;
}

Tellico::Data::EntryPtr KinoPoiskFetcher::requestEntry(const QString& filmId_) {
  QUrl url(QLatin1String(KINOPOISK_API_FILM_URL) + filmId_);

  QPointer<KIO::StoredTransferJob> getJob = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
  getJob->addMetaData(QStringLiteral("content-type"), QStringLiteral("application/json"));
  getJob->addMetaData(QStringLiteral("customHTTPHeader"), QStringLiteral("X-API-KEY: ") + m_apiKey);
  KJobWidgets::setWindow(getJob, GUI::Proxy::widget());
  if(!getJob->exec()) {
    myWarning() << "unable to read" << url;
    return Data::EntryPtr();
  }

  QByteArray data = getJob->data();
#if 0
  myDebug() << url;
  myWarning() << "Remove json debug from kinopoiskfetcher.cpp";
  QFile file(QString::fromLatin1("/tmp/test-kinopoisk.json"));
  if(file.open(QIODevice::WriteOnly)) {
    QTextStream t(&file);
    t << data;
  }
  file.close();
#endif

  Data::CollPtr coll(new Data::VideoCollection(true));
  Data::EntryPtr entry(new Data::Entry(coll));
  coll->addEntries(entry);

  QJsonDocument doc = QJsonDocument::fromJson(data);
  const auto obj = doc.object();

  entry->setField(QStringLiteral("title"), objValue(obj, "nameRu"));
  entry->setField(QStringLiteral("year"), objValue(obj, "year"));
  entry->setField(QStringLiteral("nationality"), objValue(obj, "countries", "country"));
  entry->setField(QStringLiteral("genre"), objValue(obj, "genres", "genre"));
  entry->setField(QStringLiteral("running-time"), objValue(obj, "filmLength"));
  entry->setField(QStringLiteral("plot"), objValue(obj, "description"));
  entry->setField(QStringLiteral("cover"), objValue(obj, "posterUrl"));

  const QString cert(QStringLiteral("certification"));
  auto certField = coll->fieldByName(cert);
  if(certField) {
    entry->setField(cert, mpaaRating(objValue(obj, "ratingMpaa"), certField->allowed()));
  }

  const QString imdb(QStringLiteral("imdb"));
  const QString imdbId = objValue(obj, "imdbId");
  if(optionalFields().contains(imdb) && !imdbId.isEmpty()) {
    coll->addField(Data::Field::createDefaultField(Data::Field::ImdbField));
    entry->setField(imdb, QStringLiteral("https://www.imdb.com/title/") + imdbId);
  }
  const QString origTitle(QStringLiteral("origtitle"));
  if(optionalFields().contains(origTitle)) {
    if(!coll->hasField(origTitle)) {
      Data::FieldPtr f(new Data::Field(origTitle, i18n("Original Title")));
      f->setFormatType(FieldFormat::FormatTitle);
      coll->addField(f);
    }
    entry->setField(origTitle, objValue(obj, "nameOriginal"));
  }

  url = QUrl(QLatin1String(KINOPOISK_API_STAFF_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("filmId"), filmId_);
  url.setQuery(q);

  getJob = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
  getJob->addMetaData(QStringLiteral("content-type"), QStringLiteral("application/json"));
  getJob->addMetaData(QStringLiteral("customHTTPHeader"), QStringLiteral("X-API-KEY: ") + m_apiKey);
  KJobWidgets::setWindow(getJob, GUI::Proxy::widget());
  if(!getJob->exec()) {
    myWarning() << "unable to read" << url;
    return Data::EntryPtr();
  }

  data = getJob->data();
#if 0
  myDebug() << url;
  myWarning() << "Remove json2 debug from kinopoiskfetcher.cpp";
  QFile file2(QString::fromLatin1("/tmp/test-kinopoisk-staff.json"));
  if(file2.open(QIODevice::WriteOnly)) {
    QTextStream t(&file2);
    t << data;
  }
  file2.close();
#endif

  QStringList directors, writers, actors, producers, composers;

  const auto staffArray = QJsonDocument::fromJson(data).array();
  const int sz = staffArray.size();
  for(int i = 0; i < sz; ++i) {
    const auto obj = staffArray.at(i).toObject();
    const QString key = objValue(obj, "professionKey");
    QString name = objValue(obj, "nameRu");
    if(name.isEmpty()) name = objValue(obj, "nameEn");
    if(name.isEmpty()) continue;
    if(key == QLatin1String("DIRECTOR")) {
      directors += name;
    } else if(key == QLatin1String("ACTOR")) {
      if(actors.size() < m_numCast) {
        actors += (name + FieldFormat::columnDelimiterString() + objValue(obj, "description"));
      }
    } else if(key == QLatin1String("WRITER")) {
      writers += name;
    } else if(key == QLatin1String("PRODUCER")) {
      producers += name;
    } else if(key == QLatin1String("COMPOSER")) {
      composers += name;
    } else {
//      myDebug() << "...skipping" << key;
    }
  }

  entry->setField(QStringLiteral("director"), directors.join(Tellico::FieldFormat::delimiterString()));
  entry->setField(QStringLiteral("writer"), writers.join(Tellico::FieldFormat::delimiterString()));
  entry->setField(QStringLiteral("producer"), producers.join(Tellico::FieldFormat::delimiterString()));
  entry->setField(QStringLiteral("composer"), composers.join(Tellico::FieldFormat::delimiterString()));
  entry->setField(QStringLiteral("cast"), actors.join(Tellico::FieldFormat::rowDelimiterString()));

  return entry;
}

Tellico::Data::EntryPtr KinoPoiskFetcher::parseEntry(const QString& str_) {
  static const QRegularExpression jsonRx(QStringLiteral("<script.*?type=\"application/json\".*?>(.+?)</script>"));
  QRegularExpressionMatch jsonMatch = jsonRx.match(str_);
  if(!jsonMatch.hasMatch()) {
    myDebug() << "No JSON data";
    return Data::EntryPtr();
  }

  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(jsonMatch.captured(1).toUtf8(), &parseError);
  if(doc.isNull()) {
    myDebug() << "Bad json data:" << parseError.errorString();
    return Data::EntryPtr();
  }

  const QString queryId = doc.object().value(QLatin1StringView("query")).toObject()
                                      .value(QLatin1StringView("id")).toString();
  // if there's no query ID, then this is not a film object to parse
  if(queryId.isEmpty()) {
//    myDebug() << "No query ID...";
    return Data::EntryPtr();
  }

  // otherwise, we're good to go
  Data::CollPtr coll(new Data::VideoCollection(true));
  Data::EntryPtr entry(new Data::Entry(coll));
  coll->addEntries(entry);

  QJsonObject dataObject = doc.object().value(QLatin1StringView("props")).toObject()
                                       .value(QLatin1StringView("apolloState")).toObject()
                                       .value(QLatin1StringView("data")).toObject();
  QJsonObject filmObject = dataObject.value(QStringLiteral("Film:") + queryId).toObject();
  if(filmObject.isEmpty()) {
    filmObject = dataObject.value(QStringLiteral("TvSeries:") + queryId).toObject();
  }
  if(filmObject.isEmpty()) {
    return Data::EntryPtr();
  }
  // iterate over the filmObject members to find the json keys in the dataObject
  QJsonObject::const_iterator i = filmObject.constBegin();
  for( ; i != filmObject.constEnd(); ++i) {
    const QString fieldName = fieldNameFromKey(i.key());
    if(fieldName.isEmpty()) {
      continue;
    }
    Data::FieldPtr field = entry->collection()->fieldByName(fieldName);
    Q_ASSERT(field);
    QString fieldValue = fieldValueFromObject(dataObject, fieldName, i.value(),
                                              field ? field->allowed() : QStringList());
    if(!fieldValue.isEmpty()) {
      entry->setField(fieldName, fieldValue);
    }

    // also add original title
    if(fieldName == QLatin1StringView("title")) {
      const QString origTitle(QStringLiteral("origtitle"));
      if(optionalFields().contains(origTitle)) {
        if(!entry->collection()->hasField(origTitle)) {
          Data::FieldPtr f(new Data::Field(origTitle, i18n("Original Title")));
          f->setFormatType(FieldFormat::FormatTitle);
          entry->collection()->addField(f);
        }
        fieldValue = fieldValueFromObject(dataObject, origTitle, i.value(), QStringList());
        if(!fieldValue.isEmpty()) {
          entry->setField(origTitle, fieldValue);
        }
      }
    }
  }

  return entry;
}

Tellico::Data::EntryPtr KinoPoiskFetcher::parseEntryLinkedData(const QString& str_) {
  QRegularExpression jsonRx(QStringLiteral("<script.*?type=\"application/ld\\+json\".*?>(.+?)</script>"));
  QRegularExpressionMatch jsonMatch = jsonRx.match(str_);
  if(!jsonMatch.hasMatch()) {
    myDebug() << "No LD+JSON data";
    return Data::EntryPtr();
  }

  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(jsonMatch.captured(1).toUtf8(), &parseError);
  if(doc.isNull()) {
    myDebug() << "Bad json data:" << parseError.errorString();
    return Data::EntryPtr();
  }

  // otherwise, we're good to go
  Data::CollPtr coll(new Data::VideoCollection(true));
  Data::EntryPtr entry(new Data::Entry(coll));
  coll->addEntries(entry);

  const auto obj = doc.object();
  entry->setField(QStringLiteral("title"), objValue(obj, "name"));
  entry->setField(QStringLiteral("year"), objValue(obj, "datePublished").left(4));
  entry->setField(QStringLiteral("nationality"), objValue(obj, "countryOfOrigin"));
  entry->setField(QStringLiteral("cast"), objValue(obj, "actor", "name"));
  entry->setField(QStringLiteral("director"), objValue(obj, "director", "name"));
  entry->setField(QStringLiteral("producer"), objValue(obj, "producer", "name"));
  entry->setField(QStringLiteral("genre"), objValue(obj, "genre"));
  entry->setField(QStringLiteral("plot"), objValue(obj, "description"));
  QString cover = objValue(obj, "image");
  if(cover.startsWith(QLatin1Char('/'))) {
    cover.prepend(QLatin1String("https:"));
  }
  entry->setField(QStringLiteral("cover"), cover);

  return entry;
}

// static
QString KinoPoiskFetcher::fieldNameFromKey(const QString& key_) {
  static QHash<QString, QString> fieldHash;
  if(fieldHash.isEmpty()) {
    fieldHash.insert(QStringLiteral("title"), QStringLiteral("title"));
    fieldHash.insert(QStringLiteral("productionYear"), QStringLiteral("year"));
    fieldHash.insert(QStringLiteral("duration"), QStringLiteral("running-time"));
    fieldHash.insert(QStringLiteral("countries"), QStringLiteral("nationality"));
    fieldHash.insert(QStringLiteral("genres"), QStringLiteral("genre"));
    fieldHash.insert(QStringLiteral("restriction"), QStringLiteral("certification"));
    fieldHash.insert(QStringLiteral("synopsis"), QStringLiteral("plot"));
    fieldHash.insert(QStringLiteral("poster"), QStringLiteral("cover"));
  }
  if(fieldHash.contains(key_)) {
    return fieldHash.value(key_);
  }

  // otherwise some wonky key names
  if(key_.contains(QLatin1StringView("DIRECTOR"), Qt::CaseInsensitive)) {
    return QStringLiteral("director");
  }
  if(key_.contains(QLatin1StringView("WRITER"), Qt::CaseInsensitive)) {
    return QStringLiteral("writer");
  }
  if(key_.contains(QLatin1StringView("PRODUCER"), Qt::CaseInsensitive)) {
    return QStringLiteral("producer");
  }
  if(key_.contains(QLatin1StringView("COMPOSER"), Qt::CaseInsensitive)) {
    return QStringLiteral("composer");
  }
  if(key_.contains(QLatin1StringView("ACTOR"), Qt::CaseInsensitive)) {
    return QStringLiteral("cast");
  }
  return QString();
}

QString KinoPoiskFetcher::fieldValueFromObject(const QJsonObject& obj_, const QString& field_,
                                               const QJsonValue& value_, const QStringList& allowed_) {
  // if it's an array, loop over and recurse
  if(value_.isArray()) {
    QJsonArray arr = value_.toArray();
    QStringList fieldValues;
    for(QJsonArray::const_iterator i = arr.constBegin(); i != arr.constEnd(); ++i) {
      const QString value = fieldValueFromObject(obj_, field_, *i, allowed_);
      if(!value.isEmpty()) {
        fieldValues << value;
      }
    }
    return fieldValues.isEmpty() ? QString() : fieldValues.join(field_ == QLatin1String("cast") ?
                                                                Tellico::FieldFormat::rowDelimiterString() :
                                                                Tellico::FieldFormat::delimiterString());
  }

  if(field_ == QLatin1String("year") ||
     field_ == QLatin1String("running-time")) {
    const int n = value_.toInt();
    return n > 0 ? QString::number(n) : QString();
  }
  if(field_ == QLatin1String("plot")) {
    return value_.toString();
  }

  QJsonObject valueObj = value_.toObject();
  // if there's a reference to another object, need to pull it from the higher level data object
  if(valueObj.contains(QLatin1StringView("__ref"))) {
    valueObj = obj_.value(valueObj.value(QLatin1StringView("__ref")).toString()).toObject();
  }

  // if it has a 'person' field, gotta grab the person name
  if(valueObj.contains(QLatin1StringView("person"))) {
    return fieldValueFromObject(obj_, field_, valueObj.value(QLatin1StringView("person")), allowed_);
  }

  if(field_ == QLatin1StringView("title")) {
    const QString title = valueObj.value(QLatin1StringView("russian")).toString();
    // return original if russian is not available
    return title.isEmpty() ? valueObj.value(QLatin1StringView("original")).toString() : title;
  } else if(field_ == QLatin1StringView("origtitle")) {
    return valueObj.value(QLatin1StringView("original")).toString();
  } else if(field_ == QLatin1StringView("cover")) {
    QString url = valueObj.value(QLatin1StringView("avatarsUrl")).toString();
    if(url.startsWith(QLatin1Char('/'))) {
      url.prepend(QLatin1String("https:"));
    }
    // also add size
    url.append(QLatin1Char('/') + QLatin1String(KINOPOISK_IMAGE_SIZE));
    return url;
  } else if(field_ == QLatin1StringView("certification")) {
    return mpaaRating(valueObj.value(QLatin1StringView("mpaa")).toString(), allowed_);
  // with an 'originalName' or 'name' field return that
  // and check this before comparing against field names for people, like 'director'
  } else if(valueObj.contains(QLatin1StringView("originalName")) ||
            valueObj.contains(QLatin1StringView("name"))) {
    const QString name = valueObj.value(QLatin1StringView("name")).toString();
    // prefer name to originalName
    return name.isEmpty() ? valueObj.value(QLatin1StringView("originalName")).toString() : name;
  } else if(valueObj.contains(QLatin1StringView("items"))) {
    // some additional nesting apparently
    // key in film object points to director object, whose 'items' is an array where each 'is' points to
    // a director object which has a person.id pointing to a person object with a 'name' and 'original' value
    // valueObj is the director so we want the items array
    QJsonValue itemsValue = valueObj.value(QLatin1StringView("items"));
    if(!itemsValue.isArray()) {
      myDebug() << "items value is not an array";
      return QString();
    }
    return fieldValueFromObject(obj_, field_, itemsValue, allowed_);
  }

  return QString();
}

QString KinoPoiskFetcher::mpaaRating(const QString& value_, const QStringList& allowed_) {
  // default collection has 5 MPAA values
  if(allowed_.size() != 5) return value_;
  if(value_ == QLatin1StringView("g")) {
    return allowed_.at(0);
  } else if(value_ == QLatin1StringView("pg")) {
    return allowed_.at(1);
  } else if(value_ == QLatin1StringView("pg13")) {
    return allowed_.at(2);
  } else if(value_ == QLatin1StringView("r")) {
    return allowed_.at(3);
  } else {
    return allowed_.at(4);
  }
}

Tellico::Fetch::FetchRequest KinoPoiskFetcher::updateRequest(Data::EntryPtr entry_) {
  QString t = entry_->field(QStringLiteral("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* KinoPoiskFetcher::configWidget(QWidget* parent_) const {
  return new KinoPoiskFetcher::ConfigWidget(parent_);
}

QString KinoPoiskFetcher::defaultName() {
  return QStringLiteral("КиноПоиск (KinoPoisk.ru)");
}

QString KinoPoiskFetcher::defaultIcon() {
  return favIcon("http://www.kinopoisk.ru");
}

Tellico::StringHash KinoPoiskFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("kinopoisk")] = i18n("KinoPoisk Link");
  hash[QStringLiteral("imdb")] = i18n("IMDb Link");
  hash[QStringLiteral("origtitle")] = i18n("Original Title");
  return hash;
}

KinoPoiskFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const KinoPoiskFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* label = new QLabel(i18n("&Maximum cast: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_numCast = new QSpinBox(optionsWidget());
  m_numCast->setMaximum(99);
  m_numCast->setMinimum(0);
  m_numCast->setValue(KINOPOISK_DEFAULT_CAST_SIZE);
  void (QSpinBox::* textChanged)(const QString&) = &QSpinBox::textChanged;
  connect(m_numCast, textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_numCast, row, 1);
  QString w = i18n("The list of cast members may include many people. Set the maximum number returned from the search.");
  label->setWhatsThis(w);
  m_numCast->setWhatsThis(w);
  label->setBuddy(m_numCast);

  l->setRowStretch(++row, 10);

  addFieldsWidget(KinoPoiskFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
  if(fetcher_) {
    m_numCast->setValue(fetcher_->m_numCast);
  }
}

QString KinoPoiskFetcher::ConfigWidget::preferredName() const {
  return KinoPoiskFetcher::defaultName();
}

void KinoPoiskFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  config_.writeEntry("Max Cast", m_numCast->value());
}
