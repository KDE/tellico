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
#include "freebasefetcher.h"
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
  static const char* FREEBASE_QUERY_URL = "http://api.freebase.com/api/service/mqlread";
  static const char* FREEBASE_IMAGE_URL = "http://www.freebase.com/api/trans/image_thumb";

  QString value(const QVariantMap& map, const char* name) {
    const QVariant v = map.value(QLatin1String(name));
    if(v.isNull())  {
      return QString();
    } else if(v.canConvert(QVariant::String)) {
      return v.toString();
    } else if(v.canConvert(QVariant::StringList)) {
      return v.toStringList().join(Tellico::FieldFormat::delimiterString());
    } else if(v.canConvert(QVariant::Map)) {
      return v.toMap().value(QLatin1String("name")).toString();
    } else {
      return QString();
    }
  }

  QString value(const QVariantMap& map, const char* object, const char* name) {
    const QVariant v = map.value(QLatin1String(object));
    if(v.isNull())  {
      return QString();
    } else if(v.canConvert(QVariant::Map)) {
      return value(v.toMap(), name);
    } else {
      return QString();
    }
  }

}

using namespace Tellico;
using Tellico::Fetch::FreebaseFetcher;

FreebaseFetcher::FreebaseFetcher(QObject* parent_)
    : Fetcher(parent_), m_job(0), m_started(false) {
}

FreebaseFetcher::~FreebaseFetcher() {
}

QString FreebaseFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool FreebaseFetcher::canFetch(int type) const {
  return type == Data::Collection::Book;
}

bool FreebaseFetcher::canSearch(FetchKey k) const {
  return k == ISBN;
}

void FreebaseFetcher::readConfigHook(const KConfigGroup&) {
}

void FreebaseFetcher::search() {
  m_started = true;
  doSearch();
}

void FreebaseFetcher::continueSearch() {
  m_started = true;
  doSearch();
}

void FreebaseFetcher::doSearch() {
#ifndef HAVE_QJSON
  stop();
  return;
#else

  QVariantMap query;
  const int type = collectionType();
  switch(type) {
    case Data::Collection::Book:
    case Data::Collection::Bibtex:
      // null properties for books
      query.insert(QLatin1String("type"), QLatin1String("/book/book_edition"));
//      query.insert(QLatin1String("publisher"), QVariantList());
//      query.insert(QLatin1String("publication_date"), QVariantList());
//      query.insert(QLatin1String("name"), QVariantList());
//      query.insert(QLatin1String("number_of_pages"), QVariantList());
//      query.insert(QLatin1String("LCCN"), QVariantList());
//      query.insert(QLatin1String("binding"), QVariantList());
      break;

    default:
      myWarning() << "collection type not available:" << type;
      stop();
      return;
  }

  // grab all properties at the entity level
  query.insert(QLatin1String("*"), QVariantList());
  switch(request().key) {
    case ISBN:
      {
        QVariantMap isbn_query;
        isbn_query.insert(QLatin1String("name"), ISBNValidator::cleanValue(request().value));
        query.insert(QLatin1String("isbn"), isbn_query);

        QVariantMap book_query;
        book_query.insert(QLatin1String("type"), QLatin1String("/book/book"));
        book_query.insert(QLatin1String("*"), QVariantList());
        query.insert(QLatin1String("book"), book_query);

        QVariantMap work_query;
        work_query.insert(QLatin1String("type"), QLatin1String("/book/written_work"));
        work_query.insert(QLatin1String("*"), QVariantList());
        query.insert(QLatin1String("work:book"), work_query);

        QVariantMap topic_query;
        topic_query.insert(QLatin1String("type"), QLatin1String("/common/topic"));
        topic_query.insert(QLatin1String("*"), QVariantList());
        query.insert(QLatin1String("topic:book"), topic_query);

        QVariantMap page_query;
        page_query.insert(QLatin1String("type"), QLatin1String("/book/pagination"));
        page_query.insert(QLatin1String("numbered_pages"), QVariantList());
        query.insert(QLatin1String("number_of_pages"), page_query);

        QVariantMap image_query;
        image_query.insert(QLatin1String("id"), QVariantList());
        query.insert(QLatin1String("/common/topic/image"), image_query);
      }
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      stop();
      return;
  }

  QVariantMap map;
  map.insert(QLatin1String("query"), query);

  QJson::Serializer serializer;
  QByteArray query_string = serializer.serialize(map);
  myDebug() << "query:" << query_string;

  KUrl u(FREEBASE_QUERY_URL);
  u.addQueryItem(QLatin1String("query"), QString::fromUtf8(query_string));

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
#endif
}

