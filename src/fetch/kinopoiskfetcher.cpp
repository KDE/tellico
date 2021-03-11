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

#include "kinopoiskfetcher.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../images/imagefactory.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/Job>
#include <KJobUiDelegate>
#include <KJobWidgets/KJobWidgets>

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>

namespace {
  static const char* KINOPOISK_SEARCH_URL = "https://www.kinopoisk.ru/index.php";
  static const char* KINOPOISK_IMAGE_SIZE = "300x450";
}

using namespace Tellico;
using Tellico::Fetch::KinoPoiskFetcher;

KinoPoiskFetcher::KinoPoiskFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false) {
}

KinoPoiskFetcher::~KinoPoiskFetcher() {
}

QString KinoPoiskFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool KinoPoiskFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

bool KinoPoiskFetcher::canSearch(FetchKey k) const {
  return k == Title;
}

void KinoPoiskFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void KinoPoiskFetcher::search() {
  m_started = true;
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
      myWarning() << "key not recognized: " << request().key();
      stop();
      return;
  }
  u.setQuery(q);
//  myDebug() << "url: " << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result, this, &KinoPoiskFetcher::slotComplete);
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
  emit signalDone(this);
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
    t.setCodec("UTF-8");
    t << output;
  }
  f.close();
#endif

  // look for a paragraph, class=",", with an internal /ink to "/level/1/film..."
  QRegularExpression resultRx(QStringLiteral("<p class=\"name\">\\s*"
                                             "<a href=\"/film[^\"]+\".*? data-url=\"([^\"]*)\".*?>(.*?)</a>\\s*"
                                             "<span class=\"year\">(.*?)</span"));

  QString href, title, year;
  QRegularExpressionMatchIterator i = resultRx.globalMatch(output);
  while(m_started && i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    href = match.captured(1);
    title = match.captured(2);
    year = match.captured(3);
    if(!href.isEmpty()) {
      QUrl url(QString::fromLatin1(KINOPOISK_SEARCH_URL));
      url = url.resolved(QUrl(href));
//      myDebug() << url << title << year;
      FetchResult* r = new FetchResult(this, title, year);
      m_matches.insert(r->uid, url);
      emit signalResultFound(r);
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
    myDebug() << "no text results";
    return Data::EntryPtr();
  }

#if 0
  myDebug() << url.url();
  myWarning() << "Remove debug from kinopoiskfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test2.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << results;
  }
  f.close();
