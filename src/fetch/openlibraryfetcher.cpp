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
#include "../gui/guiproxy.h"
#include "../utils/isbnvalidator.h"
#include "../tellico_utils.h"
#include "../entry.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <KConfigGroup>

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
  static const char* OPENLIBRARY_GET_URL = "http://openlibrary.org/api/get";
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

  switch(request().key) {
    case ISBN:
      query.insert(QLatin1String("isbn_10"), ISBNValidator::cleanValue(request().value));
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      stop();
      return;
  }

  QJson::Serializer serializer;
  QByteArray json = serializer.serialize(query);
  myDebug() << json;

  u.addQueryItem(QLatin1String("query"), QString::fromUtf8(json));
  myDebug() << "url:" << u;

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
      if(id.isEmpty()) {
        myWarning() << "no image";
      } else {
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

#if 0
  myWarning() << "Remove debug from openlibraryfetcher.cpp";
  const QString text = QString::fromUtf8(data, data.size());
  QFile f(QString::fromLatin1("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec(QTextCodec::codecForName("UTF-8"));
    t << text;
  }
  f.close();
#endif

  QJson::Parser parser;
  QVariantMap result = parser.parse(data).toMap();
  if(result.value(QLatin1String("status"))  != QLatin1String("ok")) {
    myDebug() << "bad status result:" << result.value(QLatin1String("status"));
    stop();
    return;
  }

  QVariantList resultList = result.value(QLatin1String("result")).toList();
  if(resultList.isEmpty()) {
    myDebug() << "no results";
    stop();
    return;
  }

  QString result1 = resultList.at(0).toString();
//  myDebug() << "found result:" << result1;

  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  KUrl u(OPENLIBRARY_GET_URL);
  u.addQueryItem(QLatin1String("key"), result1);

  QString output = FileHandler::readTextFile(u, false /*quiet*/, true /*utf8*/);
  result = parser.parse(output.toUtf8()).toMap();
  QVariantMap resultMap = result.value(QLatin1String("result")).toMap();

//  myDebug() << resultMap.value(QLatin1String("isbn_10")).toList().at(0);

  Data::CollPtr coll(new Data::BookCollection(true));
  Data::EntryPtr entry(new Data::Entry(coll));

  entry->setField(QLatin1String("title"), resultMap.value(QLatin1String("title")).toString());
  entry->setField(QLatin1String("pub_year"), resultMap.value(QLatin1String("publish_date")).toString());
  entry->setField(QLatin1String("isbn"), resultMap.value(QLatin1String("isbn_10")).toList().at(0).toString());
  entry->setField(QLatin1String("lccn"), resultMap.value(QLatin1String("lccn")).toList().at(0).toString());

  FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
  m_entries.insert(r->uid, entry);
  emit signalResultFound(r);

//  m_start = m_entries.count();
//  m_hasMoreResults = m_start <= m_total;
  m_hasMoreResults = false; // for now, no continued searches

  stop(); // required
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
