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
#include <qjson/parser.h>
#endif

namespace {
  static const char* OPENLIBRARY_QUERY_URL = "http://openlibrary.org/query.json";
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

// without QJSON, we can only search on ISBN for covers
bool OpenLibraryFetcher::canSearch(FetchKey k) const {
#ifndef HAVE_QJSON
  return k == ISBN;
#else
  return k == Title || k == Person || k == ISBN || k == LCCN || k == Keyword;
#endif
}

bool OpenLibraryFetcher::canFetch(int type) const {
  return type == Data::Collection::Book || Data::Collection::Bibtex;
}

void OpenLibraryFetcher::readConfigHook(const KConfigGroup&) {
}

void OpenLibraryFetcher::search() {
  m_started = true;
  // we only split ISBN and LCCN values
  QStringList searchTerms;
  if(request().key == ISBN || request().key == LCCN) {
    searchTerms = FieldFormat::splitValue(request().value);
  } else  {
    searchTerms += request().value;
  }
  foreach(const QString& searchTerm, searchTerms) {
    doSearch(searchTerm);
  }
  if(m_jobs.isEmpty()) {
    stop();
  }
}

void OpenLibraryFetcher::doSearch(const QString& term_) {
#ifndef HAVE_QJSON
  doCoverOnly(term_);
#else
  KUrl u(OPENLIBRARY_QUERY_URL);

  // books are type/edition
  u.addQueryItem(QLatin1String("type"), QLatin1String("/type/edition"));
  u.addQueryItem(QLatin1String("*"), QString());

  switch(request().key) {
    case Title:
      u.addQueryItem(QLatin1String("title"), term_);
      break;

    case Person:
      {
        const QString author = getAuthorKeys(term_);
        if(author.isEmpty()) {
          myWarning() << "no authors found";
          return;
        }
        u.addQueryItem(QLatin1String("authors"), author);
      }
      break;

    case ISBN:
      {
        const QString isbn = ISBNValidator::cleanValue(term_);
        if(isbn.size() > 10) {
          u.addQueryItem(QLatin1String("isbn_13"), isbn);
        } else {
          u.addQueryItem(QLatin1String("isbn_10"), isbn);
        }
      }
      break;

    case LCCN:
      u.addQueryItem(QLatin1String("lccn"), term_);
      break;

    case Keyword:
      myWarning() << "not supported";
      return;

    default:
      myWarning() << "key not recognized:" << request().key;
      return;
  }

//  myDebug() << "url:" << u;

  QPointer<KIO::StoredTransferJob> job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  job->ui()->setWindow(GUI::Proxy::widget());
  connect(job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
  m_jobs << job;
#endif
}

void OpenLibraryFetcher::doCoverOnly(const QString& term_) {
  switch(request().key) {
     case ISBN:
       break;

    default:
      myWarning() << "key not recognized: " << request().key;
      return;
  }

  int pos = 0;
  ISBNValidator val(this);
  QString isbn = term_;
  if(val.validate(isbn, pos) == QValidator::Acceptable) {
    Data::CollPtr coll(new Data::BookCollection(true));
    Data::EntryPtr entry(new Data::Entry(coll));
    entry->setField(QLatin1String("isbn"), isbn);

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, Data::EntryPtr(entry));
    emit signalResultFound(r);
  }
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

void OpenLibraryFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);
#ifdef HAVE_QJSON
//  myDebug();

  if(job->error()) {
    job->ui()->showErrorMessage();
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
    t.setCodec(QTextCodec::codecForName("UTF-8"));
    t << data;
  }
  f.close();
#endif

  QJson::Parser parser;
  QVariantList resultList = parser.parse(data).toList();
  if(resultList.isEmpty()) {
    myDebug() << "no results";
    endJob(job);
    return;
  }

  Data::CollPtr coll(new Data::BookCollection(true));
  if(!coll->hasField(QLatin1String("openlibrary")) && optionalFields().contains(QLatin1String("openlibrary"))) {
    Data::FieldPtr field(new Data::Field(QLatin1String("openlibrary"), i18n("OpenLibrary Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }

  QVariantMap resultMap;
  foreach(const QVariant& result, resultList) {
    // be sure to check that the fetcher has not been stopped
    // crashes can occur if not
    if(!m_started) {
      break;
    }
//    myDebug() << "found result:" << result;
    resultMap = result.toMap();

//  myDebug() << resultMap.value(QLatin1String("isbn_10")).toList().at(0);

    Data::EntryPtr entry(new Data::Entry(coll));

    entry->setField(QLatin1String("title"), value(resultMap, "title"));
    entry->setField(QLatin1String("subtitle"), value(resultMap, "subtitle"));
    entry->setField(QLatin1String("pub_year"), value(resultMap, "publish_date"));
    QString isbn = value(resultMap, "isbn_10");
    if(isbn.isEmpty()) {
      isbn = value(resultMap, "isbn_13");
    }
    if(!isbn.isEmpty()) {
      ISBNValidator val(this);
      val.fixup(isbn);
      entry->setField(QLatin1String("isbn"), isbn);
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

    if(optionalFields().contains(QLatin1String("openlibrary"))) {
      entry->setField(QLatin1String("openlibrary"), QLatin1String("http://openlibrary.org") + value(resultMap, "key"));
    }

    QStringList authors;
    foreach(const QVariant& authorMap, resultMap.value(QLatin1String("authors")).toList()) {
      const QString key = value(authorMap.toMap(), "key");
      if(!key.isEmpty()) {
        KUrl authorUrl(OPENLIBRARY_QUERY_URL);
        authorUrl.addQueryItem(QLatin1String("type"), QLatin1String("/type/author"));
        authorUrl.addQueryItem(QLatin1String("key"), key);
        authorUrl.addQueryItem(QLatin1String("name"), QString());

        QString output = FileHandler::readTextFile(authorUrl, false /*quiet*/);
        QVariantList authorList = parser.parse(output.toUtf8()).toList();
        QVariantMap authorResult = authorList.isEmpty() ? QVariantMap() : authorList.at(0).toMap();
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
        KUrl langUrl(OPENLIBRARY_QUERY_URL);
        langUrl.addQueryItem(QLatin1String("type"), QLatin1String("/type/language"));
        langUrl.addQueryItem(QLatin1String("key"), key);
        langUrl.addQueryItem(QLatin1String("name"), QString());

        QString output = FileHandler::readTextFile(langUrl, false /*quiet*/, true /*utf8*/);
        QVariantList langList = parser.parse(output.toUtf8()).toList();
        QVariantMap langResult = langList.isEmpty() ? QVariantMap() : langList.at(0).toMap();
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
#endif
  endJob(job);
}

QString OpenLibraryFetcher::getAuthorKeys(const QString& term_) {
#ifdef HAVE_QJSON
  KUrl u(OPENLIBRARY_QUERY_URL);

  u.addQueryItem(QLatin1String("type"), QLatin1String("/type/author"));
  u.addQueryItem(QLatin1String("name"), term_);

  QString output = FileHandler::readTextFile(u, false /*quiet*/, true /*utf8*/);
  QJson::Parser parser;
  QVariantList results = parser.parse(output.toUtf8()).toList();
  myDebug() << "found" << results.count() << "authors";
  // right now, only use the first
  return results.isEmpty() ? QString() : value(results.at(0).toMap(), "key");
#else
  Q_UNUSED(term_);
  return QString();
#endif
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

Tellico::StringHash OpenLibraryFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("openlibrary")] = i18n("OpenLibrary Link");
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

// static
QString OpenLibraryFetcher::value(const QVariantMap& map, const char* name) {
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

#include "openlibraryfetcher.moc"