#endif

  entry = parseEntry(results);
  if(!entry) {
    // might want to check LD+JSON format
    myDebug() << "...trying Linked Data";
    entry = parseEntryLinkedData(results);
  }
  if(!entry) {
    myDebug() << "No discernible entry data";
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

Tellico::Data::EntryPtr KinoPoiskFetcher::parseEntry(const QString& str_) {
  QRegularExpression jsonRx(QStringLiteral("<script.*?type=\"application/json\">(.+?)</script>"));
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

  const QString queryId = doc.object().value(QStringLiteral("query")).toObject()
                                      .value(QStringLiteral("id")).toString();
  // if there's no query ID, then this is not a film object to parse
  if(queryId.isEmpty()) {
//    myDebug() << "No query ID...";
    return Data::EntryPtr();
  }

  // otherwise, we're good to go
  Data::CollPtr coll(new Data::VideoCollection(true));
  Data::EntryPtr entry(new Data::Entry(coll));
  coll->addEntries(entry);

  QJsonObject dataObject = doc.object().value(QStringLiteral("props")).toObject()
                                        .value(QStringLiteral("apolloState")).toObject()
                                        .value(QStringLiteral("data")).toObject();
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
    if(fieldName == QLatin1String("title")) {
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
  QRegularExpression jsonRx(QStringLiteral("<script.*?type=\"application/ld\\+json\">(.+?)</script>"));
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

  // otherwise, we're good to go
  Data::CollPtr coll(new Data::VideoCollection(true));
  Data::EntryPtr entry(new Data::Entry(coll));
  coll->addEntries(entry);

  QVariantMap objectMap = doc.object().toVariantMap();
  entry->setField(QStringLiteral("title"), mapValue(objectMap, "name"));
  entry->setField(QStringLiteral("year"), mapValue(objectMap, "datePublished").left(4));
  entry->setField(QStringLiteral("nationality"), mapValue(objectMap, "countryOfOrigin"));
  entry->setField(QStringLiteral("cast"), mapValue(objectMap, "actor", "name"));
  entry->setField(QStringLiteral("director"), mapValue(objectMap, "director", "name"));
  entry->setField(QStringLiteral("producer"), mapValue(objectMap, "producer", "name"));
  entry->setField(QStringLiteral("genre"), mapValue(objectMap, "genre"));
  entry->setField(QStringLiteral("plot"), mapValue(objectMap, "description"));
  QString cover = mapValue(objectMap, "image");
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
  if(key_.contains(QLatin1String("DIRECTOR"), Qt::CaseInsensitive)) {
    return QStringLiteral("director");
  }
  if(key_.contains(QLatin1String("WRITER"), Qt::CaseInsensitive)) {
    return QStringLiteral("writer");
  }
  if(key_.contains(QLatin1String("PRODUCER"), Qt::CaseInsensitive)) {
    return QStringLiteral("producer");
  }
  if(key_.contains(QLatin1String("COMPOSER"), Qt::CaseInsensitive)) {
    return QStringLiteral("composer");
  }
  if(key_.contains(QLatin1String("ACTOR"), Qt::CaseInsensitive)) {
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

  QJsonObject valueObj = obj_.value(value_.toObject().value(QStringLiteral("id")).toString()).toObject();

  // if it has a 'person' field, gotta grab the person name
  if(valueObj.contains(QLatin1String("person"))) {
    return fieldValueFromObject(obj_, field_, valueObj.value(QLatin1String("person")), allowed_);
  }

  if(field_ == QLatin1String("title")) {
    QString title = valueObj.value(QStringLiteral("russian")).toString();
    // return original if russian is not available
    return title.isEmpty() ? valueObj.value(QStringLiteral("original")).toString() : title;
  } else if(field_ == QLatin1String("origtitle")) {
    return valueObj.value(QStringLiteral("original")).toString();
  } else if(field_ == QLatin1String("cover")) {
    QString url = valueObj.value(QStringLiteral("avatarsUrl")).toString();
    if(url.startsWith(QLatin1Char('/'))) {
      url.prepend(QLatin1String("https:"));
    }
    // also add size
    url.append(QLatin1Char('/') + QLatin1String(KINOPOISK_IMAGE_SIZE));
    return url;
  } else if(field_ == QLatin1String("certification")) {
    Q_ASSERT(allowed_.size() == 5);
    // default collection has 5 MPAA values
    if(allowed_.size() != 5) return QString();
    const QString mpaa = valueObj.value(QLatin1String("mpaa")).toString();
    if(mpaa == QLatin1String("g")) {
      return allowed_.at(0);
    } else if(mpaa == QLatin1String("pg")) {
      return allowed_.at(1);
    } else if(mpaa == QLatin1String("pg13")) {
      return allowed_.at(2);
    } else if(mpaa == QLatin1String("r")) {
      return allowed_.at(3);
    } else {
      return allowed_.at(4);
    }
  // with an 'originalName' or 'name' field return that
  // and check this before comparing against field names for people, like 'director'
  } else if(valueObj.contains(QLatin1String("originalName")) || valueObj.contains(QLatin1String("name"))) {
    const QString name = valueObj.value(QStringLiteral("name")).toString();
    // prefer name to originalName
    return name.isEmpty() ? valueObj.value(QStringLiteral("originalName")).toString() : name;
  } else if(valueObj.contains(QLatin1String("items"))) {
    // some additional nesting apparently
    // key in film object points to director object, whose 'items' is an array where each 'is' points to
    // a director object which has a person.id pointing to a person object with a 'name' and 'original' value
    // valueObj is the director so we want the items array
    QJsonValue itemsValue = valueObj.value(QLatin1String("items"));
    if(!itemsValue.isArray()) {
      myDebug() << "items value is not an array";
      return QString();
    }
    return fieldValueFromObject(obj_, field_, itemsValue, allowed_);
  }

  return QString();
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
  hash[QStringLiteral("origtitle")] = i18n("Original Title");
  hash[QStringLiteral("kinopoisk")] = i18n("KinoPoisk Link");
  return hash;
}

KinoPoiskFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const KinoPoiskFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  addFieldsWidget(KinoPoiskFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString KinoPoiskFetcher::ConfigWidget::preferredName() const {
  return KinoPoiskFetcher::defaultName();
}
