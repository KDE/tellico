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

#include <config.h>
#include "moviemeterfetcher.h"
#include "../collections/videocollection.h"
#include "../gui/guiproxy.h"
#include "../core/filehandler.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"

#include <KLocale>
#include <kio/job.h>
#include <kio/jobuidelegate.h>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QTextCodec>

#ifdef HAVE_QJSON
#include <qjson/serializer.h>
#include <qjson/parser.h>
#include <KJobWidgets/KJobWidgets>
#endif

namespace {
  static const char* MOVIEMETER_API_KEY = "t80a06uf736d0yd00jpynpdsgea255yk";
  static const char* MOVIEMETER_API_URL = "http://www.moviemeter.nl/api/film/";
}

using namespace Tellico;
using Tellico::Fetch::MovieMeterFetcher;

MovieMeterFetcher::MovieMeterFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false) {
}

MovieMeterFetcher::~MovieMeterFetcher() {
}

QString MovieMeterFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString MovieMeterFetcher::attribution() const {
  return QLatin1String("<a href=\"http://www.moviemeter.nl\">MovieMeter</a>");
}

bool MovieMeterFetcher::canSearch(FetchKey k) const {
#ifdef HAVE_QJSON
  return k == Keyword;
#else
  return false;
#endif
}

bool MovieMeterFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

void MovieMeterFetcher::readConfigHook(const KConfigGroup&) {
}

void MovieMeterFetcher::search() {
#ifdef HAVE_QJSON
  m_started = true;

  KUrl u(MOVIEMETER_API_URL);
  u.addQueryItem(QLatin1String("api_key"), QLatin1String(MOVIEMETER_API_KEY));

  switch(request().key) {
    case Keyword:
      u.addQueryItem(QLatin1String("q"), request().value);
      //u.addQueryItem(QLatin1String("type"), QLatin1String("all"));
      break;

    case Raw:
      u.setEncodedQuery(request().value.toUtf8());
      break;

    default:
      myWarning() << "key not recognized:" << request().key;
      stop();
      return;
  }

//  myDebug() << "url: " << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
#else
  stop();
#endif
}

void MovieMeterFetcher::stop() {
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

Tellico::Data::EntryPtr MovieMeterFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

#ifdef HAVE_QJSON
  QString id = entry->field(QLatin1String("moviemeter-id"));
  if(!id.isEmpty()) {
    KUrl u(MOVIEMETER_API_URL);
    u.addPath(id);
    u.addQueryItem(QLatin1String("api_key"), QLatin1String(MOVIEMETER_API_KEY));
    // quiet
    QByteArray data = FileHandler::readDataFile(u, true);

#if 0
    myWarning() << "Remove debug2 from moviemeterfetcher.cpp";
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

  // don't want to include ID field
  entry->setField(QLatin1String("moviemeter-id"), QString());

  return entry;
}

Tellico::Fetch::FetchRequest MovieMeterFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Keyword, title);
  }
  return FetchRequest();
}

void MovieMeterFetcher::slotComplete(KJob* job_) {
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
  myWarning() << "Remove debug from moviemeterfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  Data::CollPtr coll(new Data::VideoCollection(true));
  // always add ID for fetchEntryHook
  Data::FieldPtr field(new Data::Field(QLatin1String("moviemeter-id"), QLatin1String("MovieMeter ID"), Data::Field::Line));
  field->setCategory(i18n("General"));
  coll->addField(field);

  if(optionalFields().contains(QLatin1String("moviemeter"))) {
    Data::FieldPtr field(new Data::Field(QLatin1String("moviemeter"), i18n("MovieMeter Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  if(optionalFields().contains(QLatin1String("alttitle"))) {
    Data::FieldPtr field(new Data::Field(QLatin1String("alttitle"), i18n("Alternative Titles"), Data::Field::Table));
    field->setFormatType(FieldFormat::FormatTitle);
    coll->addField(field);
  }

  QJson::Parser parser;
  const QVariant result = parser.parse(data);

  foreach(const QVariant& result, result.toList()) {
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

void MovieMeterFetcher::populateEntry(Data::EntryPtr entry_, const QVariantMap& resultMap_, bool fullData_) {
  entry_->setField(QLatin1String("moviemeter-id"), value(resultMap_, "id"));
  entry_->setField(QLatin1String("title"), value(resultMap_, "title"));
  entry_->setField(QLatin1String("year"),  value(resultMap_, "year"));

  // if we only need cursory data, then we're done
  if(!fullData_) {
    return;
  }

  entry_->setField(QLatin1String("genre"),  value(resultMap_, "genres"));
  entry_->setField(QLatin1String("plot"),  value(resultMap_, "plot"));
  entry_->setField(QLatin1String("running-time"),  value(resultMap_, "duration"));
  entry_->setField(QLatin1String("director"),  value(resultMap_, "directors"));
  entry_->setField(QLatin1String("nationality"),  value(resultMap_, "countries"));

  QStringList castList;
  foreach(const QVariant& actor, resultMap_.value(QLatin1String("actors")).toList()) {
    castList << value(actor.toMap(), "name");
  }
  entry_->setField(QLatin1String("cast"), castList.join(FieldFormat::rowDelimiterString()));


  if(entry_->collection()->hasField(QLatin1String("moviemeter"))) {
    entry_->setField(QLatin1String("moviemeter"), value(resultMap_, "url"));
  }

  if(entry_->collection()->hasField(QLatin1String("alttitle"))) {
    entry_->setField(QLatin1String("alttitle"), value(resultMap_, "alternative_title"));
  }

  entry_->setField(QLatin1String("cover"), value(resultMap_.value(QLatin1String("posters")).toMap(), "small"));
}

Tellico::Fetch::ConfigWidget* MovieMeterFetcher::configWidget(QWidget* parent_) const {
  return new MovieMeterFetcher::ConfigWidget(parent_, this);
}

QString MovieMeterFetcher::defaultName() {
  return QLatin1String("MovieMeter"); // no translation
}

QString MovieMeterFetcher::defaultIcon() {
  return favIcon("http://www.moviemeter.nl");
}

Tellico::StringHash MovieMeterFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("moviemeter")] = i18n("MovieMeter Link");
  hash[QLatin1String("alttitle")]   = i18n("Alternative Titles");
  return hash;
}

MovieMeterFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const MovieMeterFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(MovieMeterFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void MovieMeterFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString MovieMeterFetcher::ConfigWidget::preferredName() const {
  return MovieMeterFetcher::defaultName();
}

// static
QString MovieMeterFetcher::value(const QVariantMap& map, const char* name) {
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

