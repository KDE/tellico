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

#include "openlibraryfetcher.h"
#include "../collections/bookcollection.h"
#include "../collections/comicbookcollection.h"
#include "../images/imagefactory.h"
#include "../utils/isbnvalidator.h"
#include "../utils/guiproxy.h"
#include "../utils/objvalue.h"
#include "../entry.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KJobUiDelegate>
#include <KJobWidgets>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

namespace {
  static const char* OPENLIBRARY_QUERY_URL = "https://openlibrary.org/query.json";
  static const char* OPENLIBRARY_AUTHOR_QUERY_URL = "https://openlibrary.org/search/authors.json";
}

using namespace Tellico;
using Tellico::Fetch::OpenLibraryFetcher;

OpenLibraryFetcher::OpenLibraryFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false) {
}

OpenLibraryFetcher::~OpenLibraryFetcher() {
}

QString OpenLibraryFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool OpenLibraryFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == Person || k == ISBN || k == LCCN;
}

bool OpenLibraryFetcher::canFetch(int type) const {
  return type == Data::Collection::Book ||
      type == Data::Collection::Bibtex ||
      type == Data::Collection::ComicBook;
}

void OpenLibraryFetcher::readConfigHook(const KConfigGroup&) {
}

void OpenLibraryFetcher::search() {
  m_started = true;
  // we only split ISBN and LCCN values
  QStringList searchTerms;
  if(request().key() == ISBN || request().key() == LCCN) {
    searchTerms = FieldFormat::splitValue(request().value());
  } else  {
    searchTerms += request().value();
  }
  foreach(const QString& searchTerm, searchTerms) {
    doSearch(searchTerm);
  }
  if(m_jobs.isEmpty()) {
    stop();
  }
}

void OpenLibraryFetcher::doSearch(const QString& term_) {
  QUrl u(QString::fromLatin1(OPENLIBRARY_QUERY_URL));
  QUrlQuery q;
  // books are type/edition
  q.addQueryItem(QStringLiteral("type"), QStringLiteral("/type/edition"));

  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("title"), term_);
      break;

    case Person:
      {
        QString author = getAuthorKeys(term_);
        if(author.isEmpty()) {
          myLog() << "No matching authors found";
          return;
        }
        author.prepend(QLatin1String("/authors/"));
        q.addQueryItem(QStringLiteral("authors"), author);
      }
      break;

    case ISBN:
      {
        const QString isbn = ISBNValidator::cleanValue(term_);
        if(isbn.size() > 10) {
          q.addQueryItem(QStringLiteral("isbn_13"), isbn);
        } else {
          q.addQueryItem(QStringLiteral("isbn_10"), isbn);
        }
      }
      break;

    case LCCN:
      q.addQueryItem(QStringLiteral("lccn"), term_);
      break;

    case Raw:
      {
        // raw query comes in as a query string, combine it
        QUrlQuery newQuery(term_);
        const auto newQueryItems = newQuery.queryItems();
        for(const auto& item : newQueryItems) {
          q.addQueryItem(item.first, item.second);
        }
      }
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      return;
  }
  q.addQueryItem(QStringLiteral("*"), QString());
  u.setQuery(q);
//  myDebug() << u;

  QPointer<KIO::StoredTransferJob> job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  connect(job.data(), &KJob::result, this, &OpenLibraryFetcher::slotComplete);
  m_jobs << job;
}

void OpenLibraryFetcher::endJob(KIO::StoredTransferJob* job_) {
  m_jobs.removeAll(job_);
  if(m_jobs.isEmpty())  {
    stop();
  }
}

void OpenLibraryFetcher::stop() {
  if(!m_started) {
    return;
  }
  foreach(QPointer<KIO::StoredTransferJob> job, m_jobs) {
    if(job) {
      job->kill();
    }
  }
  m_jobs.clear();
  m_started = false;
  Q_EMIT signalDone(this);
}

