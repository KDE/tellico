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
#include "filmasterfetcher.h"
#include "../collections/videocollection.h"
#include "../images/imagefactory.h"
#include "../gui/guiproxy.h"
#include "../tellico_utils.h"
#include "../entry.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QTextCodec>

#ifdef HAVE_QJSON
#include <qjson/parser.h>
#endif

namespace {
  static const char* FILMASTER_API_URL = "http://api.filmaster.com";
  static const char* FILMASTER_QUERY_URL = "http://api.filmaster.com/1.0/search/";
}

using namespace Tellico;
using Tellico::Fetch::FilmasterFetcher;

FilmasterFetcher::FilmasterFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false) {
}

FilmasterFetcher::~FilmasterFetcher() {
}

QString FilmasterFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString FilmasterFetcher::attribution() const {
  return i18n("This data is licensed under <a href=""%1"">specific terms</a>.",
              QLatin1String("http://filmaster.com/license/"));
}

bool FilmasterFetcher::canSearch(FetchKey k) const {
#ifndef HAVE_QJSON
  return false;
#else
  return k == Title || k == Person || k == Keyword;
#endif
}

bool FilmasterFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

void FilmasterFetcher::readConfigHook(const KConfigGroup&) {
}

void FilmasterFetcher::search() {
  m_started = true;

#ifdef HAVE_QJSON
  KUrl u(FILMASTER_QUERY_URL);

  switch(request().key) {
    case Title:
      u.addPath(QLatin1String("film/"));
      break;

    case Person:
      u.addPath(QLatin1String("person/"));
      break;

    default:
      break;
  }
  u.addQueryItem(QLatin1String("phrase"), request().value);

//  myDebug() << "url:" << u;

  QPointer<KIO::StoredTransferJob> job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  job->ui()->setWindow(GUI::Proxy::widget());
  connect(job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
#else
  stop();
#endif
}

void FilmasterFetcher::stop() {
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

Tellico::Data::EntryPtr FilmasterFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  const QString image = entry->field(QLatin1String("cover"));
  if(image.contains(QLatin1Char('/'))) {
    KUrl imageUrl(FILMASTER_API_URL);
    imageUrl.addPath(image);
    const QString id = ImageFactory::addImage(imageUrl, true);
    if(!id.isEmpty()) {
      entry->setField(QLatin1String("cover"), id);
    }
  }
  return entry;
}

Tellico::Fetch::FetchRequest FilmasterFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

void FilmasterFetcher::slotComplete(KJob* job_) {
#ifndef HAVE_QJSON
  stop();
#else
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);
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
  myWarning() << "Remove debug from filmasterfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  QJson::Parser parser;
  const QVariantMap resultsMap = parser.parse(data).toMap();
  QVariantList resultList;
  switch(request().key) {
    case Title:
      resultList = resultsMap.value(QLatin1String("best_results")).toList()
                 + resultsMap.value(QLatin1String("results")).toList();
      break;

    case Person:
      {
        const QVariantList personList = resultsMap.value(QLatin1String("best_results")).toList();
        QStringList uris;
        foreach(const QVariant& person, personList) {
          const QVariantMap personMap = person.toMap();
          uris << value(personMap, "films_played_uri");
          uris << value(personMap, "films_directed_uri");
        }
        foreach(const QString& uri, uris) {
          KUrl u(FILMASTER_API_URL);
          u.setPath(uri);
          // myDebug() << u;
          QString output = FileHandler::readTextFile(u, false /*quiet*/, true /*utf8*/);
          resultList += parser.parse(output.toUtf8()).toMap().value(QLatin1String("objects")).toList();
        }
      }
      break;

    case Keyword:
      resultList = resultsMap.value(QLatin1String("films")).toMap().value(QLatin1String("best_results")).toList();
      break;

    default:
      break;
  }

  if(resultList.isEmpty()) {
    myDebug() << "no results";
    stop();
    return;
  }

  Data::CollPtr coll(new Data::VideoCollection(true));
  if(!coll->hasField(QLatin1String("filmaster")) && optionalFields().contains(QLatin1String("filmaster"))) {
    Data::FieldPtr field(new Data::Field(QLatin1String("filmaster"), i18n("Filmaster Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }

  foreach(const QVariant& result, resultList) {
  //  myDebug() << "found result:" << result;
    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toMap());

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }

//  m_start = m_entries.count();
//  m_hasMoreResults = m_start <= m_total;
  m_hasMoreResults = false; // for now, no continued searches
  stop();
#endif
}

void FilmasterFetcher::populateEntry(Data::EntryPtr entry_, const QVariantMap& result_) {
#ifdef HAVE_QJSON
  entry_->setField(QLatin1String("title"), value(result_, "title"));
  entry_->setField(QLatin1String("year"), value(result_, "release_year"));
  entry_->setField(QLatin1String("genre"), value(result_, "tags"));
  entry_->setField(QLatin1String("nationality"), value(result_, "production_country_list"));
  entry_->setField(QLatin1String("cover"), value(result_, "image"));
  entry_->setField(QLatin1String("plot"), value(result_, "description"));

  QStringList directors;
  foreach(const QVariant& director, result_.value(QLatin1String("directors")).toList()) {
    const QVariantMap directorMap = director.toMap();
    directors << value(directorMap, "name") + QLatin1Char(' ') + value(directorMap, "surname");
  }
  if(!directors.isEmpty()) {
    entry_->setField(QLatin1String("director"), directors.join(FieldFormat::delimiterString()));
  }

  const QString castUri = value(result_, "characters_uri");
  if(!castUri.isEmpty()) {
    QJson::Parser parser;
    KUrl u(FILMASTER_API_URL);
    u.setPath(castUri);
    QString output = FileHandler::readTextFile(u, false /*quiet*/, true /*utf8*/);
    const QVariantList castList = parser.parse(output.toUtf8()).toMap().value(QLatin1String("objects")).toList();
    QStringList castLines;
    foreach(const QVariant& castResult, castList) {
      const QVariantMap castMap = castResult.toMap();
      const QVariantMap nameMap = castMap.value(QLatin1String("person")).toMap();
      castLines << value(nameMap, "name") + QLatin1Char(' ') + value(nameMap, "surname")
                 + FieldFormat::columnDelimiterString()
                 + value(castMap, "character");
    }
    if(!castLines.isEmpty()) {
      entry_->setField(QLatin1String("cast"), castLines.join(FieldFormat::rowDelimiterString()));
    }
  }

  if(optionalFields().contains(QLatin1String("filmaster"))) {
    entry_->setField(QLatin1String("filmaster"), QLatin1String("http://filmaster.com/film/") + value(result_, "permalink"));
  }
#endif
}

Tellico::Fetch::ConfigWidget* FilmasterFetcher::configWidget(QWidget* parent_) const {
  return new FilmasterFetcher::ConfigWidget(parent_, this);
}

QString FilmasterFetcher::defaultName() {
  return QLatin1String("Filmaster"); // no translation
}

QString FilmasterFetcher::defaultIcon() {
  return favIcon("http://www.filmaster.com");
}

Tellico::StringHash FilmasterFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("filmaster")] = i18n("Filmaster Link");
  return hash;
}

FilmasterFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const FilmasterFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(FilmasterFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void FilmasterFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString FilmasterFetcher::ConfigWidget::preferredName() const {
  return FilmasterFetcher::defaultName();
}

QString FilmasterFetcher::value(const QVariantMap& map, const char* name) {
  const QVariant v = map.value(QLatin1String(name));
  if(v.isNull())  {
    return QString();
  } else if(v.canConvert(QVariant::String)) {
    return v.toString();
  } else if(v.canConvert(QVariant::StringList)) {
    return v.toStringList().join(Tellico::FieldFormat::delimiterString());
  } else if(v.canConvert(QVariant::Map)) {
    return v.toMap().value(QLatin1String("value")).toString();
  } else {
    return QString();
  }
}

#include "filmasterfetcher.moc"
