/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#include "entrezfetcher.h"
#include "../utils/guiproxy.h"
#include "../collection.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KConfigGroup>
#include <KJobWidgets/KJobWidgets>

#include <QDomDocument>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QUrlQuery>

namespace {
  static const int ENTREZ_MAX_RETURNS_TOTAL = 25;
  static const char* ENTREZ_BASE_URL = "http://eutils.ncbi.nlm.nih.gov/entrez/eutils/";
  static const char* ENTREZ_SEARCH_CGI = "esearch.fcgi";
  static const char* ENTREZ_SUMMARY_CGI = "esummary.fcgi";
  static const char* ENTREZ_FETCH_CGI = "efetch.fcgi";
  static const char* ENTREZ_LINK_CGI = "elink.fcgi";
  static const char* ENTREZ_DEFAULT_DATABASE = "pubmed";
}

using namespace Tellico;
using namespace Tellico::Fetch;
using Tellico::Fetch::EntrezFetcher;

EntrezFetcher::EntrezFetcher(QObject* parent_) : Fetcher(parent_), m_xsltHandler(0),
    m_start(1), m_total(-1), m_step(Begin), m_started(false) {
}

EntrezFetcher::~EntrezFetcher() {
}

QString EntrezFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool EntrezFetcher::canSearch(FetchKey k) const {
  return k == Title || k == Person || k == Keyword || k == Raw || k == PubmedID || k == DOI;
}

bool EntrezFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void EntrezFetcher::readConfigHook(const KConfigGroup& config_) {
  QString s = config_.readEntry("Database", ENTREZ_DEFAULT_DATABASE); // default to pubmed
  if(!s.isEmpty()) {
    m_dbname = s;
  }
}

void EntrezFetcher::search() {
  m_started = true;
  m_start = 1;
  m_total = -1;

  if(m_dbname.isEmpty()) {
    m_dbname = QLatin1String(ENTREZ_DEFAULT_DATABASE);
  }

  QUrl u(QString::fromLatin1(ENTREZ_BASE_URL));
  u.setPath(u.path() + QLatin1String(ENTREZ_SEARCH_CGI));
  QUrlQuery q;
  q.addQueryItem(QLatin1String("tool"),       QLatin1String("Tellico"));
  q.addQueryItem(QLatin1String("retmode"),    QLatin1String("xml"));
  q.addQueryItem(QLatin1String("usehistory"), QLatin1String("y"));
  q.addQueryItem(QLatin1String("retmax"),     QLatin1String("1")); // we're just getting the count
  q.addQueryItem(QLatin1String("db"),         m_dbname);
  q.addQueryItem(QLatin1String("term"),       request().value);
  switch(request().key) {
    case Title:
      q.addQueryItem(QLatin1String("field"), QLatin1String("titl"));
      break;

    case Person:
      q.addQueryItem(QLatin1String("field"), QLatin1String("auth"));
      break;

    case Keyword:
      // for Tellico Keyword searches basically mean search for any field matching
//      q.addQueryItem(QLatin1String("field"), QLatin1String("word"));
      break;

    case PubmedID:
      q.addQueryItem(QLatin1String("field"), QLatin1String("pmid"));
      break;

    case DOI:
    case Raw:
      q.setQuery(u.query() + QLatin1Char('&') + request().value);
      break;

    default:
      myWarning() << "key not supported:" << request().key;
      stop();
      return;
  }
  u.setQuery(q);

  m_step = Search;
//  myLog() << "url: " << u.url();
  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
}

void EntrezFetcher::continueSearch() {
  m_started = true;
  doSummary();
}

void EntrezFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }
  m_started = false;
  m_step = Begin;
  emit signalDone(this);
}

