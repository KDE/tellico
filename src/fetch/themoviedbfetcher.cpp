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

#include <config.h>
#include "themoviedbfetcher.h"
#include "../collections/videocollection.h"
#include "../images/imagefactory.h"
#include "../gui/combobox.h"
#include "../gui/guiproxy.h"
#include "../core/filehandler.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <KConfigGroup>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QTextCodec>

#ifdef HAVE_QJSON
#include <qjson/serializer.h>
#include <qjson/parser.h>
#endif

namespace {
  static const int THEMOVIEDB_MAX_RETURNS_TOTAL = 20;
  static const char* THEMOVIEDB_API_URL = "http://api.themoviedb.org";
  static const char* THEMOVIEDB_API_VERSION = "3";
  static const char* THEMOVIEDB_API_KEY = "919890b4128d33c729dc368209ece555";
}

using namespace Tellico;
using Tellico::Fetch::TheMovieDBFetcher;

TheMovieDBFetcher::TheMovieDBFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false)
    , m_locale(QLatin1String("en"))
    , m_apiKey(QLatin1String(THEMOVIEDB_API_KEY)) {
  //  setLimit(THEMOVIEDB_MAX_RETURNS_TOTAL);
}

TheMovieDBFetcher::~TheMovieDBFetcher() {
}

QString TheMovieDBFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

// https://www.themoviedb.org/about/api-terms
QString TheMovieDBFetcher::attribution() const {
  return QString::fromLatin1("This product uses the TMDb API but is not endorsed or certified by TMDb.");
}

bool TheMovieDBFetcher::canSearch(FetchKey k) const {
#ifdef HAVE_QJSON
  return k == Title;
#else
  return false;
#endif
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
}

void TheMovieDBFetcher::saveConfigHook(KConfigGroup& config_) {
  if(!m_serverConfigDate.isNull()) {
    config_.writeEntry("ServerConfigDate", m_serverConfigDate);
  }
  config_.writeEntry("ImageBase", m_imageBase);
  config_.sync();
}

void TheMovieDBFetcher::search() {
  if(m_imageBase.isEmpty() || m_serverConfigDate.daysTo(QDate::currentDate()) > 7) {
    readConfiguration();
  }
  continueSearch();
}

void TheMovieDBFetcher::continueSearch() {
#ifdef HAVE_QJSON
  m_started = true;

  if(m_apiKey.isEmpty()) {
    myDebug() << "empty API key";
    stop();
    return;
  }

  KUrl u(THEMOVIEDB_API_URL);
  u.setPath(QLatin1String(THEMOVIEDB_API_VERSION));

  switch(request().key) {
    case Title:
      u.addPath(QLatin1String("/search/movie"));
      u.addQueryItem(QLatin1String("api_key"), m_apiKey);
      u.addQueryItem(QLatin1String("language"), m_locale);
      u.addQueryItem(QLatin1String("query"), request().value);
      break;

    default:
      myWarning() << "key not recognized:" << request().key;
      stop();
      return;
  }

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
#else
  stop();
#endif
}

void TheMovieDBFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }
  m_started = false;
  emit signalDone(this);
}

Tellico::Data::EntryPtr TheMovieDBFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

#ifdef HAVE_QJSON
  QString id = entry->field(QLatin1String("tmdb-id"));
  if(!id.isEmpty()) {
    // quiet
    KUrl u(THEMOVIEDB_API_URL);
    u.setPath(QString::fromLatin1("%1/movie/%2")
              .arg(QLatin1String(THEMOVIEDB_API_VERSION), id));
    u.addQueryItem(QLatin1String("api_key"), m_apiKey);
    u.addQueryItem(QLatin1String("language"), m_locale);
    u.addQueryItem(QLatin1String("append_to_response"),
                   QLatin1String("alternative_titles,credits"));
    QByteArray data = FileHandler::readDataFile(u, true);
#if 0
    myWarning() << "Remove debug2 from themoviedbfetcher.cpp";
    QFile f(QString::fromLatin1("/tmp/test2.json"));
    if(f.open(QIODevice::WriteOnly)) {
      QTextStream t(&f);
      t.setCodec("UTF-8");
      t << data;
    }
    f.close();
#endif
    QJson::Parser parser;
    populateEntry(entry, parser.parse(data).toMap(), true);
  }
#endif

  // image might still be a URL
  const QString image_id = entry->field(QLatin1String("cover"));
  if(image_id.contains(QLatin1Char('/'))) {
    const QString id = ImageFactory::addImage(image_id, true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QLatin1String("cover"), id);
  }

  // don't want to include TMDb ID field
  entry->setField(QLatin1String("tmdb-id"), QString());

  return entry;
}

Tellico::Fetch::FetchRequest TheMovieDBFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

void TheMovieDBFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);
#ifdef HAVE_QJSON
//  myDebug();

  if(job->error()) {
    job->ui()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = 0;

#if 0
  myWarning() << "Remove debug from themoviedbfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  Data::CollPtr coll(new Data::VideoCollection(true));
  // always add the tmdb-id for fetchEntryHook
  Data::FieldPtr field(new Data::Field(QLatin1String("tmdb-id"), QLatin1String("TMDb ID"), Data::Field::Line));
  field->setCategory(i18n("General"));
  coll->addField(field);

  if(optionalFields().contains(QLatin1String("tmdb"))) {
    Data::FieldPtr field(new Data::Field(QLatin1String("tmdb"), i18n("TMDb Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  if(optionalFields().contains(QLatin1String("imdb"))) {
    Data::FieldPtr field(new Data::Field(QLatin1String("imdb"), i18n("IMDb Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  if(optionalFields().contains(QLatin1String("alttitle"))) {
    Data::FieldPtr field(new Data::Field(QLatin1String("alttitle"), i18n("Alternative Titles"), Data::Field::Table));
    field->setFormatType(FieldFormat::FormatTitle);
    coll->addField(field);
  }
  if(optionalFields().contains(QLatin1String("origtitle"))) {
    Data::FieldPtr f(new Data::Field(QLatin1String("origtitle"), i18n("Original Title")));
    f->setFormatType(FieldFormat::FormatTitle);
    coll->addField(f);
  }

  QJson::Parser parser;
  QVariantMap result = parser.parse(data).toMap();

  QVariantList resultList = result.value(QLatin1String("results")).toList();
  if(resultList.isEmpty()) {
    myDebug() << "no results";
    stop();
    return;
  }

  foreach(const QVariant& result, resultList) {
  //  myDebug() << "found result:" << result;

    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toMap(), false);

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }

#endif
  stop();
}

void TheMovieDBFetcher::populateEntry(Data::EntryPtr entry_, const QVariantMap& resultMap_, bool fullData_) {
  entry_->setField(QLatin1String("tmdb-id"), value(resultMap_, "id"));
  entry_->setField(QLatin1String("title"), value(resultMap_, "title"));
  entry_->setField(QLatin1String("year"),  value(resultMap_, "release_date").left(4));

  QStringList directors, producers, writers, composers;
  QVariantList crewList = resultMap_.value(QLatin1String("credits")).toMap()
                                    .value(QLatin1String("crew")).toList();
  foreach(const QVariant& crew, crewList) {
    const QVariantMap crewMap = crew.toMap();
    const QString job = value(crewMap, "job");
    if(job == QLatin1String("Director")) {
      directors += value(crewMap, "name");
    } else if(job == QLatin1String("Producer")) {
      producers += value(crewMap, "name");
    } else if(job == QLatin1String("Screenplay")) {
      writers += value(crewMap, "name");
    } else if(job == QLatin1String("Original Music Composer")) {
      composers += value(crewMap, "name");
    }
  }
  entry_->setField(QLatin1String("director"), directors.join(FieldFormat::delimiterString()));
  entry_->setField(QLatin1String("producer"), producers.join(FieldFormat::delimiterString()));
  entry_->setField(QLatin1String("writer"),     writers.join(FieldFormat::delimiterString()));
  entry_->setField(QLatin1String("composer"), composers.join(FieldFormat::delimiterString()));

  // if we only need cursory data, then we're done
  if(!fullData_) {
    return;
  }

  if(entry_->collection()->hasField(QLatin1String("tmdb"))) {
    entry_->setField(QLatin1String("tmdb"), QLatin1String("http://www.themoviedb.org/movie/") + value(resultMap_, "id"));
  }
  if(entry_->collection()->hasField(QLatin1String("imdb"))) {
    entry_->setField(QLatin1String("imdb"), QLatin1String("http://www.imdb.com/title/") + value(resultMap_, "imdb_id"));
  }
  if(entry_->collection()->hasField(QLatin1String("origtitle"))) {
    entry_->setField(QLatin1String("origtitle"), value(resultMap_, "original_title"));
  }
  if(entry_->collection()->hasField(QLatin1String("alttitle"))) {
    QStringList atitles;
    foreach(const QVariant& atitle, resultMap_.value(QLatin1String("alternative_titles")).toMap()
                                               .value(QLatin1String("titles")).toList()) {
      atitles << value(atitle.toMap(), "title");
    }
    entry_->setField(QLatin1String("alttitle"), atitles.join(FieldFormat::rowDelimiterString()));
  }

  QStringList actors;
  QVariantList castList = resultMap_.value(QLatin1String("credits")).toMap()
                                    .value(QLatin1String("cast")).toList();
  foreach(const QVariant& cast, castList) {
    QVariantMap castMap = cast.toMap();
    actors << value(castMap, "name") + FieldFormat::columnDelimiterString() + value(castMap, "character");
  }
  entry_->setField(QLatin1String("cast"), actors.join(FieldFormat::rowDelimiterString()));

  QStringList studios;
  foreach(const QVariant& studio, resultMap_.value(QLatin1String("production_companies")).toList()) {
    studios << value(studio.toMap(), "name");
  }
  entry_->setField(QLatin1String("studio"), studios.join(FieldFormat::delimiterString()));

  QStringList countries;
  foreach(const QVariant& country, resultMap_.value(QLatin1String("production_countries")).toList()) {
    QString name = value(country.toMap(), "name");
    if(name == QLatin1String("United States of America")) {
      name = QLatin1String("USA");
    }
    countries << name;
  }
  entry_->setField(QLatin1String("nationality"), countries.join(FieldFormat::delimiterString()));

  QStringList genres;
  foreach(const QVariant& genre, resultMap_.value(QLatin1String("genres")).toList()) {
    genres << value(genre.toMap(), "name");
  }
  entry_->setField(QLatin1String("genre"), genres.join(FieldFormat::delimiterString()));

  // hard-coded poster size for now
  QString cover = m_imageBase + QLatin1String("w342") + value(resultMap_, "poster_path");
  entry_->setField(QLatin1String("cover"), cover);

  entry_->setField(QLatin1String("running-time"), value(resultMap_, "runtime"));
  entry_->setField(QLatin1String("plot"), value(resultMap_, "overview"));
}

void TheMovieDBFetcher::readConfiguration() {
#ifdef HAVE_QJSON
  KUrl u(THEMOVIEDB_API_URL);
  u.setPath(QString::fromLatin1("%1/configuration").arg(QLatin1String(THEMOVIEDB_API_VERSION)));
  u.addQueryItem(QLatin1String("api_key"), m_apiKey);

  QByteArray data = FileHandler::readDataFile(u, true);
#if 0
  myWarning() << "Remove debug3 from themoviedbfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test3.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  QJson::Parser parser;
  QVariantMap resultMap = parser.parse(data).toMap();

  m_imageBase = value(resultMap.value(QLatin1String("images")).toMap(), "base_url");
  m_serverConfigDate = QDate::currentDate();
#endif
}

Tellico::Fetch::ConfigWidget* TheMovieDBFetcher::configWidget(QWidget* parent_) const {
  return new TheMovieDBFetcher::ConfigWidget(parent_, this);
}

QString TheMovieDBFetcher::defaultName() {
  return QLatin1String("TheMovieDB.org");
}

QString TheMovieDBFetcher::defaultIcon() {
  return favIcon("http://www.themoviedb.org");
}

Tellico::StringHash TheMovieDBFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("tmdb")] = i18n("TMDb Link");
  hash[QLatin1String("imdb")] = i18n("IMDb Link");
  hash[QLatin1String("alttitle")] = i18n("Alternative Titles");
  hash[QLatin1String("origtitle")] = i18n("Original Title");
  return hash;
}

TheMovieDBFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const TheMovieDBFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* al = new QLabel(i18n("Registration is required for accessing the %1 data source. "
                               "If you agree to the terms and conditions, <a href='%2'>sign "
                               "up for an account</a>, and enter your information below.",
                                TheMovieDBFetcher::defaultName(),
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

  m_apiKeyEdit = new KLineEdit(optionsWidget());
  connect(m_apiKeyEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_apiKeyEdit, row, 1);
  QString w = i18n("The default Tellico key may be used, but searching may fail due to reaching access limits.");
  label->setWhatsThis(w);
  m_apiKeyEdit->setWhatsThis(w);
  label->setBuddy(m_apiKeyEdit);

  label = new QLabel(i18n("Language: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_langCombo = new GUI::ComboBox(optionsWidget());
  m_langCombo->addItem(i18nc("Language", "English"), QLatin1String("en"));
  m_langCombo->addItem(i18nc("Language", "French"), QLatin1String("fr"));
  m_langCombo->addItem(i18nc("Language", "German"), QLatin1String("de"));
  m_langCombo->addItem(i18nc("Language", "Spanish"), QLatin1String("es"));
  connect(m_langCombo, SIGNAL(activated(int)), SLOT(slotSetModified()));
  connect(m_langCombo, SIGNAL(activated(int)), SLOT(slotLangChanged()));
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
    m_langCombo->setCurrentData(fetcher_->m_locale);
  }
}

void TheMovieDBFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
  const QString lang = m_langCombo->currentData().toString();
  config_.writeEntry("Locale", lang);
}

QString TheMovieDBFetcher::ConfigWidget::preferredName() const {
  return i18n("TheMovieDB (%1)", m_langCombo->currentText());
}

void TheMovieDBFetcher::ConfigWidget::slotLangChanged() {
  emit signalName(preferredName());
}

// static
QString TheMovieDBFetcher::value(const QVariantMap& map, const char* name) {
  const QVariant v = map.value(QLatin1String(name));
  if(v.isNull())  {
    return QString();
  } else if(v.canConvert(QVariant::String)) {
    return v.toString();
  } else if(v.canConvert(QVariant::StringList)) {
    return v.toStringList().join(Tellico::FieldFormat::delimiterString());
  } else {
    return QString();
  }
}

#include "themoviedbfetcher.moc"
