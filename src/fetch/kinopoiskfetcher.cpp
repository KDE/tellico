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
#include "ratelimiter.h"
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
#include <QApplicationStatic>

using namespace Qt::Literals::StringLiterals;

namespace {
  static const char* KINOPOISK_API_SEARCH_URL = "https://kinopoiskapiunofficial.tech/api/v2.1/films/search-by-keyword";
  static const char* KINOPOISK_API_FILM_URL = "https://kinopoiskapiunofficial.tech/api/v2.2/films/";
  static const char* KINOPOISK_API_STAFF_URL = "https://kinopoiskapiunofficial.tech/api/v1/staff";
  static const char* KINOPOISK_API_KEY = "9ca8395a794fb28b82e01120a6968bbf03651271fd9ce5d5371a096d4f7dc7a3caa8361ba8914425a1c5c0f4f5d88dbd3d0fccaa781ca18cd4b2b587ebdeaac89cfa771622162a12";
  static const int KINOPOISK_DEFAULT_CAST_SIZE = 10;

  QList<Tellico::Fetch::RateLimiter::Tier> kinopoiskTiers() {
    using namespace std::chrono_literals;
    // https://kinopoiskapiunofficial.tech
    return {
      {u"burst"_s, 25, 1s},
      {u"daily"_s, 500, 24h}
    };
  }

  Q_APPLICATION_STATIC(Tellico::Fetch::RateLimiter,
                       s_kinopoiskLimiter,
                       kinopoiskTiers())

  Tellico::Fetch::RateLimiter& kinopoiskLimiter() {
    return *s_kinopoiskLimiter;
  }
}

using namespace Tellico;
using Tellico::Fetch::KinoPoiskFetcher;

KinoPoiskFetcher::KinoPoiskFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false), m_redirected(false), m_numCast(KINOPOISK_DEFAULT_CAST_SIZE) {
  m_apiKey = Tellico::reverseObfuscate(KINOPOISK_API_KEY);
}

KinoPoiskFetcher::~KinoPoiskFetcher() = default;

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

  QUrl u(QString::fromLatin1(KINOPOISK_API_SEARCH_URL));
  QUrlQuery q;

  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("keyword"), request().value());
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  u.setQuery(q);
//  myDebug() << "url: " << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  configureJob(m_job);
  connect(m_job.data(), &KJob::result, this, &KinoPoiskFetcher::slotComplete);
  if(!kinopoiskLimiter().addJob(m_job)) {
    myDebug() << "failed to add job";
    stop();
  }
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

void KinoPoiskFetcher::slotComplete(KJob*) {
  if(m_job->error()) {
    myDebug() << m_job->errorString();
    m_job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  const auto data = m_job->data();

#if 0
  myWarning() << "Remove debug from kinopoiskfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test-kinopoisk-results.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
  if(doc.isNull()) {
    myWarning() << "Bad KinoPoisk search JSON:" << parseError.errorString();
    stop();
    return;
  }

  const auto films = doc.object().value(QLatin1StringView("films")).toArray();
  for(const auto& value : films) {
    const auto obj = value.toObject();

    const auto filmId = obj.value(QLatin1StringView("filmId")).toInt();
    if(filmId == 0) {
      continue;
    }
    QString title = obj.value(QLatin1StringView("nameRu")).toString();
    if(title.isEmpty()) {
      title = obj.value(QLatin1StringView("nameEn")).toString();
    }
    const QString year = obj.value(QLatin1StringView("year")).toString();
    auto res = new FetchResult(this, title, year);
    m_matches.insert(res->uid, filmId);
    Q_EMIT signalResultFound(res);
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

  const auto filmId = QString::number(m_matches[uid_]);
  if(filmId.isEmpty()) {
    return Data::EntryPtr();
  }
  entry = requestEntry(filmId);
  if(!entry) {
    return Data::EntryPtr();
  }

  const QString cover = entry->field(QStringLiteral("cover"));
  if(!cover.isEmpty()) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(cover), true /* quiet */,
                                              QUrl(QString::fromLatin1("https://www.kinopoisk.ru")) /* referer */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QStringLiteral("cover"), id);
  }

  const QString kinopoisk(QStringLiteral("kinopoisk"));
  if(optionalFields().contains(kinopoisk)) {
    if(!entry->collection()->hasField(kinopoisk)) {
      Data::FieldPtr field(new Data::Field(kinopoisk, i18n("KinoPoisk Link"), Data::Field::URL));
      field->setCategory(i18n("General"));
      entry->collection()->addField(field);
    }
    const QString url(QStringLiteral("https://www.kinopoisk.ru/film/") + filmId);
    entry->setField(kinopoisk, url);
  }

  m_entries.insert(uid_, entry); // keep for later
  return entry;
}

Tellico::Data::EntryPtr KinoPoiskFetcher::requestEntry(const QString& filmId_) {
  QUrl url(QLatin1String(KINOPOISK_API_FILM_URL) + filmId_);

  auto getJob = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
  configureJob(getJob);
  if(!kinopoiskLimiter().execJob(getJob)) {
    myWarning() << "unable to read" << url;
    return Data::EntryPtr();
  }

  auto data = getJob->data();
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
  configureJob(getJob);
  if(!kinopoiskLimiter().execJob(getJob)) {
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

void KinoPoiskFetcher::configureJob(QPointer<KIO::StoredTransferJob> job_) {
  KJobWidgets::setWindow(job_, GUI::Proxy::widget());
  job_->addMetaData(QStringLiteral("content-type"), QStringLiteral("application/json"));
  job_->addMetaData(QStringLiteral("customHTTPHeader"), QStringLiteral("X-API-KEY: ") + m_apiKey);
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
