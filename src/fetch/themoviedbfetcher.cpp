/***************************************************************************
    Copyright (C) 2009-2014 Robby Stephenson <robby@periapsis.org>
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

#include "themoviedbfetcher.h"
#include "../collections/videocollection.h"
#include "../images/imagefactory.h"
#include "../gui/combobox.h"
#include "../core/filehandler.h"
#include "../utils/guiproxy.h"
#include "../utils/objvalue.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KJob>
#include <KJobUiDelegate>
#include <KJobWidgets>
#include <KIO/StoredTransferJob>
#include <KCountryFlagEmojiIconEngine>
#include <KLanguageName>

#include <QLabel>
#include <QLineEdit>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <QStandardPaths>
#include <QSpinBox>

namespace {
  static const int THEMOVIEDB_MAX_RETURNS_TOTAL = 20;
  static const char* THEMOVIEDB_API_URL = "https://api.themoviedb.org";
  static const char* THEMOVIEDB_API_VERSION = "3"; // krazy:exclude=doublequote_chars
  static const char* THEMOVIEDB_API_KEY = "919890b4128d33c729dc368209ece555";
  static const uint THEMOVIEDB_DEFAULT_CAST_SIZE = 10;
  static const uint THEMOVIEDB_MAX_SEASON_COUNT = 10;
}

using namespace Tellico;
using Tellico::Fetch::TheMovieDBFetcher;

TheMovieDBFetcher::TheMovieDBFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false)
    , m_locale(QStringLiteral("en"))
    , m_apiKey(QLatin1String(THEMOVIEDB_API_KEY))
    , m_numCast(THEMOVIEDB_DEFAULT_CAST_SIZE) {
  //  setLimit(THEMOVIEDB_MAX_RETURNS_TOTAL);
}

TheMovieDBFetcher::~TheMovieDBFetcher() {
}

QString TheMovieDBFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

// https://www.themoviedb.org/about/api-terms
QString TheMovieDBFetcher::attribution() const {
  return QStringLiteral("This product uses the TMDb API but is not endorsed or certified by TMDb.");
}

bool TheMovieDBFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title;
}

bool TheMovieDBFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

void TheMovieDBFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key", THEMOVIEDB_API_KEY);
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
  k = config_.readEntry("Locale", "en");
  if(!k.isEmpty()) {
    m_locale = k.toLower();
  }
  k = config_.readEntry("ImageBase");
  if(!k.isEmpty()) {
    m_imageBase = k;
  }
  m_serverConfigDate = config_.readEntry("ServerConfigDate", QDate());
  m_numCast = config_.readEntry("Max Cast", THEMOVIEDB_DEFAULT_CAST_SIZE);
}

void TheMovieDBFetcher::saveConfigHook(KConfigGroup& config_) {
  if(!m_serverConfigDate.isNull()) {
    config_.writeEntry("ServerConfigDate", m_serverConfigDate);
  }
  config_.writeEntry("ImageBase", m_imageBase);
}

void TheMovieDBFetcher::search() {
  continueSearch();
}

void TheMovieDBFetcher::continueSearch() {
  m_started = true;

  QUrl u(QString::fromLatin1(THEMOVIEDB_API_URL));
  u.setPath(QLatin1Char('/') + QLatin1String(THEMOVIEDB_API_VERSION));
  u = u.adjusted(QUrl::StripTrailingSlash);

  QUrlQuery q;
  switch(request().key()) {
    case Title:
      u.setPath(u.path() + QLatin1String("/search/multi"));
      q.addQueryItem(QStringLiteral("query"), request().value());
      break;

    case Raw:
      if(request().data().isEmpty()) {
        u.setPath(u.path() + QLatin1String("/search/multi"));
      } else {
        u.setPath(u.path() + request().data());
      }
      q.setQuery(request().value());
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  q.addQueryItem(QStringLiteral("language"), m_locale);
  q.addQueryItem(QStringLiteral("api_key"), m_apiKey);
  u.setQuery(q);
//  myDebug() << u;

  if(m_apiKey.isEmpty()) {
    myDebug() << source() << "- empty API key";
    message(i18n("An access key is required to use this data source.")
            + QLatin1Char(' ') +
            i18n("Those values must be entered in the data source settings."), MessageHandler::Error);
    stop();
    return;
  }

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result, this, &TheMovieDBFetcher::slotComplete);
}

void TheMovieDBFetcher::stop() {
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

Tellico::Data::EntryPtr TheMovieDBFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  if(m_imageBase.isEmpty() || m_serverConfigDate.daysTo(QDate::currentDate()) > 7) {
    readConfiguration();
  }

  QString id = entry->field(QStringLiteral("tmdb-id"));
  if(!id.isEmpty()) {
    const QString mediaType = entry->field(QStringLiteral("tmdb-type"));
    // quiet
    QUrl u(QString::fromLatin1(THEMOVIEDB_API_URL));
    u.setPath(QStringLiteral("/%1/%2/%3")
              .arg(QLatin1String(THEMOVIEDB_API_VERSION),
                   mediaType.isEmpty() ? QLatin1String("movie") : mediaType,
                   id));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("api_key"), m_apiKey);
    q.addQueryItem(QStringLiteral("language"), m_locale);
    QString append;
    if(optionalFields().contains(QStringLiteral("episode"))) {
      // can only do one season at a time?
      append = QLatin1String("alternative_titles,credits");
      for(uint snum = 1; snum <= THEMOVIEDB_MAX_SEASON_COUNT; ++snum) {
        append += QLatin1String(",season/") + QString::number(snum);
      }
    } else {
      append = QLatin1String("alternative_titles,credits");
    }
    q.addQueryItem(QStringLiteral("append_to_response"), append);
    u.setQuery(q);
    QByteArray data = FileHandler::readDataFile(u, true);
#if 0
    myWarning() << "Remove debug2 from themoviedbfetcher.cpp" << u.url();
    QFile f(QStringLiteral("/tmp/test2.json"));
    if(f.open(QIODevice::WriteOnly)) {
      QTextStream t(&f);
      t << data;
    }
    f.close();
#endif
    QJsonDocument doc = QJsonDocument::fromJson(data);
    populateEntry(entry, doc.object(), true);
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

  // don't want to include TMDb ID field
  entry->setField(QStringLiteral("tmdb-id"), QString());
  entry->setField(QStringLiteral("tmdb-type"), QString());

  return entry;
}

Tellico::Fetch::FetchRequest TheMovieDBFetcher::updateRequest(Data::EntryPtr entry_) {
  QString imdb = entry_->field(QStringLiteral("imdb"));
  if(imdb.isEmpty()) {
    imdb = entry_->field(QStringLiteral("imdb-id"));
  }
  if(!imdb.isEmpty()) {
    QRegularExpression ttRx(QStringLiteral("tt\\d+"));
    auto ttMatch = ttRx.match(imdb);
    if(ttMatch.hasMatch()) {
      FetchRequest req(Raw, QStringLiteral("external_source=imdb_id"));
      req.setData(QLatin1String("/find/") + ttMatch.captured()); // tell the request to use a different endpoint
      return req;
    }
  }

  const QString title = entry_->field(QStringLiteral("title"));
  const QString year = entry_->field(QStringLiteral("year"));
  if(!title.isEmpty()) {
    if(year.isEmpty()) {
      return FetchRequest(Title, title);
    } else {
      return FetchRequest(Raw, QStringLiteral("query=\"%1\"&year=%2").arg(title, year));
    }
  }
  return FetchRequest();
}

void TheMovieDBFetcher::slotComplete(KJob* job_) {
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
  myWarning() << "Remove debug from themoviedbfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  Data::CollPtr coll(new Data::VideoCollection(true));
  // always add the tmdb-id for fetchEntryHook
  Data::FieldPtr field(new Data::Field(QStringLiteral("tmdb-id"), QStringLiteral("TMDb ID"), Data::Field::Line));
  field->setCategory(i18n("General"));
  coll->addField(field);
  field = new Data::Field(QStringLiteral("tmdb-type"), QStringLiteral("TMDb Type"), Data::Field::Line);
  field->setCategory(i18n("General"));
  coll->addField(field);

  if(optionalFields().contains(QStringLiteral("tmdb"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("tmdb"), i18n("TMDb Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  if(optionalFields().contains(QStringLiteral("imdb"))) {
    coll->addField(Data::Field::createDefaultField(Data::Field::ImdbField));
  }
  if(optionalFields().contains(QStringLiteral("alttitle"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("alttitle"), i18n("Alternative Titles"), Data::Field::Table));
    field->setFormatType(FieldFormat::FormatTitle);
    coll->addField(field);
  }
  if(optionalFields().contains(QStringLiteral("origtitle"))) {
    Data::FieldPtr f(new Data::Field(QStringLiteral("origtitle"), i18n("Original Title")));
    f->setFormatType(FieldFormat::FormatTitle);
    coll->addField(f);
  }
  if(optionalFields().contains(QStringLiteral("network"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("network"), i18n("Network"), Data::Field::Line));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  if(optionalFields().contains(QStringLiteral("episode"))) {
    coll->addField(Data::Field::createDefaultField(Data::Field::EpisodeField));
  }

  QJsonDocument doc = QJsonDocument::fromJson(data);
  const auto result = doc.object();

  auto resultList = result.value(QLatin1StringView("results")).toArray();
  if(resultList.isEmpty()) {
    resultList = result.value(QLatin1StringView("movie_results")).toArray();
  }
  if(resultList.isEmpty()) {
    resultList = result.value(QLatin1StringView("tv_results")).toArray();
  }

  if(resultList.isEmpty()) {
    myDebug() << "no results";
    stop();
    return;
  }

  int count = 0;
  for(const auto& result : std::as_const(resultList)) {
    // skip people results
    const auto resultObj = result.toObject();
    if(objValue(resultObj, "media_type") == QLatin1String("person")) {
      continue;
    }
//    myDebug() << "found result:" << result;

    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, resultObj, false);

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    Q_EMIT signalResultFound(r);
    ++count;
    if(count >= THEMOVIEDB_MAX_RETURNS_TOTAL) {
      break;
    }
  }

  stop();
}

void TheMovieDBFetcher::populateEntry(Data::EntryPtr entry_, const QJsonObject& obj_, bool fullData_) {
  entry_->setField(QStringLiteral("tmdb-id"), objValue(obj_, "id"));
  const QString tmdbType = QStringLiteral("tmdb-type");
  if(entry_->field(tmdbType).isEmpty()) {
    entry_->setField(tmdbType, objValue(obj_, "media_type"));
  }
  entry_->setField(QStringLiteral("title"), objValue(obj_, "title"));
  if(entry_->title().isEmpty()) {
    entry_->setField(QStringLiteral("title"), objValue(obj_, "name"));
  }
  if(obj_.contains(QLatin1StringView("release_date"))) {
    entry_->setField(QStringLiteral("year"), objValue(obj_, "release_date").left(4));
  } else {
    entry_->setField(QStringLiteral("year"), objValue(obj_, "first_air_date").left(4));
  }

  QStringList directors, producers, writers, composers;
  const auto crewList = obj_[QLatin1StringView("credits")][QLatin1StringView("crew")].toArray();
  for(const auto& crew : crewList) {
    const auto crewObj = crew.toObject();
    const QString job = objValue(crewObj, "job");
    if(job == QLatin1String("Director")) {
      directors += objValue(crewObj, "name");
    } else if(job == QLatin1String("Producer")) {
      producers += objValue(crewObj, "name");
    } else if(job == QLatin1String("Screenplay")) {
      writers += objValue(crewObj, "name");
    } else if(job == QLatin1String("Original Music Composer")) {
      composers += objValue(crewObj, "name");
    }
  }
  entry_->setField(QStringLiteral("director"), directors.join(FieldFormat::delimiterString()));
  entry_->setField(QStringLiteral("producer"), producers.join(FieldFormat::delimiterString()));
  entry_->setField(QStringLiteral("writer"),     writers.join(FieldFormat::delimiterString()));
  entry_->setField(QStringLiteral("composer"), composers.join(FieldFormat::delimiterString()));

  // if we only need cursory data, then we're done
  if(!fullData_) {
    return;
  }

  if(entry_->collection()->hasField(QStringLiteral("tmdb"))) {
    QString mediaType = entry_->field(tmdbType);
    if(mediaType.isEmpty()) mediaType = QLatin1String("movie");
    entry_->setField(QStringLiteral("tmdb"), QStringLiteral("https://www.themoviedb.org/%1/%2").arg(mediaType, objValue(obj_, "id")));
  }
  if(entry_->collection()->hasField(QStringLiteral("imdb"))) {
    const QString imdbId = objValue(obj_, "imdb_id");
    if(!imdbId.isEmpty()) {
      entry_->setField(QStringLiteral("imdb"), QLatin1String("https://www.imdb.com/title/") + imdbId);
    }
  }
  if(entry_->collection()->hasField(QStringLiteral("origtitle"))) {
    QString otitle = objValue(obj_, "original_title");
    if(otitle.isEmpty()) otitle = objValue(obj_, "original_name");
    entry_->setField(QStringLiteral("origtitle"), otitle);
  }
  if(entry_->collection()->hasField(QStringLiteral("alttitle"))) {
    QString atitles = objValue(obj_, "alternative_titles", "titles", "title");
    if(atitles.isEmpty()) {
      atitles += objValue(obj_, "alternative_titles", "results", "title");
    }
    entry_->setField(QStringLiteral("alttitle"), atitles);
  }
  if(entry_->collection()->hasField(QStringLiteral("network"))) {
    entry_->setField(QStringLiteral("network"), objValue(obj_, "networks", "name"));
  }
  if(optionalFields().contains(QStringLiteral("episode"))) {
    QStringList episodes;
    for(uint snum = 1; snum <= THEMOVIEDB_MAX_SEASON_COUNT; ++snum) {
      const QString seasonString = QLatin1String("season/") + QString::number(snum);
      if(!obj_.contains(seasonString)) {
        break; // no more seasons
      }
      const auto episodeList = obj_[seasonString][QLatin1StringView("episodes")].toArray();
      for(const auto& row : episodeList) {
        const auto epObj = row.toObject();
        // episode title, season, episode number
        episodes << objValue(epObj, "name") + FieldFormat::columnDelimiterString() +
                    objValue(epObj, "season_number") + FieldFormat::columnDelimiterString() +
                    objValue(epObj, "episode_number");
      }
    }
    entry_->setField(QStringLiteral("episode"), episodes.join(FieldFormat::rowDelimiterString()));
  }

  QStringList actors;
  const auto castList = obj_[QLatin1StringView("credits")][QLatin1StringView("cast")].toArray();
  for(const auto& cast : castList) {
    const auto castObj = cast.toObject();
    actors << objValue(castObj, "name") + FieldFormat::columnDelimiterString() + objValue(castObj, "character");
    if(actors.count() >= m_numCast) {
      break;
    }
  }
  entry_->setField(QStringLiteral("cast"), actors.join(FieldFormat::rowDelimiterString()));

  entry_->setField(QStringLiteral("studio"), objValue(obj_, "production_companies", "name"));

  QStringList countries;
  auto countryList = obj_.value(QLatin1StringView("production_countries")).toArray();
  if(countryList.isEmpty()) {
    countryList = obj_.value(QLatin1StringView("origin_country")).toArray();
  }
  for(const auto& country : std::as_const(countryList)) {
    QString name = objValue(country.toObject(), "name");
    if(name == QLatin1String("United States of America") || name == QLatin1String("US")) {
      name = QStringLiteral("USA");
    }
    if(!name.isEmpty()) countries << name;
  }
  entry_->setField(QStringLiteral("nationality"), countries.join(FieldFormat::delimiterString()));

  entry_->setField(QStringLiteral("genre"), objValue(obj_, "genres", "name"));

  // hard-coded poster size for now
  const QString cover = m_imageBase + QLatin1String("w342") + objValue(obj_, "poster_path");
  entry_->setField(QStringLiteral("cover"), cover);

  entry_->setField(QStringLiteral("running-time"), objValue(obj_, "runtime"));
  QString lang = objValue(obj_, "original_language");
  const QString langName = KLanguageName::nameForCode(lang);
  if(!langName.isEmpty()) lang = langName;
  if(lang == QLatin1String("US English")) lang = QLatin1String("English");
  entry_->setField(QStringLiteral("language"), lang);
  entry_->setField(QStringLiteral("plot"), objValue(obj_, "overview"));
}

void TheMovieDBFetcher::readConfiguration() {
  QUrl u(QString::fromLatin1(THEMOVIEDB_API_URL));
  u.setPath(QStringLiteral("/%1/configuration").arg(QLatin1String(THEMOVIEDB_API_VERSION)));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("api_key"), m_apiKey);
  u.setQuery(q);

  QByteArray data = FileHandler::readDataFile(u, true);
#if 0
  myWarning() << "Remove debug3 from themoviedbfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test3.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  const auto obj = doc.object();

  m_imageBase = objValue(obj, "images", "base_url");
  m_serverConfigDate = QDate::currentDate();
}

Tellico::Fetch::ConfigWidget* TheMovieDBFetcher::configWidget(QWidget* parent_) const {
  return new TheMovieDBFetcher::ConfigWidget(parent_, this);
}

QString TheMovieDBFetcher::defaultName() {
  return QStringLiteral("The Movie DB (TMDb)");
}

QString TheMovieDBFetcher::defaultIcon() {
  return favIcon("https://www.themoviedb.org");
}

Tellico::StringHash TheMovieDBFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("tmdb")] = i18n("TMDb Link");
  hash[QStringLiteral("imdb")] = i18n("IMDb Link");
  hash[QStringLiteral("alttitle")] = i18n("Alternative Titles");
  hash[QStringLiteral("origtitle")] = i18n("Original Title");
  hash[QStringLiteral("network")] = i18n("Network");
  hash[QStringLiteral("episode")] = i18n("Episodes");
  return hash;
}

TheMovieDBFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const TheMovieDBFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* al = new QLabel(i18n("Registration is required for accessing this data source. "
                               "If you agree to the terms and conditions, <a href='%1'>sign "
                               "up for an account</a>, and enter your information below.",
                                QLatin1String("http://api.themoviedb.org")),
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

  label = new QLabel(i18n("&Maximum cast: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_numCast = new QSpinBox(optionsWidget());
  m_numCast->setMaximum(99);
  m_numCast->setMinimum(0);
  m_numCast->setValue(THEMOVIEDB_DEFAULT_CAST_SIZE);
  void (QSpinBox::* textChanged)(const QString&) = &QSpinBox::textChanged;
  connect(m_numCast, textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_numCast, row, 1);
  w = i18n("The list of cast members may include many people. Set the maximum number returned from the search.");
  label->setWhatsThis(w);
  m_numCast->setWhatsThis(w);
  label->setBuddy(m_numCast);

  label = new QLabel(i18n("Language: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_langCombo = new GUI::ComboBox(optionsWidget());
  // check https://www.themoviedb.org/contribute occasionally for top languages
#define LANG_ITEM(NAME, CY, ISO) \
  m_langCombo->addItem(QIcon(new KCountryFlagEmojiIconEngine(QLatin1String(CY))), \
                       KLanguageName::nameForCode(QLatin1String(ISO)),            \
                       QLatin1String(ISO));
  LANG_ITEM("Chinese", "cn", "zh");
  LANG_ITEM("English", "us", "en");
  LANG_ITEM("French",  "fr", "fr");
  LANG_ITEM("German",  "de", "de");
  LANG_ITEM("Spanish", "es", "es");
  LANG_ITEM("Russian", "ru", "ru");
#undef LANG_ITEM
  m_langCombo->setEditable(true);
  m_langCombo->setCurrentData(QLatin1String("en"));
  void (GUI::ComboBox::* activatedInt)(int) = &GUI::ComboBox::activated;
  connect(m_langCombo, activatedInt, this, &ConfigWidget::slotSetModified);
  connect(m_langCombo, activatedInt, this, &ConfigWidget::slotLangChanged);
  l->addWidget(m_langCombo, row, 1);
  label->setBuddy(m_langCombo);

  l->setRowStretch(++row, 10);

  // now add additional fields widget
  addFieldsWidget(TheMovieDBFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    // only show the key if it is not the default Tellico one...
    // that way the user is prompted to apply for their own
    if(fetcher_->m_apiKey != QLatin1String(THEMOVIEDB_API_KEY)) {
      m_apiKeyEdit->setText(fetcher_->m_apiKey);
    }
    if(m_langCombo->findData(fetcher_->m_locale) == -1) {
      m_langCombo->addItem(fetcher_->m_locale, fetcher_->m_locale);
    }
    m_langCombo->setCurrentData(fetcher_->m_locale);
    m_numCast->setValue(fetcher_->m_numCast);
  }
}

void TheMovieDBFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
  QString lang = m_langCombo->currentData().toString();
  if(lang.isEmpty()) {
    // user-entered format will not have data set for the item. Just use the text itself
    lang = m_langCombo->currentText().trimmed();
  }
  config_.writeEntry("Locale", lang);
  config_.writeEntry("Max Cast", m_numCast->value());
}

QString TheMovieDBFetcher::ConfigWidget::preferredName() const {
  return i18n("TheMovieDB (%1)", m_langCombo->currentText());
}

void TheMovieDBFetcher::ConfigWidget::slotLangChanged() {
  Q_EMIT signalName(preferredName());
}