Tellico::Data::EntryPtr OpenLibraryFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  // possible that the author is set on the work but not the edition
  // see https://github.com/internetarchive/openlibrary/issues/8144
  const QString authorString(QStringLiteral("author"));
  const QString workString(QStringLiteral("openlibrary-work"));
  if(entry->field(authorString).isEmpty()) {
    const QString work = entry->field(workString);
    if(!work.isEmpty()) {
      QUrl workUrl(QStringLiteral("https://openlibrary.org%1.json").arg(work));
      QStringList authors;
      const auto output = FileHandler::readDataFile(workUrl, true /*quiet*/);
      QJsonDocument doc = QJsonDocument::fromJson(output);
      auto array = doc.object().value(QLatin1String("authors")).toArray();
      for(int i = 0; i < array.count(); i++) {
        const QString key = objValue(array.at(i).toObject(), "author", "key");
        if(m_authorHash.contains(key)) {
          authors += m_authorHash.value(key);
          continue;
        }
        // now grab author name by key
        QUrl authorUrl(QStringLiteral("https://openlibrary.org%1.json").arg(key));
        const auto output2 = FileHandler::readDataFile(authorUrl, true /*quiet*/);
        QJsonDocument doc2 = QJsonDocument::fromJson(output2);
        const QString author = objValue(doc2.object(), "name");
        if(!author.isEmpty()) {
          m_authorHash.insert(key, author);
          authors += author;
        }
      }
      if(!authors.isEmpty()) {
        entry->setField(authorString, authors.join(FieldFormat::delimiterString()));
      }
    }
  }
  // no longer need the field
  entry->collection()->removeField(workString);

  // if the entry is not set, go ahead and try to fetch it
  const QString coverString(QStringLiteral("cover"));
  if(entry->field(coverString).isEmpty()) {
    const QString isbn = ISBNValidator::cleanValue(entry->field(QStringLiteral("isbn")));
    if(!isbn.isEmpty()) {
      QUrl imageUrl(QStringLiteral("https://covers.openlibrary.org/b/isbn/%1-M.jpg?default=false").arg(isbn));
      const QString id = ImageFactory::addImage(imageUrl, true);
      if(!id.isEmpty()) {
        entry->setField(coverString, id);
      }
    }
  }

  return entry;
}

Tellico::Fetch::FetchRequest OpenLibraryFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString isbn = entry_->field(QStringLiteral("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }
  const QString lccn = entry_->field(QStringLiteral("lccn"));
  if(!lccn.isEmpty()) {
    return FetchRequest(LCCN, lccn);
  }
  const QString title = entry_->field(QStringLiteral("title"));
  if(title.isEmpty()) {
    return FetchRequest();
  }

  // can't search by authors, for now, since the author value is a key reference
  // can't search by pub year since many of the publish_date fields are a full month, day, year

  const QString pub = entry_->field(QStringLiteral("publisher"));
  auto publishers = FieldFormat::splitValue(pub);
  if(!publishers.isEmpty()) {
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("title"), title);
    q.addQueryItem(QStringLiteral("publishers"), publishers.first());
    return FetchRequest(Raw, q.query());
  }

  // fallback to just title search
  return FetchRequest(Title, title);
}

void OpenLibraryFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    job->uiDelegate()->showErrorMessage();
    endJob(job);
    return;
  }

  QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    endJob(job);
    return;
  }

