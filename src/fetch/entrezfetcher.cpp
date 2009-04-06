/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "entrezfetcher.h"
#include "searchresult.h"
#include "../tellico_kernel.h"
#include "../collection.h"
#include "../entry.h"
#include "../core/filehandler.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
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

//#define ENTREZ_TEST

namespace {
  static const int ENTREZ_MAX_RETURNS_TOTAL = 25;
  static const char* ENTREZ_BASE_URL = "http://eutils.ncbi.nlm.nih.gov/entrez/eutils/";
  static const char* ENTREZ_SEARCH_CGI = "esearch.fcgi";
  static const char* ENTREZ_SUMMARY_CGI = "esummary.fcgi";
  static const char* ENTREZ_FETCH_CGI = "efetch.fcgi";
  static const char* ENTREZ_LINK_CGI = "elink.fcgi";
  static const char* ENTREZ_DEFAULT_DATABASE = "pubmed";
}

using Tellico::Fetch::EntrezFetcher;

EntrezFetcher::EntrezFetcher(QObject* parent_) : Fetcher(parent_), m_xsltHandler(0),
    m_step(Begin), m_started(false) {
}

EntrezFetcher::~EntrezFetcher() {
}

QString EntrezFetcher::defaultName() {
  return i18n("Entrez Database");
}

QString EntrezFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool EntrezFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void EntrezFetcher::readConfigHook(const KConfigGroup& config_) {
  QString s = config_.readEntry("Database", ENTREZ_DEFAULT_DATABASE); // default to pubmed
  if(!s.isEmpty()) {
    m_dbname = s;
  }
  m_fields = config_.readEntry("Custom Fields", QStringList());
}

void EntrezFetcher::search(Tellico::Fetch::FetchKey key_, const QString& value_) {
  m_started = true;
  m_start = 1;
  m_total = -1;

// only search if current collection is a bibliography
  if(!canFetch(Kernel::self()->collectionType())) {
    myDebug() << "collection type mismatch, stopping" << endl;
    stop();
    return;
  }
  if(m_dbname.isEmpty()) {
    m_dbname = QLatin1String(ENTREZ_DEFAULT_DATABASE);
  }

#ifdef ENTREZ_TEST
  KUrl u = KUrl("/home/robby/esearch.xml");
#else
  KUrl u(ENTREZ_BASE_URL);
  u.addPath(QLatin1String(ENTREZ_SEARCH_CGI));
  u.addQueryItem(QLatin1String("tool"),       QLatin1String("Tellico"));
  u.addQueryItem(QLatin1String("retmode"),    QLatin1String("xml"));
  u.addQueryItem(QLatin1String("usehistory"), QLatin1String("y"));
  u.addQueryItem(QLatin1String("retmax"),     QLatin1String("1")); // we're just getting the count
  u.addQueryItem(QLatin1String("db"),         m_dbname);
  u.addQueryItem(QLatin1String("term"),       value_);
  switch(key_) {
    case Title:
      u.addQueryItem(QLatin1String("field"), QLatin1String("titl"));
      break;

    case Person:
      u.addQueryItem(QLatin1String("field"), QLatin1String("auth"));
      break;

    case Keyword:
      // for Tellico Keyword searches basically mean search for any field matching
//      u.addQueryItem(QLatin1String("field"), QLatin1String("word"));
      break;

    case PubmedID:
      u.addQueryItem(QLatin1String("field"), QLatin1String("pmid"));
      break;

    case DOI:
    case Raw:
      u.setQuery(u.query() + QLatin1Char('&') + value_);
      break;

    default:
      myWarning() << "EntrezFetcher::search() - FetchKey not supported";
      stop();
      return;
  }
#endif

  m_step = Search;
//  myLog() << "EntrezFetcher::doSearch() - url: " << u.url() << endl;
  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(Kernel::self()->widget());
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
    myDebug() << "no data" << endl;
    stop();
    return;
  }

#if 0
  myWarning() << "Remove debug from entrezfetcher.cpp: " << __LINE__;
  QFile f(QLatin1String("/tmp/test.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setEncoding(QTextStream::UnicodeUTF8);
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
      myLog() << "wrong step =" << m_step << endl;
      stop();
      break;
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;
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
#ifdef ENTREZ_TEST
  KUrl u = KUrl(QLatin1String("/home/robby/esummary.xml"));
#else
  KUrl u(ENTREZ_BASE_URL);
  u.addPath(QLatin1String(ENTREZ_SUMMARY_CGI));
  u.addQueryItem(QLatin1String("tool"),       QLatin1String("Tellico"));
  u.addQueryItem(QLatin1String("retmode"),    QLatin1String("xml"));
  u.addQueryItem(QLatin1String("retstart"),   QString::number(m_start));
  u.addQueryItem(QLatin1String("retmax"),     QString::number(qMin(m_total-m_start-1, ENTREZ_MAX_RETURNS_TOTAL)));
  u.addQueryItem(QLatin1String("usehistory"), QLatin1String("y"));
  u.addQueryItem(QLatin1String("db"),         m_dbname);
  u.addQueryItem(QLatin1String("query_key"),  m_queryKey);
  u.addQueryItem(QLatin1String("WebEnv"),     m_webEnv);
#endif

  m_step = Summary;
//  myLog() << "url:" << u.url() << endl;
  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(Kernel::self()->widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
}

void EntrezFetcher::summaryResults(const QByteArray& data_) {
  QDomDocument dom;
  if(!dom.setContent(data_, false)) {
    myWarning() << "EntrezFetcher::summaryResults() - server did not return valid XML.";
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
      myDebug() << "no Id elements" << endl;
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
        authors = list.join(QLatin1String("; "));
      }
      if(!title.isEmpty() && !pubdate.isEmpty() && !authors.isEmpty()) {
        break; // done now
      }
    }
    SearchResult* r = new SearchResult(Fetcher::Ptr(this), title, pubdate + QLatin1Char('/') + authors);
    m_matches.insert(r->uid, id);
    emit signalResultFound(r);
  }
  m_start = m_matches.count() + 1;
  m_hasMoreResults = m_start <= m_total;
  stop(); // done searching
}