void EntrezFetcher::slotComplete(KJob*) {
  Q_ASSERT(m_job);
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
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = 0;

#if 0
  myWarning() << "Remove debug from entrezfetcher.cpp: " << __LINE__;
  QFile f(QLatin1String("/tmp/test.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  switch(m_step) {
    case Search:
      searchResults(data);
      break;
    case Summary:
      summaryResults(data);
      break;
    case Begin:
    case Fetch:
    default:
      myLog() << "wrong step =" << m_step;
      stop();
      break;
  }
}

void EntrezFetcher::searchResults(const QByteArray& data_) {
  QDomDocument dom;
  if(!dom.setContent(data_, false)) {
    myWarning() << "server did not return valid XML.";
    stop();
    return;
  }
  // find Count, QueryKey, and WebEnv elements
  int count = 0;
  for(QDomNode n = dom.documentElement().firstChild(); !n.isNull(); n = n.nextSibling()) {
    QDomElement e = n.toElement();
    if(e.isNull()) {
      continue;
    }
    if(e.tagName() == QLatin1String("Count")) {
      m_total = e.text().toInt();
      ++count;
    } else if(e.tagName() == QLatin1String("QueryKey")) {
      m_queryKey = e.text();
      ++count;
    } else if(e.tagName() == QLatin1String("WebEnv")) {
      m_webEnv = e.text();
      ++count;
    }
    if(count >= 3) {
      break; // found them all
    }
  }

  doSummary();
}

void EntrezFetcher::doSummary() {
  QUrl u(QString::fromLatin1(ENTREZ_BASE_URL));
  u.setPath(u.path() + QLatin1String(ENTREZ_SUMMARY_CGI));
  QUrlQuery q;
  q.addQueryItem(QLatin1String("tool"),       QLatin1String("Tellico"));
  q.addQueryItem(QLatin1String("retmode"),    QLatin1String("xml"));
  if(m_start > 1) {
    q.addQueryItem(QLatin1String("retstart"),   QString::number(m_start));
  }
  q.addQueryItem(QLatin1String("retmax"),     QString::number(qMin(m_total-m_start-1, ENTREZ_MAX_RETURNS_TOTAL)));
  q.addQueryItem(QLatin1String("usehistory"), QLatin1String("y"));
  q.addQueryItem(QLatin1String("db"),         m_dbname);
  q.addQueryItem(QLatin1String("query_key"),  m_queryKey);
  q.addQueryItem(QLatin1String("WebEnv"),     m_webEnv);
  u.setQuery(q);

  m_step = Summary;
//  myLog() << "url:" << u.url();
  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
}

void EntrezFetcher::summaryResults(const QByteArray& data_) {
  QDomDocument dom;
  if(!dom.setContent(data_, false)) {
    myWarning() << "server did not return valid XML.";
    stop();
    return;
  }
  // top child is eSummaryResult
  // all children are DocSum
  for(QDomNode n = dom.documentElement().firstChild(); !n.isNull(); n = n.nextSibling()) {
    QDomElement e = n.toElement();
    if(e.isNull() || e.tagName() != QLatin1String("DocSum")) {
      continue;
    }
    QDomNodeList nodes = e.elementsByTagName(QLatin1String("Id"));
    if(nodes.count() == 0) {
      myDebug() << "no Id elements";
      continue;
    }
    int id = nodes.item(0).toElement().text().toInt();
    QString title, pubdate, authors;
    nodes = e.elementsByTagName(QLatin1String("Item"));
    for(int j = 0; j < nodes.count(); ++j) {
      if(nodes.item(j).toElement().attribute(QLatin1String("Name")) == QLatin1String("Title")) {
        title = nodes.item(j).toElement().text();
      } else if(nodes.item(j).toElement().attribute(QLatin1String("Name")) == QLatin1String("PubDate")) {
        pubdate = nodes.item(j).toElement().text();
      } else if(nodes.item(j).toElement().attribute(QLatin1String("Name")) == QLatin1String("AuthorList")) {
        QStringList list;
        for(QDomNode aNode = nodes.item(j).firstChild(); !aNode.isNull(); aNode = aNode.nextSibling()) {
          // lazy, assume all children Items are authors
          if(aNode.nodeName() == QLatin1String("Item")) {
            list << aNode.toElement().text();
          }
        }
        authors = list.join(FieldFormat::delimiterString());
      }
      if(!title.isEmpty() && !pubdate.isEmpty() && !authors.isEmpty()) {
        break; // done now
      }
    }
    FetchResult* r = new FetchResult(Fetcher::Ptr(this), title, pubdate + QLatin1Char('/') + authors);
    m_matches.insert(r->uid, id);
    emit signalResultFound(r);
  }
  m_start = m_matches.count() + 1;
  m_hasMoreResults = m_start <= m_total;
  stop(); // done searching
}

Tellico::Data::EntryPtr EntrezFetcher::fetchEntryHook(uint uid_) {
  // if we already grabbed this one, then just pull it out of the dict
  Data::EntryPtr entry = m_entries[uid_];
  if(entry) {
    return entry;
  }

  if(!m_matches.contains(uid_)) {
    return Data::EntryPtr();
  }

  if(!m_xsltHandler) {
    initXSLTHandler();
    if(!m_xsltHandler) { // probably an error somewhere in the stylesheet loading
      stop();
      return Data::EntryPtr();
    }
  }

  int id = m_matches[uid_];

  QUrl u(QString::fromLatin1(ENTREZ_BASE_URL));
  u.setPath(u.path() + QLatin1String(ENTREZ_FETCH_CGI));
  QUrlQuery q;
  q.addQueryItem(QLatin1String("tool"),       QLatin1String("Tellico"));
  q.addQueryItem(QLatin1String("retmode"),    QLatin1String("xml"));
  q.addQueryItem(QLatin1String("rettype"),    QLatin1String("abstract"));
  q.addQueryItem(QLatin1String("db"),         m_dbname);
  q.addQueryItem(QLatin1String("id"),         QString::number(id));
  u.setQuery(q);

  // now it's sychronous
  QString xmlOutput = FileHandler::readXMLFile(u, true /*quiet*/);
  if(xmlOutput.isEmpty()) {
    myWarning() << "unable to download " << u;
    return Data::EntryPtr();
  }
#if 0
  myWarning() << "turn me off!";
  QFile f1(QLatin1String("/tmp/test-entry.xml"));
  if(f1.open(QIODevice::WriteOnly)) {
    QTextStream t(&f1);
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << xmlOutput;
  }
  f1.close();
#endif
  QString str = m_xsltHandler->applyStylesheet(xmlOutput);
  Import::TellicoImporter imp(str);
  Data::CollPtr coll = imp.collection();
  if(!coll) {
    myWarning() << "invalid collection";
    return Data::EntryPtr();
  }
  if(coll->entryCount() == 0) {
    myDebug() << "no entries in collection";
    return Data::EntryPtr();
  } else if(coll->entryCount() > 1) {
    myDebug() << "collection has multiple entries, taking first one";
  }

  Data::EntryPtr e = coll->entries().front();

  // try to get a link, but only if necessary
  if(optionalFields().contains(QLatin1String("url"))) {
    QUrl link(QString::fromLatin1(ENTREZ_BASE_URL));
    link.setPath(link.path() + QLatin1String(ENTREZ_LINK_CGI));
    QUrlQuery q;
    q.addQueryItem(QLatin1String("tool"),   QLatin1String("Tellico"));
    q.addQueryItem(QLatin1String("cmd"),    QLatin1String("llinks"));
    q.addQueryItem(QLatin1String("db"),     m_dbname);
    q.addQueryItem(QLatin1String("dbfrom"), m_dbname);
    q.addQueryItem(QLatin1String("id"),     QString::number(id));
    link.setQuery(q);

    QDomDocument linkDom = FileHandler::readXMLDocument(link, false /* namespace */, true /* quiet */);
    // need eLinkResult/LinkSet/IdUrlList/IdUrlSet/ObjUrl/Url
    QDomNode linkNode = linkDom.namedItem(QLatin1String("eLinkResult"))
                               .namedItem(QLatin1String("LinkSet"))
                               .namedItem(QLatin1String("IdUrlList"))
                               .namedItem(QLatin1String("IdUrlSet"))
                               .namedItem(QLatin1String("ObjUrl"))
                               .namedItem(QLatin1String("Url"));
    if(!linkNode.isNull()) {
      QString u = linkNode.toElement().text();
//      myDebug() << u;
      if(!u.isEmpty()) {
        if(!coll->hasField(QLatin1String("url"))) {
          Data::FieldPtr field(new Data::Field(QLatin1String("url"), i18n("URL"), Data::Field::URL));
          field->setCategory(i18n("Miscellaneous"));
          coll->addField(field);
        }
        e->setField(QLatin1String("url"), u);
      }
    }
  }

  m_entries.insert(uid_, e);
  return e;
}

void EntrezFetcher::initXSLTHandler() {
  QString xsltfile = DataFileRegistry::self()->locate(QLatin1String("pubmed2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate pubmed2tellico.xsl.";
    return;
  }

  QUrl u = QUrl::fromLocalFile(xsltfile);

  if(!m_xsltHandler) {
    m_xsltHandler = new XSLTHandler(u);
  }
  if(!m_xsltHandler->isValid()) {
    myWarning() << "error in pubmed2tellico.xsl.";
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

Tellico::Fetch::FetchRequest EntrezFetcher::updateRequest(Data::EntryPtr entry_) {
//  myDebug();
  QString s = entry_->field(QLatin1String("pmid"));
  if(!s.isEmpty()) {
    return FetchRequest(PubmedID, s);
  }

  s = entry_->field(QLatin1String("doi"));
  if(!s.isEmpty()) {
    return FetchRequest(DOI, s);
  }

  s = entry_->field(QLatin1String("title"));
  if(!s.isEmpty()) {
    return FetchRequest(Title, s);
  }
  return FetchRequest();
}

QString EntrezFetcher::defaultName() {
  return i18n("Entrez Database");
}

QString EntrezFetcher::defaultIcon() {
  return favIcon("http://www.ncbi.nlm.nih.gov");
}

//static
Tellico::StringHash EntrezFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("institution")] = i18n("Institution");
  hash[QLatin1String("abstract")]    = i18n("Abstract");
  hash[QLatin1String("url")]         = i18n("URL");
  return hash;
}

Tellico::Fetch::ConfigWidget* EntrezFetcher::configWidget(QWidget* parent_) const {
  return new EntrezFetcher::ConfigWidget(parent_, this);
}

EntrezFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const EntrezFetcher* fetcher_/*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(EntrezFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString EntrezFetcher::ConfigWidget::preferredName() const {
  return EntrezFetcher::defaultName();
}