#if 0
  myWarning() << "Remove debug from openlibraryfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  const auto array = doc.array();
  if(array.isEmpty()) {
//    myDebug() << "no results";
    endJob(job);
    return;
  }

  Data::CollPtr coll;
  if(request().collectionType() == Data::Collection::ComicBook) {
    coll = new Data::ComicBookCollection(true);
  } else {
    coll = new Data::BookCollection(true);
  }
  // add a temporary work id field
  Data::FieldPtr wField(new Data::Field(QStringLiteral("openlibrary-work"), QString()));
  coll->addField(wField);

  if(!coll->hasField(QStringLiteral("openlibrary")) && optionalFields().contains(QStringLiteral("openlibrary"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("openlibrary"), i18n("OpenLibrary Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }

  static const QRegularExpression yearRx(QStringLiteral("\\d{4}"));
  for(const auto& result : array) {
    // be sure to check that the fetcher has not been stopped
    // crashes can occur if not
    if(!m_started) {
      break;
    }

    Data::EntryPtr entry(new Data::Entry(coll));
    const auto resObj = result.toObject();
    entry->setField(QStringLiteral("title"), objValue(resObj, "title"));
    // only allow comic format for comic book collections
    QString binding = objValue(resObj, "physical_format");
    if(coll->type() == Data::Collection::ComicBook &&
       (!binding.isEmpty() && binding != QLatin1String("comic"))) {
      myLog() << "Skipping non-comic result:" << entry->title();
      continue;
    }
    const auto bindingLower = binding.toLower();
    if(bindingLower == QLatin1String("hardcover")) {
      binding = QStringLiteral("Hardback");
    } else if(bindingLower == QLatin1String("ebook")) {
      binding = QStringLiteral("E-Book");
    } else if(bindingLower.contains(QStringLiteral("paperback"))) {
      binding = QStringLiteral("Paperback");
    }
    if(!binding.isEmpty()) {
      entry->setField(QStringLiteral("binding"), i18n(binding.toUtf8().constData()));
    }

    entry->setField(QStringLiteral("subtitle"), objValue(resObj, "subtitle"));
    auto yearMatch = yearRx.match(objValue(resObj, "publish_date"));
    if(yearMatch.hasMatch()) {
      entry->setField(QStringLiteral("pub_year"), yearMatch.captured());
    }
    yearMatch = yearRx.match(objValue(resObj, "copyright_date"));
    if(yearMatch.hasMatch()) {
      entry->setField(QStringLiteral("cr_year"), yearMatch.captured());
    }
    QString isbn = objValue(resObj, "isbn_10");
    if(isbn.isEmpty()) {
      isbn = objValue(resObj, "isbn_13");
    }
    const QString isbnName(QStringLiteral("isbn"));
    if(!isbn.isEmpty()) {
      if(!coll->hasField(isbnName)) {
        coll->addField(Data::Field::createDefaultField(Data::Field::IsbnField));
      }
      ISBNValidator val(this);
      val.fixup(isbn);
      entry->setField(isbnName, isbn);
    }
    const QString lccnName(QStringLiteral("lccn"));
    const QString lccn = objValue(resObj, "lccn");
    if(!lccn.isEmpty()) {
      if(!coll->hasField(lccnName)) {
        coll->addField(Data::Field::createDefaultField(Data::Field::LccnField));
      }
      entry->setField(lccnName, lccn);
    }
    entry->setField(QStringLiteral("genre"), objValue(resObj, "genres"));
    entry->setField(QStringLiteral("keyword"), objValue(resObj, "subjects"));
    entry->setField(QStringLiteral("edition"), objValue(resObj, "edition_name"));
    entry->setField(QStringLiteral("publisher"), objValue(resObj, "publishers"));
    entry->setField(QStringLiteral("series"), objValue(resObj, "series"));
    entry->setField(QStringLiteral("pages"), objValue(resObj, "number_of_pages"));
    entry->setField(QStringLiteral("comments"), objValue(resObj, "notes", "value"));

    if(optionalFields().contains(QStringLiteral("openlibrary"))) {
      entry->setField(QStringLiteral("openlibrary"), QLatin1String("https://openlibrary.org") + objValue(resObj, "key"));
    }
    const auto worksArray = resObj[QLatin1StringView("works")].toArray();
    if(!worksArray.isEmpty()) {
      const auto workObj = worksArray.first().toObject();
      const QString key = objValue(workObj, "key");
      if(!key.isEmpty()) {
        entry->setField(QStringLiteral("openlibrary-work"), key);
      }
    }

    QStringList authors;
    const auto authorArray = resObj[QLatin1StringView("authors")].toArray();
    for(const auto& author : authorArray) {
      const auto authorObj = author.toObject();
      const QString key = objValue(authorObj, "key");
      if(m_authorHash.contains(key)) {
        authors += m_authorHash.value(key);
      } else if(!key.isEmpty()) {
        QUrl authorUrl(QString::fromLatin1(OPENLIBRARY_QUERY_URL));
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("type"), QStringLiteral("/type/author"));
        q.addQueryItem(QStringLiteral("key"), key);
        q.addQueryItem(QStringLiteral("name"), QString());
        authorUrl.setQuery(q);

        QString output = FileHandler::readTextFile(authorUrl, true /*quiet*/);
        QJsonDocument doc2 = QJsonDocument::fromJson(output.toUtf8());
        const auto authorArray = doc2.array();
        if(!authorArray.isEmpty()) {
          const QString name = objValue(authorArray.at(0).toObject(), "name");
          if(!name.isEmpty()) {
            authors << name;
            m_authorHash.insert(key, name);
          }
        }
      }
    }
    if(!authors.isEmpty()) {
      entry->setField(QStringLiteral("author"), authors.join(FieldFormat::delimiterString()));
    }

    QStringList langs;
    const auto langArray = resObj[QLatin1String("languages")].toArray();
    for(const auto& lang : langArray) {
      const auto langObj = lang.toObject();
      const QString key = objValue(langObj, "key");
      if(m_langHash.contains(key)) {
        langs += m_langHash.value(key);
      } else if(!key.isEmpty()) {
        QUrl langUrl(QString::fromLatin1(OPENLIBRARY_QUERY_URL));
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("type"), QStringLiteral("/type/language"));
        q.addQueryItem(QStringLiteral("key"), key);
        q.addQueryItem(QStringLiteral("name"), QString());
        langUrl.setQuery(q);

        const auto output = FileHandler::readDataFile(langUrl, true /*quiet*/);
        QJsonDocument doc2 = QJsonDocument::fromJson(output);
        const auto langArray = doc2.array();
        if(!langArray.isEmpty()) {
          const QString name = objValue(langArray.at(0).toObject(), "name");
          if(!name.isEmpty()) {
            langs << i18n(name.toUtf8().constData());
            m_langHash.insert(key, langs.last());
          }
        }
      }
    }
    if(!langs.isEmpty()) {
      entry->setField(QStringLiteral("language"), langs.join(FieldFormat::delimiterString()));
    }

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    Q_EMIT signalResultFound(r);
  }

//  m_start = m_entries.count();
//  m_hasMoreResults = m_start <= m_total;
  m_hasMoreResults = false; // for now, no continued searches
  endJob(job);
}

QString OpenLibraryFetcher::getAuthorKeys(const QString& term_) {
  QUrl u(QString::fromLatin1(OPENLIBRARY_AUTHOR_QUERY_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("q"), term_);
  q.addQueryItem(QStringLiteral("fields"), QStringLiteral("key,name"));
  u.setQuery(q);

//  myLog() << "Searching for authors:" << u.toDisplayString();
  QString output = FileHandler::readTextFile(u, true /*quiet*/, true /*utf8*/);
#if 0
  myWarning() << "Remove author debug from openlibraryfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test-openlibraryauthor.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << output;
  }
  f.close();
#endif
  const QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
  const auto array = doc.object().value(QLatin1String("docs")).toArray();
  if(array.isEmpty()) {
    return QString();
  }
  // right now, only use the first to search on
  const auto obj1 = array.at(0).toObject();
  const auto key = obj1.value(QLatin1String("key")).toString();
  const auto name = obj1.value(QLatin1String("name")).toString();
  m_authorHash.insert(key, name);
  return key;
}

Tellico::Fetch::ConfigWidget* OpenLibraryFetcher::configWidget(QWidget* parent_) const {
  return new OpenLibraryFetcher::ConfigWidget(parent_, this);
}

QString OpenLibraryFetcher::defaultName() {
  return QStringLiteral("Open Library"); // no translation
}

QString OpenLibraryFetcher::defaultIcon() {
  return favIcon("https://openlibrary.org");
}

Tellico::StringHash OpenLibraryFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("openlibrary")] = i18n("OpenLibrary Link");
  return hash;
}

OpenLibraryFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const OpenLibraryFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(OpenLibraryFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void OpenLibraryFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString OpenLibraryFetcher::ConfigWidget::preferredName() const {
  return OpenLibraryFetcher::defaultName();
}