Tellico::Data::EntryPtr EntrezFetcher::fetchEntry(uint uid_) {
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
#ifdef ENTREZ_TEST
  KUrl u = KUrl(QLatin1String("/home/robby/pubmed.xml"));
#else
  KUrl u(ENTREZ_BASE_URL);
  u.addPath(QLatin1String(ENTREZ_FETCH_CGI));
  u.addQueryItem(QLatin1String("tool"),       QLatin1String("Tellico"));
  u.addQueryItem(QLatin1String("retmode"),    QLatin1String("xml"));
  u.addQueryItem(QLatin1String("rettype"),    QLatin1String("abstract"));
  u.addQueryItem(QLatin1String("db"),         m_dbname);
  u.addQueryItem(QLatin1String("id"),         QString::number(id));
#endif
  // now it's sychronous, and we know that it's utf8
  QString xmlOutput = FileHandler::readTextFile(u, false /*quiet*/, true /*utf8*/);
  if(xmlOutput.isEmpty()) {
    myWarning() << "EntrezFetcher::fetchEntry() - unable to download " << u;
    return Data::EntryPtr();
  }
#if 0
  myWarning() << "EntrezFetcher::fetchEntry() - turn me off!";
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
    myDebug() << "no entries in collection" << endl;
    return Data::EntryPtr();
  } else if(coll->entryCount() > 1) {
    myDebug() << "collection has multiple entries, taking first one" << endl;
  }

  Data::EntryPtr e = coll->entries().front();

  // try to get a link, but only if necessary
  if(m_fields.contains(QLatin1String("url"))) {
    KUrl link(ENTREZ_BASE_URL);
    link.addPath(QLatin1String(ENTREZ_LINK_CGI));
    link.addQueryItem(QLatin1String("tool"),   QLatin1String("Tellico"));
    link.addQueryItem(QLatin1String("cmd"),    QLatin1String("llinks"));
    link.addQueryItem(QLatin1String("db"),     m_dbname);
    link.addQueryItem(QLatin1String("dbfrom"), m_dbname);
    link.addQueryItem(QLatin1String("id"),     QString::number(id));

    QDomDocument linkDom = FileHandler::readXMLFile(link, false /* namespace */, true /* quiet */);
    // need eLinkResult/LinkSet/IdUrlList/IdUrlSet/ObjUrl/Url
    QDomNode linkNode = linkDom.namedItem(QLatin1String("eLinkResult"))
                               .namedItem(QLatin1String("LinkSet"))
                               .namedItem(QLatin1String("IdUrlList"))
                               .namedItem(QLatin1String("IdUrlSet"))
                               .namedItem(QLatin1String("ObjUrl"))
                               .namedItem(QLatin1String("Url"));
    if(!linkNode.isNull()) {
      QString u = linkNode.toElement().text();
//      myDebug() << u << endl;
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

  const StringMap customFields = EntrezFetcher::customFields();
  for(StringMap::ConstIterator it = customFields.begin(); it != customFields.end(); ++it) {
    if(!m_fields.contains(it.key())) {
      coll->removeField(it.key());
    }
  }

  m_entries.insert(uid_, e);
  return e;
}

void EntrezFetcher::initXSLTHandler() {
  QString xsltfile = KStandardDirs::locate("appdata", QLatin1String("pubmed2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate pubmed2tellico.xsl.";
    return;
  }

  KUrl u;
  u.setPath(xsltfile);

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

void EntrezFetcher::updateEntry(Tellico::Data::EntryPtr entry_) {
//  myDebug() << endl;
  QString s = entry_->field(QLatin1String("pmid"));
  if(!s.isEmpty()) {
    search(PubmedID, s);
    return;
  }

  s = entry_->field(QLatin1String("doi"));
  if(!s.isEmpty()) {
    search(DOI, s);
    return;
  }

  s = entry_->field(QLatin1String("title"));
  if(!s.isEmpty()) {
    search(Title, s);
    return;
  }

  myDebug() << "insufficient info to search" << endl;
  emit signalDone(this); // always need to emit this if not continuing with the search
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
  addFieldsWidget(EntrezFetcher::customFields(), fetcher_ ? fetcher_->m_fields : QStringList());
}

void EntrezFetcher::ConfigWidget::saveConfig(KConfigGroup& config_) {
  saveFieldsConfig(config_);
  slotSetModified(false);
}

QString EntrezFetcher::ConfigWidget::preferredName() const {
  return EntrezFetcher::defaultName();
}

//static
Tellico::StringMap EntrezFetcher::customFields() {
  StringMap map;
  map[QLatin1String("institution")] = i18n("Institution");
  map[QLatin1String("abstract")]    = i18n("Abstract");
  map[QLatin1String("url")]         = i18n("URL");
  return map;
}

#include "entrezfetcher.moc"
