/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>
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

#include "isbndbfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../gui/guiproxy.h"
#include "../tellico_utils.h"
#include "../collection.h"
#include "../entry.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kstandarddirs.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <KConfigGroup>
#include <klineedit.h>

#include <QDomDocument>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QTextCodec>

namespace {
  static const int ISBNDB_RETURNS_PER_REQUEST = 10;
  static const int ISBNDB_MAX_RETURNS_TOTAL = 25;
  static const char* ISBNDB_BASE_URL = "http://isbndb.com/api/books.xml";
  static const char* ISBNDB_APP_ID = "3B9S3BQS";
}

using namespace Tellico;
using Tellico::Fetch::ISBNdbFetcher;

ISBNdbFetcher::ISBNdbFetcher(QObject* parent_)
    : Fetcher(parent_), m_xsltHandler(0),
      m_limit(ISBNDB_MAX_RETURNS_TOTAL), m_page(1), m_total(-1), m_countOffset(0),
      m_job(0), m_started(false), m_apiKey(QLatin1String(ISBNDB_APP_ID)) {
}

ISBNdbFetcher::~ISBNdbFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = 0;
}

QString ISBNdbFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool ISBNdbFetcher::canFetch(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::ComicBook || type == Data::Collection::Bibtex;
}

void ISBNdbFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key", ISBNDB_APP_ID);
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
}

void ISBNdbFetcher::search() {
  m_started = true;
  m_page = 1;
  m_total = -1;
  m_numResults = 0;
  m_countOffset = 0;

  doSearch();
}

void ISBNdbFetcher::continueSearch() {
  m_started = true;
  m_limit += ISBNDB_MAX_RETURNS_TOTAL;
  doSearch();
}

void ISBNdbFetcher::doSearch() {
//  myDebug() << "value = " << value_;

  KUrl u(ISBNDB_BASE_URL);
  u.addQueryItem(QLatin1String("access_key"), m_apiKey);
  u.addQueryItem(QLatin1String("results"), QLatin1String("details,authors,subjects,texts"));
  u.addQueryItem(QLatin1String("page_number"), QString::number(m_page));

  switch(request().key) {
    case Title:
      u.addQueryItem(QLatin1String("index1"), QLatin1String("title"));
      u.addQueryItem(QLatin1String("value1"), request().value);
      break;

    case Person:
      // yes, this also queries titles, too, it's a limitation of the isbndb api service
      u.addQueryItem(QLatin1String("index1"), QLatin1String("combined"));
      u.addQueryItem(QLatin1String("value1"), request().value);
      break;

    case Keyword:
      u.addQueryItem(QLatin1String("index1"), QLatin1String("full"));
      u.addQueryItem(QLatin1String("value1"), request().value);
      break;

    case ISBN:
      u.addQueryItem(QLatin1String("index1"), QLatin1String("isbn"));
      {
        // only grab first value
        QString v = request().value.section(QLatin1Char(';'), 0);
        v.remove(QLatin1Char('-'));
        u.addQueryItem(QLatin1String("value1"), v);
      }
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      stop();
      return;
  }
//  myDebug() << "url: " << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
}

void ISBNdbFetcher::stop() {
  if(!m_started) {
    return;
  }
//  myDebug();
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }

  m_started = false;
  emit signalDone(this);
}

