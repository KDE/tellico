/***************************************************************************
    copyright            : (C) 2006-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "isbndbfetcher.h"
#include "messagehandler.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../tellico_kernel.h"
#include "../tellico_utils.h"
#include "../collection.h"
#include "../entry.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kstandarddirs.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <KConfigGroup>

#include <QDomDocument>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>

namespace {
  static const int ISBNDB_RETURNS_PER_REQUEST = 10;
  static const int ISBNDB_MAX_RETURNS_TOTAL = 25;
  static const char* ISBNDB_BASE_URL = "http://isbndb.com/api/books.xml";
  static const char* ISBNDB_APP_ID = "3B9S3BQS";
}

using Tellico::Fetch::ISBNdbFetcher;

ISBNdbFetcher::ISBNdbFetcher(QObject* parent_)
    : Fetcher(parent_), m_xsltHandler(0),
      m_limit(ISBNDB_MAX_RETURNS_TOTAL), m_page(1), m_total(-1), m_countOffset(0),
      m_job(0), m_started(false) {
}

ISBNdbFetcher::~ISBNdbFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = 0;
}

QString ISBNdbFetcher::defaultName() {
  return i18n("ISBNdb.com");
}

QString ISBNdbFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool ISBNdbFetcher::canFetch(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::ComicBook || type == Data::Collection::Bibtex;
}

void ISBNdbFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void ISBNdbFetcher::search(Tellico::Fetch::FetchKey key_, const QString& value_) {
  m_key = key_;
  m_value = value_.trimmed();
  m_started = true;
  m_page = 1;
  m_total = -1;
  m_numResults = 0;
  m_countOffset = 0;

  if(!canFetch(Kernel::self()->collectionType())) {
    message(i18n("%1 does not allow searching for this collection type.", source()), MessageHandler::Warning);
    stop();
    return;
  }
  doSearch();
}

void ISBNdbFetcher::continueSearch() {
  m_started = true;
  m_limit += ISBNDB_MAX_RETURNS_TOTAL;
  doSearch();
}

void ISBNdbFetcher::doSearch() {
//  myDebug() << "ISBNdbFetcher::search() - value = " << value_ << endl;

  KUrl u(QString::fromLatin1(ISBNDB_BASE_URL));
  u.addQueryItem(QString::fromLatin1("access_key"), QString::fromLatin1(ISBNDB_APP_ID));
  u.addQueryItem(QString::fromLatin1("results"), QString::fromLatin1("details,authors,subjects,texts"));
  u.addQueryItem(QString::fromLatin1("page_number"), QString::number(m_page));

  switch(m_key) {
    case Title:
      u.addQueryItem(QString::fromLatin1("index1"), QString::fromLatin1("title"));
      u.addQueryItem(QString::fromLatin1("value1"), m_value);
      break;

    case Person:
      // yes, this also queries titles, too, it's a limitation of the isbndb api service
      u.addQueryItem(QString::fromLatin1("index1"), QString::fromLatin1("combined"));
      u.addQueryItem(QString::fromLatin1("value1"), m_value);
      break;

    case Keyword:
      u.addQueryItem(QString::fromLatin1("index1"), QString::fromLatin1("full"));
      u.addQueryItem(QString::fromLatin1("value1"), m_value);
      break;

    case ISBN:
      u.addQueryItem(QString::fromLatin1("index1"), QString::fromLatin1("isbn"));
      {
        // only grab first value
        QString v = m_value.section(QChar(';'), 0);
        v.remove('-');
        u.addQueryItem(QString::fromLatin1("value1"), v);
      }
      break;

    default:
      kWarning() << "ISBNdbFetcher::search() - key not recognized: " << m_key;
      stop();
      return;
  }
//  myDebug() << "ISBNdbFetcher::search() - url: " << u.url() << endl;

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(Kernel::self()->widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
}

void ISBNdbFetcher::stop() {
  if(!m_started) {
    return;
  }
//  myDebug() << "ISBNdbFetcher::stop()" << endl;
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }

  m_started = false;
  emit signalDone(this);
}

void ISBNdbFetcher::slotComplete(KJob*) {
//  myDebug() << "ISBNdbFetcher::slotComplete()" << endl;

  if(m_job->error()) {
    m_job->ui()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "ISBNdbFetcher::slotComplete() - no data" << endl;
    stop();
    return;
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;
#if 0
  kWarning() << "Remove debug from isbndbfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << data;
  }
  f.close();
#endif

  QDomDocument dom;
  if(!dom.setContent(data, false)) {
    kWarning() << "ISBNdbFetcher::slotComplete() - server did not return valid XML.";
    return;
  }

  if(m_total == -1) {
    QDomNode n = dom.documentElement().namedItem(QString::fromLatin1("BookList"));
    QDomElement e = n.toElement();
    if(!e.isNull()) {
      m_total = e.attribute(QString::fromLatin1("total_results"), QString::number(-1)).toInt();
    }
  }

  if(!m_xsltHandler) {
    initXSLTHandler();
    if(!m_xsltHandler) { // probably an error somewhere in the stylesheet loading
      stop();
      return;
    }
  }

  // assume result is always utf-8
  QString str = m_xsltHandler->applyStylesheet(QString::fromUtf8(data, data.size()));
  Import::TellicoImporter imp(str);
  Data::CollPtr coll = imp.collection();

  int count = 0;
  Data::EntryList entries = coll->entries();
  foreach(Data::EntryPtr entry, entries) {
    if(m_numResults >= m_limit) {
      break;
    }
    if(count < m_countOffset) {
      continue;
    }
    if(!m_started) {
      // might get aborted
      break;
    }
    QString desc = entry->field(QString::fromLatin1("author"))
                 + QChar('/') + entry->field(QString::fromLatin1("publisher"));
    if(!entry->field(QString::fromLatin1("cr_year")).isEmpty()) {
      desc += QChar('/') + entry->field(QString::fromLatin1("cr_year"));
    } else if(!entry->field(QString::fromLatin1("pub_year")).isEmpty()){
      desc += QChar('/') + entry->field(QString::fromLatin1("pub_year"));
    }

    SearchResult* r = new SearchResult(Fetcher::Ptr(this), entry->title(), desc, entry->field(QString::fromLatin1("isbn")));
    m_entries.insert(r->uid, Data::EntryPtr(entry));
    emit signalResultFound(r);
    ++m_numResults;
    ++count;
  }

  // are there any additional results to get?
  m_hasMoreResults = m_page * ISBNDB_RETURNS_PER_REQUEST < m_total;

  const int currentTotal = qMin(m_total, m_limit);
  if(m_page * ISBNDB_RETURNS_PER_REQUEST < currentTotal) {
    int foundCount = (m_page-1) * ISBNDB_RETURNS_PER_REQUEST + coll->entryCount();
    message(i18n("Results from %1: %2/%3", source(), foundCount, m_total), MessageHandler::Status);
    ++m_page;
    m_countOffset = 0;
    doSearch();
  } else {
    m_countOffset = m_entries.count() % ISBNDB_RETURNS_PER_REQUEST;
    if(m_countOffset == 0) {
      ++m_page; // need to go to next page
    }
    stop(); // required
  }
}

Tellico::Data::EntryPtr ISBNdbFetcher::fetchEntry(uint uid_) {
  Data::EntryPtr entry = m_entries[uid_];
  if(!entry) {
    kWarning() << "ISBNdbFetcher::fetchEntry() - no entry in dict";
    return Data::EntryPtr();
  }

  // if the publisher id is set, then we need to grab the real publisher name
  const QString id = entry->field(QString::fromLatin1("pub_id"));
  if(!id.isEmpty()) {
    KUrl u(QString::fromLatin1(ISBNDB_BASE_URL));
    u.setFileName(QString::fromLatin1("publishers.xml"));
    u.addQueryItem(QString::fromLatin1("access_key"), QString::fromLatin1(ISBNDB_APP_ID));
    u.addQueryItem(QString::fromLatin1("index1"), QString::fromLatin1("publisher_id"));
    u.addQueryItem(QString::fromLatin1("value1"), id);

    QDomDocument dom = FileHandler::readXMLFile(u, true);
    if(!dom.isNull()) {
      QString pub = dom.documentElement().namedItem(QString::fromLatin1("PublisherList"))
                                         .namedItem(QString::fromLatin1("PublisherData"))
                                         .namedItem(QString::fromLatin1("Name"))
                                         .toElement().text();
      if(!pub.isEmpty()) {
        entry->setField(QString::fromLatin1("publisher"), pub);
      }
    }
    entry->setField(QString::fromLatin1("pub_id"), QString());
  }

  return entry;
}

void ISBNdbFetcher::initXSLTHandler() {
  QString xsltfile = KStandardDirs::locate("appdata", QString::fromLatin1("isbndb2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    kWarning() << "ISBNdbFetcher::initXSLTHandler() - can not locate isbndb2tellico.xsl.";
    return;
  }

  KUrl u;
  u.setPath(xsltfile);

  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    kWarning() << "ISBNdbFetcher::initXSLTHandler() - error in isbndb2tellico.xsl.";
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

void ISBNdbFetcher::updateEntry(Tellico::Data::EntryPtr entry_) {
//  myDebug() << "ISBNdbFetcher::updateEntry()" << endl;
  // limit to top 5 results
  m_limit = 5;

  QString isbn = entry_->field(QString::fromLatin1("isbn"));
  if(!isbn.isEmpty()) {
    search(Fetch::ISBN, isbn);
    return;
  }

  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  QString t = entry_->field(QString::fromLatin1("title"));
  if(!t.isEmpty()) {
    m_limit = 10; // raise limit so more possibility of match
    search(Fetch::Title, t);
    return;
  }

  myDebug() << "ISBNdbFetcher::updateEntry() - insufficient info to search" << endl;
  emit signalDone(this); // always need to emit this if not continuing with the search
}

Tellico::Fetch::ConfigWidget* ISBNdbFetcher::configWidget(QWidget* parent_) const {
  return new ISBNdbFetcher::ConfigWidget(parent_, this);
}

ISBNdbFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const ISBNdbFetcher* /*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

QString ISBNdbFetcher::ConfigWidget::preferredName() const {
  return ISBNdbFetcher::defaultName();
}

#include "isbndbfetcher.moc"