void FreebaseFetcher::stop() {
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

Tellico::Data::EntryPtr FreebaseFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in hash";
    return Data::EntryPtr();
  }

  return entry;
}

Tellico::Fetch::FetchRequest FreebaseFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString isbn = entry_->field(QLatin1String("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }
  const QString lccn = entry_->field(QLatin1String("lccn"));
  if(!lccn.isEmpty()) {
    return FetchRequest(LCCN, lccn);
  }
  const QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

void FreebaseFetcher::slotComplete(KJob*) {
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

#if 1
    myWarning() << "Remove debug from freebasefetcher.cpp";
    QFile f(QString::fromLatin1("/tmp/test.json"));
    if(f.open(QIODevice::WriteOnly)) {
      QTextStream t(&f);
      t.setCodec(QTextCodec::codecForName("UTF-8"));
      t << data;
    }
    f.close();
#endif

  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  QJson::Parser parser;
  QVariant resultThing = parser.parse(data).toMap().value(QLatin1String("result"));
  QVariantList resultList;
  if(resultThing.canConvert(QVariant::List)) {
    resultList = resultThing.toList();
  } else if(!resultThing.isNull()) {
    resultList.append(resultThing.toMap());
  }

  if(resultList.isEmpty()) {
    myDebug() << "no results";
    stop();
    return;
  }

  Data::CollPtr coll(new Data::BookCollection(true));

  QString output;

  QVariantMap resultMap;
  foreach(const QVariant& result, resultList) {
  //  myDebug() << "found result:" << result;
    resultMap = result.toMap();

    Data::EntryPtr entry(new Data::Entry(coll));

    entry->setField(QLatin1String("title"),     value(resultMap, "name"));
    entry->setField(QLatin1String("pub_year"),  value(resultMap, "publication_date"));
    entry->setField(QLatin1String("isbn"),      value(resultMap, "isbn"));
    entry->setField(QLatin1String("publisher"), value(resultMap, "publisher"));
    entry->setField(QLatin1String("pages"),     value(resultMap, "number_of_pages"));
    entry->setField(QLatin1String("lccn"),      value(resultMap, "LCCN"));
    entry->setField(QLatin1String("binding"),   value(resultMap, "binding"));
    entry->setField(QLatin1String("genre"),     value(resultMap, "book", "genre"));
    entry->setField(QLatin1String("author"),    value(resultMap, "work:book", "author"));
    entry->setField(QLatin1String("editor"),    value(resultMap, "work:book", "editor"));
    entry->setField(QLatin1String("cr_year"),   value(resultMap, "work:book", "copyright_date"));
    entry->setField(QLatin1String("series"),    value(resultMap, "work:book", "part_of_series"));
    entry->setField(QLatin1String("language"),  value(resultMap, "work:book", "original_language"));
    entry->setField(QLatin1String("pages"),     value(resultMap, "number_of_pages", "numbered_pages"));

    const QString image_id = value(resultMap, "/common/topic/image", "id");
    if(!image_id.isEmpty()) {
      // let's set max image size to 200x200
      KUrl imageUrl(FREEBASE_IMAGE_URL);
      imageUrl.addPath(image_id);
      imageUrl.addQueryItem(QLatin1String("maxwidth"), QLatin1String("200"));
      imageUrl.addQueryItem(QLatin1String("maxheight"), QLatin1String("200"));
      const QString id = ImageFactory::addImage(imageUrl, true);
      if(id.isEmpty()) {
        message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
      } else { // amazon serves up 1x1 gifs occasionally, but that's caught in the image constructor
        // all relevant collection types have cover fields
        entry->setField(QLatin1String("cover"), id);
      }
   }

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }

//  m_start = m_entries.count();
//  m_hasMoreResults = m_start <= m_total;
  m_hasMoreResults = false; // for now, no continued searches
#endif
  stop(); // required
}

Tellico::Fetch::ConfigWidget* FreebaseFetcher::configWidget(QWidget* parent_) const {
  return new FreebaseFetcher::ConfigWidget(parent_, this);
}

QString FreebaseFetcher::defaultName() {
  return QLatin1String("Freebase"); // no translation
}

QString FreebaseFetcher::defaultIcon() {
  return favIcon("http://www.freebase.com");
}

FreebaseFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const FreebaseFetcher*)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

void FreebaseFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString FreebaseFetcher::ConfigWidget::preferredName() const {
  return FreebaseFetcher::defaultName();
}

#include "freebasefetcher.moc"
