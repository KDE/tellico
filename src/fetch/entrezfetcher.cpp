/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
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
#include "../tellico_kernel.h"
#include "../latin1literal.h"
#include "../collection.h"
#include "../entry.h"
#include "../filehandler.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"

#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>

#include <qdom.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qfile.h>

//#define ENTREZ_TEST

namespace {
  static const int ENTREZ_MAX_RETURNS_TOTAL = 50;
  static const char* ENTREZ_BASE_URL = "http://eutils.ncbi.nlm.nih.gov/entrez/eutils/";
  static const char* ENTREZ_SEARCH_CGI = "esearch.fcgi";
  static const char* ENTREZ_SUMMARY_CGI = "esummary.fcgi";
  static const char* ENTREZ_FETCH_CGI = "efetch.fcgi";
  static const char* ENTREZ_LINK_CGI = "elink.fcgi";
  static const char* ENTREZ_DEFAULT_DATABASE = "pubmed";
}

using Tellico::Fetch::EntrezFetcher;
Tellico::XSLTHandler* EntrezFetcher::s_xsltHandler = 0;

EntrezFetcher::EntrezFetcher(QObject* parent_, const char* name_) : Fetcher(parent_, name_),
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

void EntrezFetcher::readConfigHook(KConfig* config_, const QString& group_) {
  KConfigGroupSaver groupSaver(config_, group_);
  QString s = config_->readEntry("Database", QString::fromLatin1("pubmed")); // default to pubmed
  if(!s.isEmpty()) {
    m_dbname = s;
  }
  m_fields = config_->readListEntry("Custom Fields");
}

void EntrezFetcher::search(FetchKey key_, const QString& value_) {
  m_started = true;
// only search if current collection is a bibliography
  if(Kernel::self()->collectionType() != Data::Collection::Bibtex) {
    myDebug() << "EntrezFetcher::search() - collection type mismatch, stopping" << endl;
    stop();
    return;
  }
  if(m_dbname.isEmpty()) {
    m_dbname = QString::fromLatin1(ENTREZ_DEFAULT_DATABASE);
  }

  m_started = true;
  m_data.truncate(0);
  m_matches.clear();

#ifdef ENTREZ_TEST
  m_url = KURL::fromPathOrURL(QString::fromLatin1("/home/robby/esearch.xml"));
#else
  m_url = KURL(QString::fromLatin1(ENTREZ_BASE_URL));
  m_url.addPath(QString::fromLatin1(ENTREZ_SEARCH_CGI));
  m_url.addQueryItem(QString::fromLatin1("tool"),       QString::fromLatin1("Tellico"));
  m_url.addQueryItem(QString::fromLatin1("retmode"),    QString::fromLatin1("xml"));
  m_url.addQueryItem(QString::fromLatin1("usehistory"), QString::fromLatin1("y"));
  m_url.addQueryItem(QString::fromLatin1("retmax"),     QString::fromLatin1("1")); // we're just getting the count
  m_url.addQueryItem(QString::fromLatin1("db"),         m_dbname);
  m_url.addQueryItem(QString::fromLatin1("term"),       value_);
  switch(key_) {
    case Title:
      m_url.addQueryItem(QString::fromLatin1("field"), QString::fromLatin1("titl"));
      break;

    case Person:
      m_url.addQueryItem(QString::fromLatin1("field"), QString::fromLatin1("auth"));
      break;

    case Keyword:
      // for Tellico Keyword searches basically mean search for any field matching
//      m_url.addQueryItem(QString::fromLatin1("field"), QString::fromLatin1("word"));
      break;

    case Raw:
      m_url.setQuery(m_url.query() + '&' + value_);
      break;

    default:
      kdWarning() << "EntrezFetcher::search() - FetchKey not supported" << endl;
      stop();
      return;
  }
#endif

  m_step = Search;
//  myDebug() << m_url.prettyURL() << endl;
  m_job = KIO::get(m_url, false, false);
  connect(m_job, SIGNAL(data(KIO::Job*, const QByteArray&)),
          SLOT(slotData(KIO::Job*, const QByteArray&)));
  connect(m_job, SIGNAL(result(KIO::Job*)),
          SLOT(slotComplete(KIO::Job*)));
}

void EntrezFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }
  m_data.truncate(0);
  m_started = false;
  m_step = Begin;
  emit signalDone(this);
}

void EntrezFetcher::slotData(KIO::Job*, const QByteArray& data_) {
  QDataStream stream(m_data, IO_WriteOnly | IO_Append);
  stream.writeRawBytes(data_.data(), data_.size());
}

void EntrezFetcher::slotComplete(KIO::Job* job_) {
  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  if(job_->error()) {
    job_->showErrorDialog(Kernel::self()->widget());
    stop();
    return;
  }

  if(m_data.isEmpty()) {
    myDebug() << "EntrezFetcher::slotComplete() - no data" << endl;
    stop();
    return;
  }

#if 0
  kdWarning() << "Remove debug from entrezfetcher.cpp: " << len << endl;
  QFile f(QString::fromLatin1("/tmp/test.xml"));
  if(f.open(IO_WriteOnly)) {
    QTextStream t(&f);
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << QCString(m_data, m_data.size()+1);
  }
  f.close();
#endif

  switch(m_step) {
    case Search:
      searchResults();
      break;
    case Summary:
      summaryResults();
      break;
    case Begin:
    case Fetch:
    default:
      myDebug() << "EntrezFetcher::slotComplete() - wrong step = " << m_step << endl;
      break;
  }
}

void EntrezFetcher::searchResults() {
  QDomDocument dom;
  if(!dom.setContent(m_data, false)) {
    kdWarning() << "EntrezFetcher::searchResults() - server did not return valid XML." << endl;
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
    if(e.tagName() == Latin1Literal("Count")) {
      m_total = e.text().toInt();
      ++count;
    } else if(e.tagName() == Latin1Literal("QueryKey")) {
      m_queryKey = e.text();
      ++count;
    } else if(e.tagName() == Latin1Literal("WebEnv")) {
      m_webEnv = e.text();
      ++count;
    }
    if(count >= 3) {
      break; // found them all
    }
  }

#ifdef ENTREZ_TEST
  m_url = KURL::fromPathOrURL(QString::fromLatin1("/home/robby/esummary.xml"));
#else
  m_url = KURL(QString::fromLatin1(ENTREZ_BASE_URL));
  m_url.addPath(QString::fromLatin1(ENTREZ_SUMMARY_CGI));
  m_url.addQueryItem(QString::fromLatin1("tool"),       QString::fromLatin1("Tellico"));
  m_url.addQueryItem(QString::fromLatin1("retmode"),    QString::fromLatin1("xml"));
  m_url.addQueryItem(QString::fromLatin1("retstart"),   QString::fromLatin1("1"));
  m_url.addQueryItem(QString::fromLatin1("retmax"),     QString::number(KMIN(m_total, ENTREZ_MAX_RETURNS_TOTAL)));
  m_url.addQueryItem(QString::fromLatin1("usehistory"), QString::fromLatin1("y"));
  m_url.addQueryItem(QString::fromLatin1("db"),         m_dbname);
  m_url.addQueryItem(QString::fromLatin1("query_key"),  m_queryKey);
  m_url.addQueryItem(QString::fromLatin1("WebEnv"),     m_webEnv);
#endif

  m_data.truncate(0);
  m_step = Summary;
//  myDebug() << m_url.prettyURL() << endl;
  m_job = KIO::get(m_url, false, false);
  connect(m_job, SIGNAL(data(KIO::Job*, const QByteArray&)),
          SLOT(slotData(KIO::Job*, const QByteArray&)));
  connect(m_job, SIGNAL(result(KIO::Job*)),
          SLOT(slotComplete(KIO::Job*)));
}

