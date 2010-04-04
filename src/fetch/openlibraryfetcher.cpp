/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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
#include "openlibraryfetcher.h"
#include "../collections/bookcollection.h"
#include "../images/imagefactory.h"
#include "../utils/isbnvalidator.h"
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
#include <qjson/serializer.h>
#include <qjson/parser.h>
#endif

namespace {
  // always bibtex
  static const char* OPENLIBRARY_THINGS_URL = "http://openlibrary.org/api/things";
  static const char* OPENLIBRARY_SEARCH_URL = "http://openlibrary.org/api/search";
  static const char* OPENLIBRARY_GET_URL = "http://openlibrary.org/api/get";

  QString value(const QVariantMap& map, const char* name) {
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
}

using namespace Tellico;
using Tellico::Fetch::OpenLibraryFetcher;

OpenLibraryFetcher::OpenLibraryFetcher(QObject* parent_)
    : Fetcher(parent_), m_job(0), m_started(false) {
}

OpenLibraryFetcher::~OpenLibraryFetcher() {
}

QString OpenLibraryFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool OpenLibraryFetcher::canFetch(int type) const {
  return type == Data::Collection::Book;
}

void OpenLibraryFetcher::readConfigHook(const KConfigGroup&) {
}

void OpenLibraryFetcher::search() {
  m_started = true;
  doSearch();
}

void OpenLibraryFetcher::continueSearch() {
  m_started = true;
  doSearch();
}

void OpenLibraryFetcher::doSearch() {
#ifndef HAVE_QJSON
  doCoverOnly();
  return;
#else

  KUrl u(OPENLIBRARY_THINGS_URL);

  QVariantMap query;
  // books are type/edition
  query.insert(QLatin1String("type"), QLatin1String("/type/edition"));
//  query.insert(QLatin1String("sort"), QLatin1String("-publish_date")); // causes performance hit

  switch(request().key) {
    case Title:
      query.insert(QLatin1String("title"), request().value);
      break;

    case Person:
      query.insert(QLatin1String("authors"), getAuthorKeys());
      break;

    case ISBN:
      query.insert(QLatin1String("isbn_10"), ISBNValidator::cleanValue(request().value));
      break;

    case LCCN:
      query.insert(QLatin1String("lccn"), request().value);
      break;

    case Keyword:
      u.setUrl(QLatin1String(OPENLIBRARY_SEARCH_URL));
      query.clear();
      query.insert(QLatin1String("query"), request().value);
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      stop();
      return;
  }

  QJson::Serializer serializer;
  QByteArray json = serializer.serialize(query);
//  myDebug() << json;

  if(request().key  == Keyword) {
    u.addQueryItem(QLatin1String("q"), QString::fromUtf8(json));
  } else {
    u.addQueryItem(QLatin1String("query"), QString::fromUtf8(json));
  }
//  myDebug() << "url:" << u;

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
#endif
}

void OpenLibraryFetcher::doCoverOnly() {
  switch(request().key) {
     case ISBN:
       break;

    default:
      myWarning() << "key not recognized: " << request().key;
      stop();
      return;
  }

  int pos = 0;
  QString isbn = request().value;
  ISBNValidator val(this);
  if(val.validate(isbn, pos) == QValidator::Acceptable) {
    Data::CollPtr coll(new Data::BookCollection(true));
    Data::EntryPtr entry(new Data::Entry(coll));
    entry->setField(QLatin1String("isbn"), isbn);

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, Data::EntryPtr(entry));
    emit signalResultFound(r);
  }

  stop();
}

void OpenLibraryFetcher::stop() {
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

Tellico::Data::EntryPtr OpenLibraryFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  // if the entry is not set, go ahead and try to fetch it
  if(entry->field(QLatin1String("cover")).isEmpty()) {
    const QString isbn = ISBNValidator::cleanValue(entry->field(QLatin1String("isbn")));
    if(!isbn.isEmpty()) {
      KUrl imageUrl = QString::fromLatin1("http://covers.openlibrary.org/b/isbn/%1-M.jpg?default=false").arg(isbn);
      const QString id = ImageFactory::addImage(imageUrl, true);
      if(!id.isEmpty()) {
        entry->setField(QLatin1String("cover"), id);
      }
    }
  }

  return entry;
}

