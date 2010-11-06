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
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../utils/isbnvalidator.h"
#include "../utils/lccnvalidator.h"
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
  static const char* FREEBASE_QUERY_URL = "http://api.freebase.com/api/service/mqlread";
  static const char* FREEBASE_IMAGE_URL = "http://api.freebase.com/api/trans/image_thumb";
  static const char* FREEBASE_BLURB_URL = "http://api.freebase.com/api/trans/blurb";
  static const char* FREEBASE_VIEW_URL  = "http://www.freebase.com/view";
  static const int   FREEBASE_RETURNS_PER_REQUEST = 25;

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
    } else if(v.canConvert(QVariant::List)) {
      QVariantList list = v.toList();
      return list.isEmpty() ? QString() : value(list.at(0).toMap(), name);
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
  return type == Data::Collection::Book
         || type == Data::Collection::Bibtex
         || type == Data::Collection::ComicBook
         || type == Data::Collection::Video
         || type == Data::Collection::Album
         || type == Data::Collection::Game
         || type == Data::Collection::BoardGame;
}

bool FreebaseFetcher::canSearch(FetchKey k) const {
  return k == Title || k == Person || k == ISBN || k == LCCN;
}

void FreebaseFetcher::readConfigHook(const KConfigGroup&) {
}

void FreebaseFetcher::search() {
  m_started = true;
  m_cursors.clear();
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

  QVariantList queries;
  switch(collectionType()) {
    case Data::Collection::Book:
    case Data::Collection::Bibtex:
      queries = bookQueries();
      break;

    case Data::Collection::ComicBook:
      queries = comicBookQueries();
      break;

    case Data::Collection::Video:
      queries = movieQueries();
      break;

    case Data::Collection::Album:
      queries = musicQueries();
      break;

    case Data::Collection::Game:
      queries = videoGameQueries();
      break;

    case Data::Collection::BoardGame:
      queries = boardGameQueries();
      break;

    default:
      myWarning() << "collection type not available:" << collectionType();
      break;
  }

  if(queries.isEmpty()) {
    stop();
    return;
  }

  QVariantMap httpQuery;

  // make sure if we have cursors for every query
  while(m_cursors.count() < queries.count()) {
    m_cursors.append(QVariant(true));
  }

  for(int i = 0; i < queries.size(); ++i) {
    // we skip the query if the cursor is valid and == false
    if(m_cursors.at(i).type() == QVariant::Bool && !m_cursors.at(i).toBool()) {
      myDebug() << "skipping query" << i;
      continue;
    }

    QVariantMap query = queries.at(i).toMap();
    Q_ASSERT(!query.isEmpty());

    // grab all properties at the entity level
    query.insert(QLatin1String("*"), QVariantList());
    query.insert(QLatin1String("limit"), FREEBASE_RETURNS_PER_REQUEST);

    // grab id for every query, for image and article
    QVariantMap id_query;
    id_query.insert(QLatin1String("id"), QVariantList());
    id_query.insert(QLatin1String("optional"), QLatin1String("optional"));
    id_query.insert(QLatin1String("limit"), 1);
    query.insert(QLatin1String("/common/topic/image"), id_query);
    query.insert(QLatin1String("/common/topic/article"), id_query);

    QVariantMap envelope;
    envelope.insert(QLatin1String("query"), QVariantList() << query);
    envelope.insert(QLatin1String("cursor"), m_cursors.at(i));
    httpQuery.insert(QString::fromLatin1("q%1").arg(i), envelope);
  }

  QJson::Serializer serializer;
  QByteArray query_string = serializer.serialize(httpQuery);
//  myDebug() << "query:" << query_string;

  KUrl url(FREEBASE_QUERY_URL);
  if(query_string.length() < 2048) {
    url.addQueryItem(QLatin1String("queries"), QString::fromUtf8(query_string));
    m_job = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
  } else {
    query_string.prepend("queries=");
    m_job = KIO::storedHttpPost(query_string, url, KIO::HideProgressInfo);
  }
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

  const QString image_id = entry->field(QLatin1String("cover"));
  // if it's still a freebase id, then it starts with a slash
  if(image_id.startsWith(QLatin1Char('/'))) {
    // let's set max image size to 200x200
    KUrl imageUrl(FREEBASE_IMAGE_URL);
    imageUrl.addPath(image_id);
    imageUrl.addQueryItem(QLatin1String("maxwidth"), QLatin1String("200"));
    imageUrl.addQueryItem(QLatin1String("maxheight"), QLatin1String("200"));
    const QString id = ImageFactory::addImage(imageUrl, true);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
      entry->setField(QLatin1String("cover"), QString());
    } else {
      // all relevant collection types have cover fields
      entry->setField(QLatin1String("cover"), id);
    }
  }

  QString article_field;
  switch(entry->collection()->type()) {
    case Data::Collection::Video:
      article_field = QLatin1String("plot");
      break;
    case Data::Collection::Game:
    case Data::Collection::BoardGame:
      article_field = QLatin1String("description");
      break;
    default:
      break;
  }

  if(!article_field.isEmpty()) {
    const QString article_id = entry->field(article_field);
    // if it's still a freebase id, then it starts with a slash
    if(article_id.startsWith(QLatin1Char('/'))) {
      KUrl articleUrl(FREEBASE_BLURB_URL);
      articleUrl.addPath(article_id);
      articleUrl.addQueryItem(QLatin1String("maxlength"), QLatin1String("1000"));
      articleUrl.addQueryItem(QLatin1String("break_paragraphs"), QLatin1String("true"));
      const QString output = FileHandler::readTextFile(articleUrl, true, true);
      if(output.isEmpty()) {
        entry->setField(article_field, QString());
      } else {
        entry->setField(article_field, output);
      }
    }
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

#if 0
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
  QVariant response = parser.parse(data);
  if(response.isNull()) {
    myDebug() << "no response";
    stop();
    return;
  }

  m_hasMoreResults = false;
  QVariantList resultList;

  const QVariantMap responseMap = response.toMap();
  // check response code to see if everything was ok
  if(value(responseMap, "code") == QLatin1String("/api/status/ok")) {
    // the result objects are in outer envelopes called q0, q1, q3, etc...
    for(int i = 0; i < m_cursors.count(); ++i) {
      QVariant queryResult = responseMap.value(QString::fromLatin1("q%1").arg(i));
      if(!queryResult.isNull()) {
        m_cursors[i] = queryResult.toMap().value(QLatin1String("cursor"));
        // there are more results if the cursor value is a string
        m_hasMoreResults = m_hasMoreResults || m_cursors.at(i).type() == QVariant::String;

        QVariant resultThing = queryResult.toMap().value(QLatin1String("result"));
        if(resultThing.canConvert(QVariant::List)) {
          resultList += resultThing.toList();
        } else if(!resultThing.isNull()) {
          resultList += resultThing.toMap();
        }
      }
    }
  } else {
    // we have an error!!!!!!!!!!!!!
    QString msg = value(responseMap, "message");
    if(msg.isEmpty()) {
      msg = value(responseMap, "messages", "message");
    }
    if(!msg.isEmpty()) {
      myDebug() << "message:" << msg;
    }
  }

  if(resultList.isEmpty()) {
    myDebug() << "no results";
    stop();
    return;
  }

  const int type = collectionType();

  Data::CollPtr coll = CollectionFactory::collection(type, true);
  if(!coll->hasField(QLatin1String("freebase")) && optionalFields().contains(QLatin1String("freebase"))) {
    Data::FieldPtr field(new Data::Field(QLatin1String("freebase"), i18n("Freebase Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }

  QString output;

  foreach(const QVariant& result, resultList) {
  //  myDebug() << "found result:" << result;
    QVariantMap resultMap = result.toMap();

    Data::EntryPtr entry(new Data::Entry(coll));
    entry->setField(QLatin1String("title"), value(resultMap, "name"));
    if(optionalFields().contains(QLatin1String("freebase"))) {
      entry->setField(QLatin1String("freebase"), QLatin1String(FREEBASE_VIEW_URL) + value(resultMap, "id"));
    }

    switch(type) {
      case Data::Collection::Book:
      case Data::Collection::Bibtex:
        entry->setField(QLatin1String("pub_year"),  value(resultMap, "publication_date").left(4));
        entry->setField(QLatin1String("isbn"),      value(resultMap, "isbn", "isbn"));
        entry->setField(QLatin1String("publisher"), value(resultMap, "publisher"));
        entry->setField(QLatin1String("pages"),     value(resultMap, "number_of_pages"));
        entry->setField(QLatin1String("lccn"),      value(resultMap, "LCCN"));
        entry->setField(QLatin1String("author"),    value(resultMap, "work:book", "author"));
        entry->setField(QLatin1String("editor"),    value(resultMap, "work:book", "editor"));
        entry->setField(QLatin1String("cr_year"),   value(resultMap, "work:book", "copyright_date").left(4));
        entry->setField(QLatin1String("series"),    value(resultMap, "work:book", "part_of_series"));
        entry->setField(QLatin1String("language"),  value(resultMap, "work:book", "original_language"));
        entry->setField(QLatin1String("pages"),     value(resultMap, "number_of_pages", "numbered_pages"));
        entry->setField(QLatin1String("genre"), FieldFormat::capitalize(value(resultMap, "book", "genre")));
        {
          QString binding = value(resultMap, "binding");
          if(binding.toLower() == QLatin1String("hardcover")) {
            binding = i18n("Hardback");
          } else {
            binding = FieldFormat::capitalize(binding);
          }
          entry->setField(QLatin1String("binding"),   i18n(binding.toUtf8()));
        }
        break;

      case Data::Collection::ComicBook:
        entry->setField(QLatin1String("writer"),     value(resultMap, "editor"));
        entry->setField(QLatin1String("issue"),      value(resultMap, "issue_number"));
        entry->setField(QLatin1String("pub_year"),   value(resultMap, "date_of_publication").left(4));
        entry->setField(QLatin1String("publisher"),  value(resultMap, "!/comic_books/comic_book_series/issues", "publisher"));
        entry->setField(QLatin1String("series"),     value(resultMap, "!/comic_books/comic_book_series/issues", "name"));
        entry->setField(QLatin1String("genre"),      value(resultMap, "!/comic_books/comic_book_series/issues", "genre"));
        {
          QStringList artists;
          QString colors = value(resultMap, "cover_colors");
          if(!colors.isEmpty()) {
            artists << colors;
          }
          QString inks = value(resultMap, "cover_inks");
          if(!inks.isEmpty())  {
            artists << inks;
          }
          QString letters = value(resultMap, "cover_letters");
          if(!letters.isEmpty()) {
            artists << letters;
          }
          QString pencils = value(resultMap, "cover_pencils");
          if(!pencils.isEmpty()) {
            artists << pencils;
          }
          entry->setField(QLatin1String("artist"),  artists.join(Tellico::FieldFormat::delimiterString()));
        }
        break;

      case Data::Collection::Video:
        {
          entry->setField(QLatin1String("director"),      value(resultMap, "directed_by"));
          entry->setField(QLatin1String("producer"),      value(resultMap, "produced_by"));
          entry->setField(QLatin1String("writer"),        value(resultMap, "written_by"));
          entry->setField(QLatin1String("composer"),      value(resultMap, "music"));
          QString studio = value(resultMap, "production_companies");
          if(studio.isEmpty()) {
            studio = value(resultMap, "distributors", "distributor");
          }
          entry->setField(QLatin1String("studio"),        studio);
          entry->setField(QLatin1String("genre"),         value(resultMap, "genre"));
          entry->setField(QLatin1String("nationality"),   value(resultMap, "country"));
          entry->setField(QLatin1String("keyword"),       value(resultMap, "subjects"));
          // sometimes the rating comes back with multiple values
          const QStringList certs = FieldFormat::splitValue(value(resultMap, "rating"));
          entry->setField(QLatin1String("certification"), certs.isEmpty() ? QString() : certs.first());
          entry->setField(QLatin1String("year"),          value(resultMap, "initial_release_date").left(4));
          entry->setField(QLatin1String("running-time"),  value(resultMap, "runtime", "runtime"));

          QStringList castList;
          const QVariantList castResult = resultMap.value(QLatin1String("starring")).toList();
          foreach(const QVariant& cast, castResult) {
            QVariantMap castMap = cast.toMap();
            if(!castMap.isEmpty()) {
              QVariantList actor = castMap.value(QLatin1String("actor")).toList();
              QVariantList role = castMap.value(QLatin1String("character")).toList();
              if(!actor.isEmpty()) {
                QString performance = actor.at(0).toString();
                if(!role.isEmpty()) {
                  performance += FieldFormat::columnDelimiterString() + role.at(0).toString();
                }
                castList.append(performance);
              }
            }
          }
          entry->setField(QLatin1String("cast"), castList.join(FieldFormat::rowDelimiterString()));
        }
        break;

      case Data::Collection::Album:
        {
          entry->setField(QLatin1String("artist"), value(resultMap, "artist"));
          if(entry->field(QLatin1String("artist")).isEmpty()) {
            entry->setField(QLatin1String("artist"), value(resultMap, "credited_as"));
          }
          entry->setField(QLatin1String("year"),   value(resultMap, "release_date").left(4));
          entry->setField(QLatin1String("label"),  value(resultMap, "label"));
          entry->setField(QLatin1String("genre"),  value(resultMap, "album", "genre"));

          QStringList trackList;
          const QVariantList trackResult = resultMap.value(QLatin1String("track")).toList();
          foreach(const QVariant& track, trackResult) {
            QVariantMap trackMap = track.toMap();
            if(!trackMap.isEmpty()) {
              QVariantList name = trackMap.value(QLatin1String("name")).toList();
              QVariantList artist = trackMap.value(QLatin1String("artist")).toList();
              QVariantList length = trackMap.value(QLatin1String("length")).toList();
              if(!name.isEmpty()) {
                int sec = length.isEmpty() ? 0 : static_cast<int>(length.at(0).toString().toFloat() + 0.5);
                trackList += name.at(0).toString()
                           + FieldFormat::columnDelimiterString()
                           + (artist.isEmpty() ? entry->field(QLatin1String("artist")) : artist.at(0).toString())
                           + FieldFormat::columnDelimiterString()
                           + Tellico::minutes(sec);
              }
            }
          }
          entry->setField(QLatin1String("track"), trackList.join(FieldFormat::rowDelimiterString()));
        }
        break;

      case Data::Collection::Game:
        {
          // video game stuff
          entry->setField(QLatin1String("genre"),     value(resultMap, "cvg_genre"));
          entry->setField(QLatin1String("developer"), value(resultMap, "developer"));
          entry->setField(QLatin1String("publisher"), value(resultMap, "publisher"));
          entry->setField(QLatin1String("year"),      value(resultMap, "release_date").left(4));
          const QStringList platforms = FieldFormat::splitValue(value(resultMap, "platforms"));
          if(!platforms.isEmpty()) {
            // just grab first one
            entry->setField(QLatin1String("platform"), i18n(platforms.at(0).toUtf8()));
          }
        }
        break;

      case Data::Collection::BoardGame:
        {
          // video game stuff
          entry->setField(QLatin1String("genre"),     value(resultMap, "genre"));
          entry->setField(QLatin1String("designer"),  value(resultMap, "designer"));
          entry->setField(QLatin1String("publisher"), value(resultMap, "publisher"));
          entry->setField(QLatin1String("year"),      value(resultMap, "introduced").left(4));
          const int minPlayers = value(resultMap, "number_of_players", "low_value").toInt();
          const int maxPlayers = value(resultMap, "number_of_players", "high_value").toInt();
          if(minPlayers > 0 && maxPlayers > 0) {
            QStringList players;
            for(int i = minPlayers; i <= maxPlayers; ++i) {
              players << QString::number(i);
            }
            entry->setField(QLatin1String("num-player"), players.join(Tellico::FieldFormat::delimiterString()));
          }
        }
        break;

      default:
        break;
    }

    // the the image and article ids. The actual content gets loaded in fetchEntryHook()
    entry->setField(QLatin1String("cover"), value(resultMap, "/common/topic/image", "id"));
    if(type == Data::Collection::Album && entry->field(QLatin1String("cover")).isEmpty()) {
      // musical releases can have the image from the album
      QVariantList albumList = resultMap.value(QLatin1String("album")).toList();
      if(!albumList.isEmpty()) {
        QVariantMap albumMap = albumList.at(0).toMap();
        entry->setField(QLatin1String("cover"), value(albumMap, "/common/topic/image", "id"));
      }
    }
    if(type == Data::Collection::Video) {
      entry->setField(QLatin1String("plot"), value(resultMap, "/common/topic/article", "id"));
    } else if(type == Data::Collection::Game || type == Data::Collection::BoardGame) {
      entry->setField(QLatin1String("description"), value(resultMap, "/common/topic/article", "id"));
    }

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }
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

Tellico::StringHash FreebaseFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("freebase")] = i18n("Freebase Link");
  return hash;
}

QVariantList FreebaseFetcher::bookQueries() const {
  QVariantMap query;
  query.insert(QLatin1String("type"), QLatin1String("/book/book_edition"));

  QVariantMap book_query;
  book_query.insert(QLatin1String("type"), QLatin1String("/book/book"));
  book_query.insert(QLatin1String("*"), QVariantList());
  query.insert(QLatin1String("book"), book_query);

  QVariantMap work_query;
  work_query.insert(QLatin1String("type"), QLatin1String("/book/written_work"));
  work_query.insert(QLatin1String("*"), QVariantList());
  // the author is part of the written_work type
  if(request().key != Person) {
    work_query.insert(QLatin1String("optional"), QLatin1String("optional"));
  }
  query.insert(QLatin1String("work:book"), work_query);

  QVariantMap topic_query;
  topic_query.insert(QLatin1String("type"), QLatin1String("/common/topic"));
  topic_query.insert(QLatin1String("*"), QVariantList());
  topic_query.insert(QLatin1String("optional"), QLatin1String("optional"));
  query.insert(QLatin1String("topic:book"), topic_query);

  QVariantMap page_query;
  page_query.insert(QLatin1String("type"), QLatin1String("/book/pagination"));
  page_query.insert(QLatin1String("numbered_pages"), QVariantList());
  page_query.insert(QLatin1String("optional"), QLatin1String("optional"));
  query.insert(QLatin1String("number_of_pages"), page_query);

  QVariantMap isbn_query;
  isbn_query.insert(QLatin1String("type"), QLatin1String("/book/isbn"));
  isbn_query.insert(QLatin1String("isbn"), QVariantList());
  if(request().key != ISBN) {
    isbn_query.insert(QLatin1String("optional"), QLatin1String("optional"));
  }
  query.insert(QLatin1String("isbn"), isbn_query);

  QVariantList queries;
  switch(request().key) {
    case Title:
      query.insert(QLatin1String("name~="), QLatin1Char('*') + request().value + QLatin1Char('*'));
      queries << query;
      break;

    case Person:
      // for now just check author, and need to use written_work topic_query
      {
        QVariantMap workQuery = query.value(QLatin1String("work:book")).toMap();

        QVariantMap authorSubQuery = workQuery;
        authorSubQuery.insert(QLatin1String("author~="), QLatin1Char('*') + request().value + QLatin1Char('*'));

        QVariantMap editorSubquery = workQuery;
        editorSubquery.insert(QLatin1String("editor~="), QLatin1Char('*') + request().value + QLatin1Char('*'));

        QVariantMap authorQuery = query;
        authorQuery.insert(QLatin1String("work:book"), authorSubQuery);

        QVariantMap editorQuery = query;
        editorQuery.insert(QLatin1String("work:book"), editorSubquery);

        queries << authorQuery << editorQuery;
      }
      break;

    case ISBN:
      {
        QVariantMap isbnQuery = query.value(QLatin1String("isbn")).toMap();
        // search for both ISBN10 and ISBN13
        QVariantList isbns;
        foreach(const QString& isbn, FieldFormat::splitValue(request().value)) {
          isbns << ISBNValidator::cleanValue(ISBNValidator::isbn10(isbn));
          isbns << ISBNValidator::cleanValue(ISBNValidator::isbn13(isbn));
        }
        isbnQuery.insert(QLatin1String("isbn|="), isbns);
        query.insert(QLatin1String("isbn"), isbnQuery);
      }
      queries << query;
      break;

    case LCCN:
      query.insert(QLatin1String("LCCN"), LCCNValidator::formalize(request().value));
      queries << query;
      break;

    default:
      myWarning() << "bad request key:" << request().key;
      break;
  }

  return queries;
}

QVariantList FreebaseFetcher::comicBookQueries() const {
  QVariantMap query;
  query.insert(QLatin1String("type"), QLatin1String("/comic_books/comic_book_issue"));

  QVariantMap series_query;
  series_query.insert(QLatin1String("type"), QLatin1String("/comic_books/comic_book_series"));
  series_query.insert(QLatin1String("optional"), QLatin1String("optional"));
  series_query.insert(QLatin1String("*"), QVariantList());
  query.insert(QLatin1String("!/comic_books/comic_book_series/issues"), QVariantList() << series_query);

  QVariantList queries;
  switch(request().key) {
    case Title:
      query.insert(QLatin1String("name~="), QLatin1Char('*') + request().value + QLatin1Char('*'));
      queries << query;
      break;

    case Person:
      query.insert(QLatin1String("editor~="), QLatin1Char('*') + request().value + QLatin1Char('*'));
      queries << query;
      break;

    default:
      myWarning() << "bad request key:" << request().key;
      break;
  }

  return queries;
}

QVariantList FreebaseFetcher::movieQueries() const {
  QVariantMap query;
  query.insert(QLatin1String("type"), QLatin1String("/film/film"));

  QVariantMap time_query;
  time_query.insert(QLatin1String("type"), QLatin1String("/film/film_cut"));
  time_query.insert(QLatin1String("optional"), QLatin1String("optional"));
  time_query.insert(QLatin1String("runtime"), QVariantList());
  query.insert(QLatin1String("runtime"), QVariantList() << time_query);

  QVariantMap cast_query;
  cast_query.insert(QLatin1String("type"), QLatin1String("/film/performance"));
  cast_query.insert(QLatin1String("optional"), QLatin1String("optional"));
  cast_query.insert(QLatin1String("actor"), QVariantList());
  cast_query.insert(QLatin1String("character"), QVariantList());
  query.insert(QLatin1String("starring"), QVariantList() << cast_query);

  QVariantMap studio_query;
  studio_query.insert(QLatin1String("type"), QLatin1String("/film/film_film_distributor_relationship"));
  studio_query.insert(QLatin1String("optional"), QLatin1String("optional"));
  studio_query.insert(QLatin1String("distributor"), QVariantList());
  query.insert(QLatin1String("distributors"), QVariantList() << studio_query);

  QVariantList queries;
  switch(request().key) {
    case Title:
      query.insert(QLatin1String("name~="), QLatin1Char('*') + request().value + QLatin1Char('*'));
      queries << query;
      break;

    case Person:
      {
        QVariantMap directorQuery = query;
        directorQuery.insert(QLatin1String("directed_by~="), QLatin1Char('*') + request().value + QLatin1Char('*'));

        QVariantMap producerQuery = query;
        producerQuery.insert(QLatin1String("produced_by~="), QLatin1Char('*') + request().value + QLatin1Char('*'));

        QVariantMap writerQuery = query;
        writerQuery.insert(QLatin1String("written_by~="), QLatin1Char('*') + request().value + QLatin1Char('*'));

        QVariantMap composerQuery = query;
        composerQuery.insert(QLatin1String("music~="), QLatin1Char('*') + request().value + QLatin1Char('*'));

        QVariantMap castSubquery;
        castSubquery.insert(QLatin1String("actor~="), QLatin1Char('*') + request().value + QLatin1Char('*'));
        QVariantMap castQuery = query;
        // have to give it a dummy namespace so we get back all the actors
        castQuery.insert(QLatin1String("b:starring"), QVariantList() << castSubquery);

        queries << directorQuery << castQuery << producerQuery << writerQuery << composerQuery;
      }
      break;

    default:
      myWarning() << "bad request key:" << request().key;
      break;
  }

  return queries;
}

QVariantList FreebaseFetcher::musicQueries() const {
  QVariantMap query;
  query.insert(QLatin1String("type"), QLatin1String("/music/release"));

  QVariantMap track_query;
  track_query.insert(QLatin1String("type"), QLatin1String("/music/track"));
  track_query.insert(QLatin1String("name"), QVariantList());
  track_query.insert(QLatin1String("length"), QVariantList());
  track_query.insert(QLatin1String("artist"), QVariantList());
  track_query.insert(QLatin1String("optional"), QLatin1String("optional"));
  query.insert(QLatin1String("track"), QVariantList() << track_query);

  QVariantMap album_query;
//  album_query.insert(QLatin1String("type"), QLatin1String("/music/album"));
//  album_query.insert(QLatin1String("label"), QVariantList());
  album_query.insert(QLatin1String("genre"), QVariantList());
  album_query.insert(QLatin1String("optional"), QLatin1String("optional"));

  // the image and article can be under the album attribute instead of release
  QVariantMap id_query;
  id_query.insert(QLatin1String("id"), QVariantList());
  id_query.insert(QLatin1String("optional"), QLatin1String("optional"));
  id_query.insert(QLatin1String("limit"), 1);
  album_query.insert(QLatin1String("/common/topic/image"), id_query);
  album_query.insert(QLatin1String("/common/topic/article"), id_query);

  query.insert(QLatin1String("album"), QVariantList() << album_query);

  QVariantList queries;
  switch(request().key) {
    case Title:
      query.insert(QLatin1String("name~="), QLatin1Char('*') + request().value + QLatin1Char('*'));
      queries << query;
      break;

    case Person:
      {
        //release/artist and //release/credited_as are both rawstrings, so they're case sensitive
        // match against album artist instead
        QVariantMap albumQuery = query.value(QLatin1String("album")).toList().at(0).toMap();
        albumQuery.remove(QLatin1String("optional"));
        albumQuery.insert(QLatin1String("artist~="), QLatin1Char('*') + request().value + QLatin1Char('*'));

        query.insert(QLatin1String("album"), QVariantList() << albumQuery);
        queries << query;
      }
      break;

    default:
      myWarning() << "bad request key:" << request().key;
      break;
  }

  return queries;
}

QVariantList FreebaseFetcher::videoGameQueries() const {
  QVariantMap query;
  query.insert(QLatin1String("type"), QLatin1String("/cvg/computer_videogame"));

  QVariantList queries;
  switch(request().key) {
    case Title:
      query.insert(QLatin1String("name~="), QLatin1Char('*') + request().value + QLatin1Char('*'));
      queries << query;
      break;

    case Person:
      {
        QVariantMap developerQuery = query;
        developerQuery.insert(QLatin1String("developer~="), QLatin1Char('*') + request().value + QLatin1Char('*'));

        QVariantMap publisherQuery = query;
        publisherQuery.insert(QLatin1String("publisher~="), QLatin1Char('*') + request().value + QLatin1Char('*'));
        queries << developerQuery << publisherQuery;
      }
      break;

    default:
      myWarning() << "bad request key:" << request().key;
      break;
  }

  return queries;
}

QVariantList FreebaseFetcher::boardGameQueries() const {
  QVariantMap query;
  query.insert(QLatin1String("type"), QLatin1String("/games/game"));

  QVariantMap player_query;
  player_query.insert(QLatin1String("type"), QLatin1String("/measurement_unit/integer_range"));
  player_query.insert(QLatin1String("high_value"), QVariantList());
  player_query.insert(QLatin1String("low_value"), QVariantList());
  player_query.insert(QLatin1String("optional"), QLatin1String("optional"));
  query.insert(QLatin1String("number_of_players"), player_query);

  QVariantList queries;
  switch(request().key) {
    case Title:
      query.insert(QLatin1String("name~="), QLatin1Char('*') + request().value + QLatin1Char('*'));
      queries << query;
      break;

    case Person:
      {
        QVariantMap designerQuery = query;
        designerQuery.insert(QLatin1String("designer~="), QLatin1Char('*') + request().value + QLatin1Char('*'));

        QVariantMap publisherQuery = query;
        publisherQuery.insert(QLatin1String("publisher~="), QLatin1Char('*') + request().value + QLatin1Char('*'));
        queries << designerQuery << publisherQuery;
      }
      break;

    default:
      myWarning() << "bad request key:" << request().key;
      break;
  }

  return queries;
}

FreebaseFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const FreebaseFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(FreebaseFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void FreebaseFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString FreebaseFetcher::ConfigWidget::preferredName() const {
  return FreebaseFetcher::defaultName();
}

#include "freebasefetcher.moc"
