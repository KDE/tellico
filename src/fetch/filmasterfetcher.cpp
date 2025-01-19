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

#include "filmasterfetcher.h"
#include "../collections/videocollection.h"
#include "../images/imagefactory.h"
#include "../utils/guiproxy.h"
#include "../utils/mapvalue.h"
#include "../entry.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KIO/JobUiDelegate>
#include <KJobWidgets>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

namespace {
  static const char* FILMASTER_API_URL = "http://api.filmaster.com";
  static const char* FILMASTER_QUERY_URL = "http://api.filmaster.com/1.1/search/";
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

bool FilmasterFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == Person || k == Keyword;
}

bool FilmasterFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

void FilmasterFetcher::readConfigHook(const KConfigGroup&) {
}

void FilmasterFetcher::search() {
  m_started = true;

  QUrl u(QString::fromLatin1(FILMASTER_QUERY_URL));

  switch(request().key()) {
    case Title:
      u.setPath(u.path() + QLatin1String("film/"));
      break;

    case Person:
      u.setPath(u.path() + QLatin1String("person/"));
      break;

    case Keyword:
      break;

    default:
      stop();
      return;
  }
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("phrase"), request().value());
  u.setQuery(q);

//  myDebug() << "url:" << u;

  QPointer<KIO::StoredTransferJob> job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  connect(job.data(), &KJob::result, this, &FilmasterFetcher::slotComplete);
}

void FilmasterFetcher::stop() {
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

Tellico::Data::EntryPtr FilmasterFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  const QString image = entry->field(QStringLiteral("cover"));
  if(image.contains(QLatin1Char('/'))) {
    QUrl imageUrl;
    if(image.startsWith(QLatin1String("http"))) {
      imageUrl = QUrl(image);
    } else if(image.startsWith(QLatin1String("//"))) {
      imageUrl = QUrl(QLatin1String("http:") + image);
    } else {
      imageUrl = QUrl(QString::fromLatin1(FILMASTER_API_URL));
      imageUrl.setPath(imageUrl.path() + image);
    }
    const QString id = ImageFactory::addImage(imageUrl, true);
    if(id.isEmpty()) {
      myDebug() << "Failed to load" << imageUrl;
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QStringLiteral("cover"), id);
  }
  return entry;
}

Tellico::Fetch::FetchRequest FilmasterFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

void FilmasterFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);
//  myDebug();

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
  myWarning() << "Remove debug from filmasterfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  QVariantMap resultsMap = doc.object().toVariantMap();
  QVariantList resultList;
  switch(request().key()) {
    case Title:
      resultList = resultsMap.value(QStringLiteral("best_results")).toList()
                 + resultsMap.value(QStringLiteral("results")).toList();
      break;

    case Person:
      {
        const QVariantList personList = resultsMap.value(QStringLiteral("best_results")).toList();
        QStringList uris;
        foreach(const QVariant& person, personList) {
          const QVariantMap personMap = person.toMap();
          uris << mapValue(personMap, "films_played_uri");
          uris << mapValue(personMap, "films_directed_uri");
        }
        foreach(const QString& uri, uris) {
          QUrl u(QString::fromLatin1(FILMASTER_API_URL));
          u.setPath(uri);
          QString output = FileHandler::readTextFile(u, false /*quiet*/, true /*utf8*/);
          QJsonDocument doc2 = QJsonDocument::fromJson(output.toUtf8());
          resultList += doc2.object().toVariantMap().value(QStringLiteral("objects")).toList();
        }
      }
      break;

    case Keyword:
      resultList = resultsMap.value(QStringLiteral("films")).toMap().value(QStringLiteral("best_results")).toList();
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
  if(!coll->hasField(QStringLiteral("filmaster")) && optionalFields().contains(QStringLiteral("filmaster"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("filmaster"), i18n("Filmaster Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }

  foreach(const QVariant& result, resultList) {
//    myDebug() << "found result:" << result;
    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toMap());

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    Q_EMIT signalResultFound(r);
  }

//  m_start = m_entries.count();
//  m_hasMoreResults = m_start <= m_total;
  m_hasMoreResults = false; // for now, no continued searches
  stop();
}

void FilmasterFetcher::populateEntry(Data::EntryPtr entry_, const QVariantMap& result_) {
  entry_->setField(QStringLiteral("title"), mapValue(result_, "title"));
  entry_->setField(QStringLiteral("year"), mapValue(result_, "release_year"));
  entry_->setField(QStringLiteral("genre"), mapValue(result_, "tags"));
  entry_->setField(QStringLiteral("nationality"), mapValue(result_, "production_country_list"));
  entry_->setField(QStringLiteral("cover"), mapValue(result_, "image"));
  entry_->setField(QStringLiteral("plot"), mapValue(result_, "description"));

  QStringList directors;
  foreach(const QVariant& director, result_.value(QLatin1String("directors")).toList()) {
    const QVariantMap directorMap = director.toMap();
    directors << mapValue(directorMap, "name") + QLatin1Char(' ') + mapValue(directorMap, "surname");
  }
  if(!directors.isEmpty()) {
    entry_->setField(QStringLiteral("director"), directors.join(FieldFormat::delimiterString()));
  }

  const QString castUri = mapValue(result_, "characters_uri");
  if(!castUri.isEmpty()) {
    QUrl u(QString::fromLatin1(FILMASTER_API_URL));
    u.setPath(castUri);
    QString output = FileHandler::readTextFile(u, false /*quiet*/, true /*utf8*/);
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    QVariantList castList = doc.object().toVariantMap().value(QStringLiteral("objects")).toList();
    QStringList castLines;
    foreach(const QVariant& castResult, castList) {
      const QVariantMap castMap = castResult.toMap();
      const QVariantMap nameMap = castMap.value(QStringLiteral("person")).toMap();
      castLines << mapValue(nameMap, "name") + QLatin1Char(' ') + mapValue(nameMap, "surname")
                 + FieldFormat::columnDelimiterString()
                 + mapValue(castMap, "character");
    }
    if(!castLines.isEmpty()) {
      entry_->setField(QStringLiteral("cast"), castLines.join(FieldFormat::rowDelimiterString()));
    }
  }

  if(optionalFields().contains(QStringLiteral("filmaster"))) {
    entry_->setField(QStringLiteral("filmaster"), QLatin1String("http://filmaster.com/film/") + mapValue(result_, "permalink"));
  }
}

Tellico::Fetch::ConfigWidget* FilmasterFetcher::configWidget(QWidget* parent_) const {
  return new FilmasterFetcher::ConfigWidget(parent_, this);
}

QString FilmasterFetcher::defaultName() {
  return QStringLiteral("Filmaster"); // no translation
}

QString FilmasterFetcher::defaultIcon() {
  return favIcon("https://filmaster.com/static/favicon.ico");
}

Tellico::StringHash FilmasterFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("filmaster")] = i18n("Filmaster Link");
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