Tellico::Fetch::FetchRequest OpenLibraryFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString isbn = entry_->field(QLatin1String("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }
  return FetchRequest();
}

void OpenLibraryFetcher::slotComplete(KJob*) {
#ifdef HAVE_QJSON
//  myDebug();

  if(m_job->error()) {
    m_job->ui()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  QJson::Parser parser;
  QVariantMap resultMap = parser.parse(data).toMap();
  if(resultMap.value(QLatin1String("status")) != QLatin1String("ok")) {
    myDebug() << "bad status result:" << resultMap.value(QLatin1String("status"));
    stop();
    return;
  }

  QVariantList resultList = resultMap.value(QLatin1String("result")).toList();
  if(resultList.isEmpty()) {
    myDebug() << "no results";
    stop();
    return;
  }

  Data::CollPtr coll(new Data::BookCollection(true));

  foreach(const QVariant& result, resultList) {
  //  myDebug() << "found result:" << result;

    KUrl u(OPENLIBRARY_GET_URL);
    u.addQueryItem(QLatin1String("key"), result.toString());

    QString output = FileHandler::readTextFile(u, false /*quiet*/, true /*utf8*/);
    resultMap = parser.parse(output.toUtf8()).toMap().value(QLatin1String("result")).toMap();

#if 0
  myWarning() << "Remove debug from openlibraryfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec(QTextCodec::codecForName("UTF-8"));
    t << output;
  }
  f.close();
#endif

//  myDebug() << resultMap.value(QLatin1String("isbn_10")).toList().at(0);

    Data::EntryPtr entry(new Data::Entry(coll));

    entry->setField(QLatin1String("title"), value(resultMap, "title"));
    entry->setField(QLatin1String("subtitle"), value(resultMap, "subtitle"));
    entry->setField(QLatin1String("pub_year"), value(resultMap, "publish_date"));
    entry->setField(QLatin1String("isbn"), value(resultMap, "isbn_10"));
    if(entry->field(QLatin1String("isbn")).isEmpty()) {
      entry->setField(QLatin1String("isbn"), value(resultMap, "isbn_13"));
    }
    entry->setField(QLatin1String("lccn"), value(resultMap, "lccn"));
    entry->setField(QLatin1String("genre"), value(resultMap, "genres"));
    entry->setField(QLatin1String("keyword"), value(resultMap, "subjects"));
    entry->setField(QLatin1String("edition"), value(resultMap, "edition_name"));
    QString binding = value(resultMap, "physical_format");
    if(binding.toLower() == QLatin1String("hardcover")) {
      binding = QLatin1String("Hardback");
    } else if(binding.toLower().contains(QLatin1String("paperback"))) {
      binding = QLatin1String("Paperback");
    }
    entry->setField(QLatin1String("binding"), i18n(binding.toUtf8()));
    entry->setField(QLatin1String("publisher"), value(resultMap, "publishers"));
    entry->setField(QLatin1String("series"), value(resultMap, "series"));
    entry->setField(QLatin1String("pages"), value(resultMap, "number_of_pages"));
    entry->setField(QLatin1String("comments"), value(resultMap, "notes"));

    QStringList authors;
    foreach(const QVariant& authorMap, resultMap.value(QLatin1String("authors")).toList()) {
      const QString key = value(authorMap.toMap(), "key");
      if(!key.isEmpty()) {
        KUrl authorUrl(OPENLIBRARY_GET_URL);
        authorUrl.addQueryItem(QLatin1String("key"), key);

        output = FileHandler::readTextFile(authorUrl, false /*quiet*/, true /*utf8*/);
        QVariantMap authorResult = parser.parse(output.toUtf8()).toMap().value(QLatin1String("result")).toMap();
        const QString name = value(authorResult, "name");
        if(!name.isEmpty()) {
          authors << name;
        }
      }
    }
    if(!authors.isEmpty()) {
      entry->setField(QLatin1String("author"), authors.join(FieldFormat::delimiterString()));
    }

    QStringList langs;
    foreach(const QVariant& langMap, resultMap.value(QLatin1String("languages")).toList()) {
      const QString key = value(langMap.toMap(), "key");
      if(!key.isEmpty()) {
        KUrl langUrl(OPENLIBRARY_GET_URL);
        langUrl.addQueryItem(QLatin1String("key"), key);

        output = FileHandler::readTextFile(langUrl, false /*quiet*/, true /*utf8*/);
        QVariantMap langResult = parser.parse(output.toUtf8()).toMap().value(QLatin1String("result")).toMap();
        const QString name = value(langResult, "name");
        if(!name.isEmpty()) {
          langs << i18n(name.toUtf8());
        }
      }
    }
    if(!langs.isEmpty()) {
      entry->setField(QLatin1String("language"), langs.join(FieldFormat::delimiterString()));
    }

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }

//  m_start = m_entries.count();
//  m_hasMoreResults = m_start <= m_total;
  m_hasMoreResults = false; // for now, no continued searches

  stop(); // required
#endif
}

QString OpenLibraryFetcher::getAuthorKeys() {
  KUrl u(OPENLIBRARY_THINGS_URL);

  QVariantMap query;
  query.insert(QLatin1String("type"), QLatin1String("/type/author"));
  query.insert(QLatin1String("name"), request().value);

  QJson::Serializer serializer;
  QByteArray json = serializer.serialize(query);

  u.addQueryItem(QLatin1String("query"), QString::fromUtf8(json));
  QString output = FileHandler::readTextFile(u, false /*quiet*/, true /*utf8*/);
  QJson::Parser parser;
  QVariantList results = parser.parse(output.toUtf8()).toMap().value(QLatin1String("result")).toList();
  // right now, only use the first
  return results.isEmpty() ? QString() : results.at(0).toString();
}

Tellico::Fetch::ConfigWidget* OpenLibraryFetcher::configWidget(QWidget* parent_) const {
  return new OpenLibraryFetcher::ConfigWidget(parent_, this);
}

QString OpenLibraryFetcher::defaultName() {
  return QLatin1String("Open Library"); // no translation
}

QString OpenLibraryFetcher::defaultIcon() {
  return favIcon("http://www.openlibrary.org");
}

OpenLibraryFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const OpenLibraryFetcher*)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

void OpenLibraryFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString OpenLibraryFetcher::ConfigWidget::preferredName() const {
  return OpenLibraryFetcher::defaultName();
}

#include "openlibraryfetcher.moc"
