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
#include "googlebookfetcher.h"
#include "../collections/bookcollection.h"
#include "../entry.h"
#include "../utils/isbnvalidator.h"
#include "../gui/guiproxy.h"
#include "../tellico_utils.h"
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
  static const int GOOGLEBOOK_MAX_RETURNS = 20;
  static const char* GOOGLEBOOK_API_URL = "https://www.googleapis.com/books/v1/volumes";
}

using namespace Tellico;
using Tellico::Fetch::GoogleBookFetcher;

GoogleBookFetcher::GoogleBookFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false)
    , m_start(0) {
}

GoogleBookFetcher::~GoogleBookFetcher() {
}

QString GoogleBookFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool GoogleBookFetcher::canSearch(FetchKey k) const {
#ifdef HAVE_QJSON
  return k == Title || k == Person || k == ISBN || k == Keyword;
#else
  return false;
#endif
}

bool GoogleBookFetcher::canFetch(int type) const {
  return type == Data::Collection::Book;
}

void GoogleBookFetcher::readConfigHook(const KConfigGroup&) {
}

void GoogleBookFetcher::search() {
  m_start = 0;
  m_total = -1;
  continueSearch();
}

void GoogleBookFetcher::continueSearch() {
  m_started = true;
  // we only split ISBN and LCCN values
  QStringList searchTerms;
  if(request().key == ISBN) {
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

void GoogleBookFetcher::doSearch(const QString& term_) {
#ifdef HAVE_QJSON
  KUrl u(GOOGLEBOOK_API_URL);

  u.addQueryItem(QLatin1String("maxResults"), QString::number(GOOGLEBOOK_MAX_RETURNS));
  u.addQueryItem(QLatin1String("startIndex"), QString::number(m_start));
  u.addQueryItem(QLatin1String("printType"), QLatin1String("books"));

  switch(request().key) {
    case Title:
      u.addQueryItem(QLatin1String("q"), QLatin1String("intitle:") + term_);
      break;

    case Person:
      u.addQueryItem(QLatin1String("q"), QLatin1String("inauthor:") + term_);
      break;

    case ISBN:
      {
        const QString isbn = ISBNValidator::cleanValue(term_);
        u.addQueryItem(QLatin1String("q"), QLatin1String("isbn:") + isbn);
      }
      break;

    case Keyword:
      u.addQueryItem(QLatin1String("q"), term_);
      break;

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

void GoogleBookFetcher::endJob(KIO::StoredTransferJob* job_) {
  m_jobs.removeOne(job_);
  if(m_jobs.isEmpty())  {
    stop();
  }
}

void GoogleBookFetcher::stop() {
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

Tellico::Data::EntryPtr GoogleBookFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }
  return entry;
}

Tellico::Fetch::FetchRequest GoogleBookFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString isbn = entry_->field(QLatin1String("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }
  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

void GoogleBookFetcher::slotComplete(KJob* job_) {
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
  myWarning() << "Remove debug from googlebookfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec(QTextCodec::codecForName("UTF-8"));
    t << data;
  }
  f.close();
#endif

  QJson::Parser parser;
  QVariantMap result = parser.parse(data).toMap();
  m_total = result.value(QLatin1String("totalItems")).toInt();
//  myDebug() << "total:" << m_total;

  QVariantList resultList = result.value(QLatin1String("items")).toList();
  if(resultList.isEmpty()) {
    myDebug() << "no results";
    endJob(job);
    return;
  }

  Data::CollPtr coll(new Data::BookCollection(true));
  if(!coll->hasField(QLatin1String("googlebook")) && optionalFields().contains(QLatin1String("googlebook"))) {
    Data::FieldPtr field(new Data::Field(QLatin1String("googlebook"), i18n("Google Book Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }

  QVariantMap resultMap;
  foreach(const QVariant& result, resultList) {
  //  myDebug() << "found result:" << result;
    resultMap = result.toMap().value(QLatin1String("volumeInfo")).toMap();

//  myDebug() << resultMap.value(QLatin1String("isbn_10")).toList().at(0);

    Data::EntryPtr entry(new Data::Entry(coll));

    entry->setField(QLatin1String("title"), value(resultMap, "title"));
    entry->setField(QLatin1String("subtitle"), value(resultMap, "subtitle"));
    entry->setField(QLatin1String("pub_year"), value(resultMap, "publishedDate").left(4));
    entry->setField(QLatin1String("keyword"), value(resultMap, "categories"));
    entry->setField(QLatin1String("author"), value(resultMap, "authors"));
    entry->setField(QLatin1String("publisher"), value(resultMap, "publisher"));
    entry->setField(QLatin1String("pages"), value(resultMap, "pageCount"));
    entry->setField(QLatin1String("language"), value(resultMap, "language"));
    entry->setField(QLatin1String("comments"), value(resultMap, "description"));

    QString isbn;
    foreach(const QVariant& idVariant, resultMap.value(QLatin1String("industryIdentifiers")).toList()) {
      const QVariantMap idMap = idVariant.toMap();
      if(value(idMap, "type") == QLatin1String("ISBN_10")) {
        isbn = value(idMap, "identifier");
        break;
      } else if(value(idMap, "type") == QLatin1String("ISBN_13")) {
        isbn = value(idMap, "identifier");
        // allow isbn10 to override, so don't break here
      }
    }
    if(!isbn.isEmpty()) {
      ISBNValidator val(this);
      val.fixup(isbn);
      entry->setField(QLatin1String("isbn"), isbn);
    }

    const QVariantMap imageMap = resultMap.value(QLatin1String("imageLinks")).toMap();
    if(imageMap.contains(QLatin1String("small"))) {
      entry->setField(QLatin1String("cover"), value(imageMap, "small"));
    } else if(imageMap.contains(QLatin1String("thumbnail"))) {
      entry->setField(QLatin1String("cover"), value(imageMap, "thumbnail"));
    } else if(imageMap.contains(QLatin1String("smallThumbnail"))) {
      entry->setField(QLatin1String("cover"), value(imageMap, "smallThumbnail"));
    }

    if(optionalFields().contains(QLatin1String("googlebook"))) {
      entry->setField(QLatin1String("googlebook"), value(resultMap, "infoLink"));
    }

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }

  m_start = m_entries.count();
  m_hasMoreResults = request().key != ISBN && m_start <= m_total;
#endif
  endJob(job);
}

Tellico::Fetch::ConfigWidget* GoogleBookFetcher::configWidget(QWidget* parent_) const {
  return new GoogleBookFetcher::ConfigWidget(parent_, this);
}

QString GoogleBookFetcher::defaultName() {
  return i18n("Google Book Search");
}

QString GoogleBookFetcher::defaultIcon() {
  return favIcon("http://books.google.com");
}

Tellico::StringHash GoogleBookFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("googlebook")] = i18n("Google Book Link");
  return hash;
}

GoogleBookFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const GoogleBookFetcher*)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

void GoogleBookFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString GoogleBookFetcher::ConfigWidget::preferredName() const {
  return GoogleBookFetcher::defaultName();
}

// static
QString GoogleBookFetcher::value(const QVariantMap& map, const char* name) {
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

#include "googlebookfetcher.moc"