void EntrezFetcher::summaryResults() {
  QDomDocument dom;
  if(!dom.setContent(m_data, false)) {
    kdWarning() << "EntrezFetcher::summaryResults() - server did not return valid XML." << endl;
    stop();
    return;
  }
  // top child is eSummaryResult
  // all children are DocSum
  for(QDomNode n = dom.documentElement().firstChild(); !n.isNull(); n = n.nextSibling()) {
    QDomElement e = n.toElement();
    if(e.isNull() || e.tagName() != Latin1Literal("DocSum")) {
      continue;
    }
    QDomNodeList nodes = e.elementsByTagName(QString::fromLatin1("Id"));
    if(nodes.count() == 0) {
      myDebug() << "EntrezFetcher::summaryResults() - no Id elements" << endl;
      continue;
    }
    int id = nodes.item(0).toElement().text().toInt();
    QString title, pubdate, authors;
    nodes = e.elementsByTagName(QString::fromLatin1("Item"));
    for(uint j = 0; j < nodes.count(); ++j) {
      if(nodes.item(j).toElement().attribute(QString::fromLatin1("Name")) == Latin1Literal("Title")) {
        title = nodes.item(j).toElement().text();
      } else if(nodes.item(j).toElement().attribute(QString::fromLatin1("Name")) == Latin1Literal("PubDate")) {
        pubdate = nodes.item(j).toElement().text();
      } else if(nodes.item(j).toElement().attribute(QString::fromLatin1("Name")) == Latin1Literal("AuthorList")) {
        QStringList list;
        for(QDomNode aNode = nodes.item(j).firstChild(); !aNode.isNull(); aNode = aNode.nextSibling()) {
          // lazy, assume all children Items are authors
          if(aNode.nodeName() == Latin1Literal("Item")) {
            list << aNode.toElement().text();
          }
        }
        authors = list.join(QString::fromLatin1("; "));
      }
      if(!title.isNull() && !pubdate.isNull() && !authors.isNull()) {
        break; // done now
      }
    }
    SearchResult* r = new SearchResult(this, title, pubdate + '/' + authors);
    m_matches.insert(r->uid, id);
    emit signalResultFound(r);
  }
  stop(); // done searching
}