void ISBNdbFetcher::slotComplete(KJob*) {
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
#if 0
  myWarning() << "Remove debug from isbndbfetcher.cpp";
  QFile f(QLatin1String("/tmp/test.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec(QTextCodec::codecForName("UTF-8"));
    t << data;
  }
  f.close();
#endif

  QDomDocument dom;
  if(!dom.setContent(data, false)) {
    myWarning() << "server did not return valid XML.";
    return;
  }

  if(m_total == -1) {
    QDomNode n = dom.documentElement().namedItem(QLatin1String("BookList"));
    QDomElement e = n.toElement();
    if(!e.isNull()) {
      m_total = e.attribute(QLatin1String("total_results"), QString::number(-1)).toInt();
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

  if(coll->entryCount() == 0) {
    QDomNode n = dom.documentElement().namedItem(QLatin1String("ErrorMessage"));
    QDomElement e = n.toElement();
    if(!e.isNull() && e.text() == QLatin1String("Access key error")) {
      message(i18n("<qt><p><b>The ISBNndb.com server reports an access key error.</b></p>"
                   "You may have reached the maximum number of searches for today with this key, "
                   "or you may have entered the access key incorrectly.</qt>"), MessageHandler::Error);
      stop();
      return;
    }
  }

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

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
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

Tellico::Data::EntryPtr ISBNdbFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries[uid_];
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  // if the publisher id is set, then we need to grab the real publisher name
  const QString id = entry->field(QLatin1String("pub_id"));
  if(!id.isEmpty()) {
    KUrl u(ISBNDB_BASE_URL);
    u.setFileName(QLatin1String("publishers.xml"));
    u.addQueryItem(QLatin1String("access_key"), m_apiKey);
    u.addQueryItem(QLatin1String("index1"), QLatin1String("publisher_id"));
    u.addQueryItem(QLatin1String("value1"), id);

    QDomDocument dom = FileHandler::readXMLDocument(u, true);
    if(!dom.isNull()) {
      QString pub = dom.documentElement().namedItem(QLatin1String("PublisherList"))
                                         .namedItem(QLatin1String("PublisherData"))
                                         .namedItem(QLatin1String("Name"))
                                         .toElement().text();
      if(!pub.isEmpty()) {
        entry->setField(QLatin1String("publisher"), pub);
      }
    }
    entry->setField(QLatin1String("pub_id"), QString());
  }

  return entry;
}

void ISBNdbFetcher::initXSLTHandler() {
  QString xsltfile = KStandardDirs::locate("appdata", QLatin1String("isbndb2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate isbndb2tellico.xsl.";
    return;
  }

  KUrl u;
  u.setPath(xsltfile);

  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    myWarning() << "error in isbndb2tellico.xsl.";
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

Tellico::Fetch::FetchRequest ISBNdbFetcher::updateRequest(Data::EntryPtr entry_) {
  QString isbn = entry_->field(QLatin1String("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(Fetch::ISBN, isbn);
  }

  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  QString t = entry_->field(QLatin1String("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* ISBNdbFetcher::configWidget(QWidget* parent_) const {
  return new ISBNdbFetcher::ConfigWidget(parent_, this);
}

QString ISBNdbFetcher::defaultName() {
  return i18n("ISBNdb.com");
}

QString ISBNdbFetcher::defaultIcon() {
  return favIcon("http://isbndb.com");
}

ISBNdbFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const ISBNdbFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;
  QLabel* al = new QLabel(i18n("Registration is required for accessing the %1 data source. "
                               "If you agree to the terms and conditions, <a href='%2'>sign "
                               "up for an account</a>, and enter your information below.",
                                preferredName(),
                                QLatin1String("http://isbndb.com/docs/api/30-keys.html")),
                          optionsWidget());
  al->setOpenExternalLinks(true);
  al->setWordWrap(true);
  ++row;
  l->addWidget(al, row, 0, 1, 2);
  // richtext gets weird with size
  al->setMinimumWidth(al->sizeHint().width());

  QLabel* label = new QLabel(i18n("Access key: "), optionsWidget());
  l->addWidget(label, ++row, 0);

  m_apiKeyEdit = new KLineEdit(optionsWidget());
  connect(m_apiKeyEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_apiKeyEdit, row, 1);
  QString w = i18n("The default Tellico key may be used, but searching may fail due to reaching access limits.");
  label->setWhatsThis(w);
  m_apiKeyEdit->setWhatsThis(w);
  label->setBuddy(m_apiKeyEdit);

  l->setRowStretch(++row, 10);

  if(fetcher_) {
    // only show the key if it is not the default Tellico one...
    // that way the user is prompted to apply for their own
    if(fetcher_->m_apiKey != QLatin1String(ISBNDB_APP_ID)) {
      m_apiKeyEdit->setText(fetcher_->m_apiKey);
    }
  }
}

void ISBNdbFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
}

QString ISBNdbFetcher::ConfigWidget::preferredName() const {
  return ISBNdbFetcher::defaultName();
}

#include "isbndbfetcher.moc"
