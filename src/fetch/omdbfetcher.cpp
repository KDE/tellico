/***************************************************************************
    Copyright (C) 2017 Robby Stephenson <robby@periapsis.org>
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

#include "omdbfetcher.h"
#include "../collections/videocollection.h"
#include "../images/imagefactory.h"
#include "../utils/guiproxy.h"
#include "../core/filehandler.h"
#include "../utils/mapvalue.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KJob>
#include <KJobUiDelegate>
#include <KJobWidgets>
#include <KIO/StoredTransferJob>

#include <QUrl>
#include <QLabel>
#include <QLineEdit>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

namespace {
  static const int OMDB_MAX_RETURNS_TOTAL = 20;
  static const char* OMDB_API_URL = "https://www.omdbapi.com";
}

using namespace Tellico;
using Tellico::Fetch::OMDBFetcher;

OMDBFetcher::OMDBFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false) {
  //  setLimit(OMDB_MAX_RETURNS_TOTAL);
}

OMDBFetcher::~OMDBFetcher() {
}

QString OMDBFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool OMDBFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title;
}

bool OMDBFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

void OMDBFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key", QString());
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
}

void OMDBFetcher::saveConfigHook(KConfigGroup&) {
}

void OMDBFetcher::search() {
  continueSearch();
}

void OMDBFetcher::continueSearch() {
  m_started = true;

  QUrl u(QString::fromLatin1(OMDB_API_URL));
  QUrlQuery q;
  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("type"), QStringLiteral("movie"));
      q.addQueryItem(QStringLiteral("r"), QStringLiteral("json"));
      q.addQueryItem(QStringLiteral("s"), request().value());
      break;

    case Raw:
      q.setQuery(request().value());
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }

  if(m_apiKey.isEmpty()) {
    myDebug() << source() << "- No API key";
    message(i18n("An access key is required to use this data source.")
            + QLatin1Char(' ') +
            i18n("Those values must be entered in the data source settings."), MessageHandler::Error);
    stop();
    return;
  }

  q.addQueryItem(QStringLiteral("apikey"), m_apiKey);
  u.setQuery(q);
//  myDebug() << u;

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result, this, &OMDBFetcher::slotComplete);
}

void OMDBFetcher::stop() {
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

Tellico::Data::EntryPtr OMDBFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  QString id = entry->field(QStringLiteral("imdb-id"));
  if(!id.isEmpty()) {
    // quiet
    QUrl u(QString::fromLatin1(OMDB_API_URL));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("type"), QStringLiteral("movie"));
    q.addQueryItem(QStringLiteral("r"), QStringLiteral("json"));
    q.addQueryItem(QStringLiteral("i"), id);
    q.addQueryItem(QStringLiteral("apikey"), m_apiKey);
    u.setQuery(q);
    QByteArray data = FileHandler::readDataFile(u, true);
#if 0
    myWarning() << "Remove debug2 from omdbfetcher.cpp";
    QFile f(QString::fromLatin1("/tmp/test2.json"));
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

  // don't want to include IMDb ID field
  entry->setField(QStringLiteral("imdb-id"), QString());

  return entry;
}

Tellico::Fetch::FetchRequest OMDBFetcher::updateRequest(Data::EntryPtr entry_) {
  QString imdb = entry_->field(QStringLiteral("imdb"));
  if(imdb.isEmpty()) {
    imdb = entry_->field(QStringLiteral("imdb-id"));
  }
  if(!imdb.isEmpty()) {
    QRegularExpression ttRx(QStringLiteral("tt\\d+"));
    auto ttMatch = ttRx.match(imdb);
    if(ttMatch.hasMatch()) {
      return FetchRequest(Raw, QStringLiteral("type=movie&r=json&i=") + ttMatch.captured());
    }
  }

  const QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    const QString year = entry_->field(QStringLiteral("year"));
    if(year.isEmpty()) {
      return FetchRequest(Title, title);
    } else {
      return FetchRequest(Raw, QStringLiteral("type=movie&r=json&s=\"%1\"&y=%2").arg(title, year));
    }
  }
  return FetchRequest();
}

void OMDBFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    job->uiDelegate()->showErrorMessage();
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
  m_job = nullptr;

#if 0
  myWarning() << "Remove debug from omdbfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  Data::CollPtr coll(new Data::VideoCollection(true));
  // always add the imdb-id for fetchEntryHook
  Data::FieldPtr field(new Data::Field(QStringLiteral("imdb-id"), QStringLiteral("IMDb ID"), Data::Field::Line));
  field->setCategory(i18n("General"));
  coll->addField(field);

#if 0
  if(optionalFields().contains(QStringLiteral("alttitle"))) {
    Data::FieldPtr field(new Data::Field(QLatin1String("alttitle"), i18n("Alternative Titles"), Data::Field::Table));
    field->setFormatType(FieldFormat::FormatTitle);
    coll->addField(field);
  }
  if(optionalFields().contains(QStringLiteral("origtitle"))) {
    Data::FieldPtr f(new Data::Field(QLatin1String("origtitle"), i18n("Original Title")));
    f->setFormatType(FieldFormat::FormatTitle);
    coll->addField(f);
  }
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  QVariantMap result = doc.object().toVariantMap();

  const bool response = result.value(QStringLiteral("Response")).toBool();
  if(!response) {
    // a lack of results is considered an error
    // don't show a user alert for that
    myDebug() << "Error:" << result.value(QStringLiteral("Error")).toString();
//    message(result.value(QStringLiteral("Error")).toString(), MessageHandler::Error);
    stop();
    return;
  }

  const QString search = QStringLiteral("Search");
  QVariantList resultList = result.value(search).toList();
  if(resultList.isEmpty()) {
    // might be a single result
    if(result.contains(QLatin1String("Title"))) {
      resultList << result;
    } else {
      myDebug() << "no results";
      stop();
      return;
    }
  }

  // if the search was a Raw update, then fully populate it
  // and wipe the imdb-id value
  const bool fullResult = request().key() == Fetch::Raw &&
                          !result.contains(search);

  int count = 0;
  foreach(const QVariant& result, resultList) {
//    myDebug() << "found result:" << result;

    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toMap(), fullResult);
    if(fullResult) {
      // indicate it's already fully populated
      entry->setField(QStringLiteral("imdb-id"), QString());
    }

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
    ++count;
    if(count >= OMDB_MAX_RETURNS_TOTAL) {
      break;
    }
  }

  stop();
}

void OMDBFetcher::populateEntry(Data::EntryPtr entry_, const QVariantMap& resultMap_, bool fullData_) {
  entry_->setField(QStringLiteral("imdb-id"), mapValue(resultMap_, "imdbID"));
  entry_->setField(QStringLiteral("title"), mapValue(resultMap_, "Title"));
  entry_->setField(QStringLiteral("year"),  mapValue(resultMap_, "Year"));

  if(!fullData_) {
    return;
  }

  const QString cert = mapValue(resultMap_, "Rated");
  Data::FieldPtr certField = entry_->collection()->fieldByName(QStringLiteral("certification"));
  if(certField) {
    foreach(const QString& value, certField->allowed()) {
      if(value.startsWith(cert)) {
        entry_->setField(QStringLiteral("certification"), value);
        break;
      }
    }
  }
  static const QRegularExpression nondigitsRx(QLatin1String("[^\\d]"));
  entry_->setField(QStringLiteral("running-time"),
                   mapValue(resultMap_, "Runtime").remove(nondigitsRx));

  const QStringList genres = mapValue(resultMap_, "Genre").split(QStringLiteral(", "));
  entry_->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));

  const QStringList directors = mapValue(resultMap_, "Director").split(QStringLiteral(", "));
  entry_->setField(QStringLiteral("director"), directors.join(FieldFormat::delimiterString()));

  QStringList writers = mapValue(resultMap_, "Writer").split(QStringLiteral(", "));
  // some writers have parentheticals, remove those
  static const QRegularExpression parenRx(QLatin1String("\\s*\\(.+\\)\\s*"));
  entry_->setField(QStringLiteral("writer"),
                   writers.replaceInStrings(parenRx, QString())
                          .join(FieldFormat::delimiterString()));

  const QStringList producers = mapValue(resultMap_, "Producer").split(QStringLiteral(", "));
  entry_->setField(QStringLiteral("producer"), producers.join(FieldFormat::delimiterString()));

  const QStringList actors = mapValue(resultMap_, "Actors").split(QStringLiteral(", "));
  entry_->setField(QStringLiteral("cast"), actors.join(FieldFormat::rowDelimiterString()));

  const QStringList countries = mapValue(resultMap_, "Country").split(QStringLiteral(", "));
  entry_->setField(QStringLiteral("nationality"), countries.join(FieldFormat::delimiterString()));

  const QStringList langs = mapValue(resultMap_, "Language").split(QStringLiteral(", "));
  entry_->setField(QStringLiteral("language"), langs.join(FieldFormat::delimiterString()));

  entry_->setField(QStringLiteral("cover"), mapValue(resultMap_, "Poster"));
  entry_->setField(QStringLiteral("plot"), mapValue(resultMap_, "Plot"));

  const QString imdb(QStringLiteral("imdb"));
  if(optionalFields().contains(imdb)) {
    if(!entry_->collection()->hasField(imdb)) {
      entry_->collection()->addField(Data::Field::createDefaultField(Data::Field::ImdbField));
    }
    entry_->setField(imdb, QLatin1String("https://www.imdb.com/title/")
                                          + entry_->field(QStringLiteral("imdb-id"))
                                          + QLatin1Char('/'));
  }

}

Tellico::Fetch::ConfigWidget* OMDBFetcher::configWidget(QWidget* parent_) const {
  return new OMDBFetcher::ConfigWidget(parent_, this);
}

QString OMDBFetcher::defaultName() {
  return QStringLiteral("The Open Movie Database");
}

QString OMDBFetcher::defaultIcon() {
  return favIcon("http://www.omdbapi.com");
}

Tellico::StringHash OMDBFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("imdb")] = i18n("IMDb Link");
  return hash;
}

OMDBFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const OMDBFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;
  QLabel* al = new QLabel(i18n("Registration is required for accessing this data source. "
                               "If you agree to the terms and conditions, <a href='%1'>sign "
                               "up for an account</a>, and enter your information below.",
                                QLatin1String("http://www.omdbapi.com")),
                          optionsWidget());
  al->setOpenExternalLinks(true);
  al->setWordWrap(true);
  ++row;
  l->addWidget(al, row, 0, 1, 2);
  // richtext gets weird with size
  al->setMinimumWidth(al->sizeHint().width());
  al->setMinimumHeight(al->sizeHint().height());

  QLabel* label = new QLabel(i18n("Access key: "), optionsWidget());
  l->addWidget(label, ++row, 0);

  m_apiKeyEdit = new QLineEdit(optionsWidget());
  connect(m_apiKeyEdit, &QLineEdit::textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_apiKeyEdit, row, 1);
  label->setBuddy(m_apiKeyEdit);

  l->setRowStretch(++row, 10);

  if(fetcher_) {
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
  }

  // now add additional fields widget
  addFieldsWidget(OMDBFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void OMDBFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
}

QString OMDBFetcher::ConfigWidget::preferredName() const {
  return OMDBFetcher::defaultName();
}