Tellico::Data::EntryPtr EntrezFetcher::fetchEntry(uint uid_) {
  // if we already grabbed this one, then just pull it out of the dict
  Data::EntryPtr entry = m_entries[uid_];
  if(entry) {
    return entry;
  }

  if(!m_matches.contains(uid_)) {
    return 0;
  }

  if(!s_xsltHandler) {
    initXSLTHandler();
    if(!s_xsltHandler) { // probably an error somewhere in the stylesheet loading
      stop();
      return 0;
    }
  }

  int id = m_matches[uid_];
#ifdef ENTREZ_TEST
  m_url = KURL::fromPathOrURL(QString::fromLatin1("/home/robby/pubmed.xml"));
#else
  m_url = KURL(QString::fromLatin1(ENTREZ_BASE_URL));
  m_url.addPath(QString::fromLatin1(ENTREZ_FETCH_CGI));
  m_url.addQueryItem(QString::fromLatin1("tool"),       QString::fromLatin1("Tellico"));
  m_url.addQueryItem(QString::fromLatin1("retmode"),    QString::fromLatin1("xml"));
  m_url.addQueryItem(QString::fromLatin1("rettype"),    QString::fromLatin1("abstract"));
  m_url.addQueryItem(QString::fromLatin1("db"),         m_dbname);
  m_url.addQueryItem(QString::fromLatin1("id"),         QString::number(id));
#endif
#if 1
  kdWarning() << "EntrezFetcher::fetchEntry() - turn me off!" << endl;
  QFile f1(QString::fromLatin1("/tmp/test.xml"));
  if(f1.open(IO_WriteOnly)) {
    QTextStream t(&f1);
    t << FileHandler::readTextFile(m_url);
  }
  f1.close();
#endif
  // now it's sychronous
  QString str = s_xsltHandler->applyStylesheet(FileHandler::readTextFile(m_url));
  Import::TellicoImporter imp(str);
  Data::CollPtr coll = imp.collection();
  if(coll->entryCount() == 0) {
    myDebug() << "EntrezFetcher::fetchEntry() - no entries in collection" << endl;
    return 0;
  } else if(coll->entryCount() > 1) {
    myDebug() << "EntrezFetcher::fetchEntry() - collection has multiple entries, taking first one" << endl;
  }

  Data::EntryPtr e = coll->entries()[0];

  // try to get a link, but only if necessary
  if(m_fields.contains(QString::fromLatin1("url"))) {
    KURL link(QString::fromLatin1(ENTREZ_BASE_URL));
    link.addPath(QString::fromLatin1(ENTREZ_LINK_CGI));
    link.addQueryItem(QString::fromLatin1("tool"),  QString::fromLatin1("Tellico"));
    link.addQueryItem(QString::fromLatin1("cmd"),    QString::fromLatin1("llinks"));
    link.addQueryItem(QString::fromLatin1("db"),     m_dbname);
    link.addQueryItem(QString::fromLatin1("dbfrom"), m_dbname);
    link.addQueryItem(QString::fromLatin1("id"),     QString::number(id));

    QDomDocument linkDom = FileHandler::readXMLFile(link, false /* namespace */, true /* quiet */);
    // need eLinkResult/LinkSet/IdUrlList/IdUrlSet/ObjUrl/Url
    QDomNode linkNode = linkDom.namedItem(QString::fromLatin1("eLinkResult"))
                               .namedItem(QString::fromLatin1("LinkSet"))
                               .namedItem(QString::fromLatin1("IdUrlList"))
                               .namedItem(QString::fromLatin1("IdUrlSet"))
                               .namedItem(QString::fromLatin1("ObjUrl"))
                               .namedItem(QString::fromLatin1("Url"));
    if(!linkNode.isNull()) {
      QString u = linkNode.toElement().text();
      myDebug() << u << endl;
      if(!u.isEmpty()) {
        if(!coll->hasField(QString::fromLatin1("url"))) {
          Data::FieldPtr field = new Data::Field(QString::fromLatin1("url"), i18n("URL"), Data::Field::URL);
          field->setCategory(i18n("Miscellaneous"));
          coll->addField(field);
        }
        e->setField(QString::fromLatin1("url"), u);
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
  QString xsltfile = locate("appdata", QString::fromLatin1("pubmed2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    kdWarning() << "EntrezFetcher::initXSLTHandler() - can not locate pubmed2tellico.xsl." << endl;
    return;
  }

  KURL u;
  u.setPath(xsltfile);

  if(!s_xsltHandler) {
    s_xsltHandler = new XSLTHandler(u);
  }
  if(!s_xsltHandler->isValid()) {
    kdWarning() << "EntrezFetcher::initXSLTHandler() - error in pubmed2tellico.xsl." << endl;
    delete s_xsltHandler;
    s_xsltHandler = 0;
    return;
  }
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

void EntrezFetcher::ConfigWidget::saveConfig(KConfig* config_) {
  saveFieldsConfig(config_);
  slotSetModified(false);
}

QString EntrezFetcher::ConfigWidget::preferredName() const {
  return EntrezFetcher::defaultName();
}

//static
Tellico::StringMap EntrezFetcher::customFields() {
  StringMap map;
  map[QString::fromLatin1("institution")] = i18n("Institution");
  map[QString::fromLatin1("abstract")]    = i18n("Abstract");
  map[QString::fromLatin1("url")]         = i18n("URL");
  return map;
}

#include "entrezfetcher.moc"
