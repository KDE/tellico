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
#include "../utils/string_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KJob>
#include <KJobUiDelegate>
#include <KJobWidgets/KJobWidgets>
#include <KIO/StoredTransferJob>

#include <QUrl>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QTextCodec>
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

bool OMDBFetcher::canSearch(FetchKey k) const {
  return k == Title;
}

bool OMDBFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

void OMDBFetcher::readConfigHook(const KConfigGroup&) {
}

void OMDBFetcher::saveConfigHook(KConfigGroup&) {
}

void OMDBFetcher::search() {
  continueSearch();
}

void OMDBFetcher::continueSearch() {
  m_started = true;

  QUrl u(QString::fromLatin1(OMDB_API_URL));

  switch(request().key) {
    case Title:
      {
        QUrlQuery q;
        q.addQueryItem(QLatin1String("type"), QLatin1String("movie"));
        q.addQueryItem(QLatin1String("r"), QLatin1String("json"));
        q.addQueryItem(QLatin1String("s"), request().value);
        u.setQuery(q);
      }
      break;

    default:
      myWarning() << "key not recognized:" << request().key;
      stop();
      return;
  }

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
}

void OMDBFetcher::stop() {
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

Tellico::Data::EntryPtr OMDBFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  QString id = entry->field(QLatin1String("imdb-id"));
  if(!id.isEmpty()) {
    // quiet
    QUrl u(QString::fromLatin1(OMDB_API_URL));
    QUrlQuery q;
    q.addQueryItem(QLatin1String("type"), QLatin1String("movie"));
    q.addQueryItem(QLatin1String("r"), QLatin1String("json"));
    q.addQueryItem(QLatin1String("i"), id);
    u.setQuery(q);
    QByteArray data = FileHandler::readDataFile(u, true);
#if 0
    myWarning() << "Remove debug2 from omdbfetcher.cpp";
    QFile f(QString::fromLatin1("/tmp/test2.json"));
    if(f.open(QIODevice::WriteOnly)) {
      QTextStream t(&f);
      t.setCodec("UTF-8");
      t << data;
    }
    f.close();
#endif
    QJsonDocument doc = QJsonDocument::fromJson(data);
    populateEntry(entry, doc.object().toVariantMap(), true);
  }

  // image might still be a URL
  const QString image_id = entry->field(QLatin1String("cover"));
  if(image_id.contains(QLatin1Char('/'))) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(image_id), true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QLatin1String("cover"), id);
  }

  // don't want to include IMDb ID field
  entry->setField(QLatin1String("imdb-id"), QString());

  return entry;
}

Tellico::Fetch::FetchRequest OMDBFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

void OMDBFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

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
  myWarning() << "Remove debug from omdbfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  Data::CollPtr coll(new Data::VideoCollection(true));
  // always add the imdb-id for fetchEntryHook
  Data::FieldPtr field(new Data::Field(QLatin1String("imdb-id"), QLatin1String("IMDb ID"), Data::Field::Line));
  field->setCategory(i18n("General"));
  coll->addField(field);

#if 0
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
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  QVariantMap result = doc.object().toVariantMap();

  QVariantList resultList = result.value(QLatin1String("Search")).toList();
  if(resultList.isEmpty()) {
    myDebug() << "no results";
    stop();
    return;
  }

  foreach(const QVariant& result, resultList) {
//    myDebug() << "found result:" << result;

    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toMap(), false);

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }

  stop();
}

void OMDBFetcher::populateEntry(Data::EntryPtr entry_, const QVariantMap& resultMap_, bool fullData_) {
  entry_->setField(QLatin1String("imdb-id"), value(resultMap_, "imdbID"));
  entry_->setField(QLatin1String("title"), value(resultMap_, "Title"));
  entry_->setField(QLatin1String("year"),  value(resultMap_, "Year"));

  if(!fullData_) {
    return;
  }

  const QString cert = value(resultMap_, "Rated");
  Data::FieldPtr certField = entry_->collection()->fieldByName(QLatin1String("certification"));
  if(certField) {
    foreach(const QString& value, certField->allowed()) {
      if(value.startsWith(cert)) {
        entry_->setField(QLatin1String("certification"), value);
        break;
      }
    }
  }
  entry_->setField(QLatin1String("running-time"), value(resultMap_, "Runtime")
                                                  .remove(QRegExp(QLatin1String("[^\\d]"))));

  const QStringList genres = value(resultMap_, "Genre").split(QLatin1String(", "));
  entry_->setField(QLatin1String("genre"), genres.join(FieldFormat::delimiterString()));

  const QStringList directors = value(resultMap_, "Director").split(QLatin1String(", "));
  entry_->setField(QLatin1String("director"), directors.join(FieldFormat::delimiterString()));

  QStringList writers = value(resultMap_, "Writer").split(QLatin1String(", "));
  // some wrtiers have parentheticals, remove those
  entry_->setField(QLatin1String("writer"), writers
                                           .replaceInStrings(QRegExp(QLatin1String("\\s*\\(.+\\)\\s*")), QString())
                                           .join(FieldFormat::delimiterString()));

  const QStringList producers = value(resultMap_, "Producer").split(QLatin1String(", "));
  entry_->setField(QLatin1String("producer"), producers.join(FieldFormat::delimiterString()));

  const QStringList actors = value(resultMap_, "Actors").split(QLatin1String(", "));
  entry_->setField(QLatin1String("cast"), actors.join(FieldFormat::rowDelimiterString()));

  const QStringList countries = value(resultMap_, "Country").split(QLatin1String(", "));
  entry_->setField(QLatin1String("nationality"), countries.join(FieldFormat::delimiterString()));

  const QStringList langs = value(resultMap_, "Language").split(QLatin1String(", "));
  entry_->setField(QLatin1String("language"), langs.join(FieldFormat::delimiterString()));

  entry_->setField(QLatin1String("cover"), value(resultMap_, "Poster"));
  entry_->setField(QLatin1String("plot"), value(resultMap_, "Plot"));
  
  if(optionalFields().contains(QLatin1String("imdb"))) {
    if(!entry_->collection()->hasField(QLatin1String("imdb"))) {
      Data::FieldPtr field(new Data::Field(QLatin1String("imdb"), i18n("IMDb Link"), Data::Field::URL));
      field->setCategory(i18n("General"));
      entry_->collection()->addField(field);
    }
    entry_->setField(QLatin1String("imdb"), QLatin1String("http://www.imdb.com/title/")
                                          + entry_->field(QLatin1String("imdb-id"))
                                          + QLatin1Char('/'));
  }

}

Tellico::Fetch::ConfigWidget* OMDBFetcher::configWidget(QWidget* parent_) const {
  return new OMDBFetcher::ConfigWidget(parent_, this);
}

QString OMDBFetcher::defaultName() {
  return QLatin1String("The Open Movie Database");
}

QString OMDBFetcher::defaultIcon() {
  return favIcon("http://www.omdbapi.com");
}

Tellico::StringHash OMDBFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("imdb")] = i18n("IMDb Link");
  return hash;
}

OMDBFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const OMDBFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(OMDBFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void OMDBFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString OMDBFetcher::ConfigWidget::preferredName() const {
  return OMDBFetcher::defaultName();
}

// static
QString OMDBFetcher::value(const QVariantMap& map, const char* name) {
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
