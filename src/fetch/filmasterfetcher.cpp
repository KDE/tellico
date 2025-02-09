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
#include "../utils/objvalue.h"
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
using namespace Qt::Literals::StringLiterals;

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
  const auto result = doc.object();
  QJsonArray resultList;
  switch(request().key()) {
    case Title:
      resultList = result["best_results"_L1].toArray();
      {
        const auto list = result["results"_L1].toArray();
        // can't use operator+ since it converts to a QJsonValue
        for(const auto& v : list) resultList.append(v);
      }
      break;

    case Person:
      {
        const auto personList = result["best_results"_L1].toArray();
        QStringList uris;
        for(const auto& person : personList) {
          const auto personObj = person.toObject();
          uris << objValue(personObj, "films_played_uri");
          uris << objValue(personObj, "films_directed_uri");
        }
        for(const QString& uri : std::as_const(uris)) {
          QUrl u(QString::fromLatin1(FILMASTER_API_URL));
          u.setPath(uri);
          QString output = FileHandler::readTextFile(u, false /*quiet*/, true /*utf8*/);
          QJsonDocument doc2 = QJsonDocument::fromJson(output.toUtf8());
          const auto list = doc2.object().value("objects"_L1).toArray();
          for(const auto& v : list) resultList.append(v);
        }
      }
      break;

    case Keyword:
      resultList = result["films"_L1]["best_results"_L1].toArray();
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

  for(const auto& result : std::as_const(resultList)) {
//    myDebug() << "found result:" << result["title"_L1];
    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toObject());

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    Q_EMIT signalResultFound(r);
  }

//  m_start = m_entries.count();
//  m_hasMoreResults = m_start <= m_total;
  m_hasMoreResults = false; // for now, no continued searches
  stop();
}

void FilmasterFetcher::populateEntry(Data::EntryPtr entry_, const QJsonObject& obj_) {
  entry_->setField(QStringLiteral("title"), objValue(obj_, "title"));
  entry_->setField(QStringLiteral("year"), objValue(obj_, "release_year"));
  entry_->setField(QStringLiteral("genre"), objValue(obj_, "tags"));
  entry_->setField(QStringLiteral("nationality"), objValue(obj_, "production_country_list"));
  entry_->setField(QStringLiteral("cover"), objValue(obj_, "image"));
  entry_->setField(QStringLiteral("plot"), objValue(obj_, "description"));

  QStringList directors;
  const auto directorList = obj_["directors"_L1].toArray();
  for(const auto& director : directorList) {
    const auto directorObj = director.toObject();
    directors << objValue(directorObj, "name") + QLatin1Char(' ') + objValue(directorObj, "surname");
  }
  if(!directors.isEmpty()) {
    entry_->setField(QStringLiteral("director"), directors.join(FieldFormat::delimiterString()));
  }

  const QString castUri = objValue(obj_, "characters_uri");
  if(!castUri.isEmpty()) {
    QUrl u(QString::fromLatin1(FILMASTER_API_URL));
    u.setPath(castUri);
    QString output = FileHandler::readTextFile(u, false /*quiet*/, true /*utf8*/);
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    const auto castList = doc.object().value("objects"_L1).toArray();
    QStringList castLines;
    for(const auto& castResult : castList) {
      const auto castObj = castResult.toObject();
      const auto nameObj = castObj.value("person"_L1).toObject();
      castLines << objValue(nameObj, "name") + QLatin1Char(' ') + objValue(nameObj, "surname")
                 + FieldFormat::columnDelimiterString()
                 + objValue(castObj, "character");
    }
    if(!castLines.isEmpty()) {
      entry_->setField(QStringLiteral("cast"), castLines.join(FieldFormat::rowDelimiterString()));
    }
  }

  if(optionalFields().contains(QStringLiteral("filmaster"))) {
    entry_->setField(QStringLiteral("filmaster"), QLatin1String("http://filmaster.com/film/") + objValue(obj_, "permalink"));
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
