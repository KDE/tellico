/***************************************************************************
    Copyright (C) 2005-2020 Robby Stephenson <robby@periapsis.org>
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
#include <KIO/StoredTransferJob>
#include <KIO/JobUiDelegate>
#include <KConfigGroup>
#include <KJobWidgets>

#include <QDomDocument>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QLineEdit>
#include <QUrlQuery>
#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>

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

EntrezFetcher::EntrezFetcher(QObject* parent_) : Fetcher(parent_), m_xsltHandler(nullptr),
    m_start(1), m_total(-1), m_step(Step::Begin), m_started(false) {
  m_idleTime.start();
}

EntrezFetcher::~EntrezFetcher() {
}

QString EntrezFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool EntrezFetcher::canSearch(Fetch::FetchKey k) const {
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
  QString k = config_.readEntry("API Key");
  if(!k.isEmpty()) {
    m_apiKey = k;
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
  q.addQueryItem(QStringLiteral("tool"),       QStringLiteral("Tellico"));
  q.addQueryItem(QStringLiteral("retmode"),    QStringLiteral("xml"));
  q.addQueryItem(QStringLiteral("usehistory"), QStringLiteral("y"));
  q.addQueryItem(QStringLiteral("retmax"),     QStringLiteral("1")); // we're just getting the count
  q.addQueryItem(QStringLiteral("db"),         m_dbname);
  q.addQueryItem(QStringLiteral("term"),       request().value());
  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("field"), QStringLiteral("titl"));
      break;

    case Person:
      q.addQueryItem(QStringLiteral("field"), QStringLiteral("auth"));
      break;

    case Keyword:
      // for Tellico Keyword searches basically mean search for any field matching
//      q.addQueryItem(QLatin1String("field"), QLatin1String("word"));
      break;

    case PubmedID:
      q.addQueryItem(QStringLiteral("field"), QStringLiteral("pmid"));
      break;

    case DOI:
    case Raw:
      // for DOI, enough to match any field to DOI value
      //q.setQuery(u.query() + QLatin1Char('&') + request().value());
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  if(!m_apiKey.isEmpty()) {
    q.addQueryItem(QStringLiteral("api_key"), m_apiKey);
  }
  u.setQuery(q);

  m_step = Step::Search;
//  myLog() << "search url: " << u.url();
  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result,
          this, &EntrezFetcher::slotComplete);
  markTime();
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
    m_job = nullptr;
  }
  m_started = false;
  m_step = Step::Begin;
  emit signalDone(this);
}

void EntrezFetcher::slotComplete(KJob*) {
  Q_ASSERT(m_job);
  if(m_job->error()) {
    m_job->uiDelegate()->showErrorMessage();
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
  m_job = nullptr;

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
    case Step::Search:
      searchResults(data);
      break;
    case Step::Summary:
      summaryResults(data);
      break;
    case Step::Begin:
    case Step::Fetch:
    default:
      myLog() << "wrong step =" << int(m_step);
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
  q.addQueryItem(QStringLiteral("tool"),       QStringLiteral("Tellico"));
  q.addQueryItem(QStringLiteral("retmode"),    QStringLiteral("xml"));
  if(m_start > 1) {
    q.addQueryItem(QStringLiteral("retstart"),   QString::number(m_start));
  }
  q.addQueryItem(QStringLiteral("retmax"),     QString::number(qMin(m_total-m_start-1, ENTREZ_MAX_RETURNS_TOTAL)));
  q.addQueryItem(QStringLiteral("usehistory"), QStringLiteral("y"));
  q.addQueryItem(QStringLiteral("db"),         m_dbname);
  q.addQueryItem(QStringLiteral("query_key"),  m_queryKey);
  q.addQueryItem(QStringLiteral("WebEnv"),     m_webEnv);
  if(!m_apiKey.isEmpty()) {
    q.addQueryItem(QStringLiteral("api_key"), m_apiKey);
  }
  u.setQuery(q);

  m_step = Step::Summary;
//  myLog() << "summary url:" << u.url();
  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result,
          this, &EntrezFetcher::slotComplete);
  markTime();
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
    QDomNodeList nodes = e.elementsByTagName(QStringLiteral("Id"));
    if(nodes.count() == 0) {
      myDebug() << "no Id elements";
      continue;
    }
    int id = nodes.item(0).toElement().text().toInt();
    QString title, pubdate, authors;
    nodes = e.elementsByTagName(QStringLiteral("Item"));
    for(int j = 0; j < nodes.count(); ++j) {
      const auto elem = nodes.item(j).toElement();
      if(elem.attribute(QStringLiteral("Name")) == QLatin1String("Title")) {
        title = elem.text();
      } else if(elem.attribute(QStringLiteral("Name")) == QLatin1String("PubDate")) {
        pubdate = elem.text();
      } else if(elem.attribute(QStringLiteral("Name")) == QLatin1String("AuthorList")) {
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
    FetchResult* r = new FetchResult(this, title, pubdate + QLatin1Char('/') + authors);
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
  q.addQueryItem(QStringLiteral("tool"),       QStringLiteral("Tellico"));
  q.addQueryItem(QStringLiteral("retmode"),    QStringLiteral("xml"));
  q.addQueryItem(QStringLiteral("rettype"),    QStringLiteral("abstract"));
  q.addQueryItem(QStringLiteral("db"),         m_dbname);
  q.addQueryItem(QStringLiteral("id"),         QString::number(id));
  if(!m_apiKey.isEmpty()) {
    q.addQueryItem(QStringLiteral("api_key"), m_apiKey);
  }
  u.setQuery(q);

  // now it's synchronous
//  myDebug() << "id url:" << u.url();
  markTime();
  QString xmlOutput = FileHandler::readXMLFile(u, true /*quiet*/);
  if(xmlOutput.isEmpty()) {
    myWarning() << "unable to download " << u;
    return Data::EntryPtr();
  }
#if 0
  myWarning() << "turn me off in entrezfetcher.cpp!";
  QFile f1(QLatin1String("/tmp/test-entry.xml"));
  if(f1.open(QIODevice::WriteOnly)) {
    QTextStream t(&f1);
    t.setCodec("UTF-8");
    t << xmlOutput;
  }
  f1.close();
#endif
  QString str = m_xsltHandler->applyStylesheet(xmlOutput);
  if(str.isEmpty()) {
    // might be an API error, and message is in JSON
    QJsonDocument doc = QJsonDocument::fromJson(xmlOutput.toUtf8());
    if(!doc.isNull() && doc.object().contains(QStringLiteral("error"))) {
      const QString error = doc.object().value(QStringLiteral("error")).toString();
      message(error, MessageHandler::Error);
      myLog() << "EntrezFetcher -" << error;
    }
    return Data::EntryPtr();
  }
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
  if(optionalFields().contains(QStringLiteral("url"))) {
    QUrl link(QString::fromLatin1(ENTREZ_BASE_URL));
    link.setPath(link.path() + QLatin1String(ENTREZ_LINK_CGI));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("tool"),   QStringLiteral("Tellico"));
    q.addQueryItem(QStringLiteral("cmd"),    QStringLiteral("llinks"));
    q.addQueryItem(QStringLiteral("db"),     m_dbname);
    q.addQueryItem(QStringLiteral("dbfrom"), m_dbname);
    q.addQueryItem(QStringLiteral("id"),     QString::number(id));
    if(!m_apiKey.isEmpty()) {
      q.addQueryItem(QStringLiteral("api_key"), m_apiKey);
    }
    link.setQuery(q);

    markTime();
    QDomDocument linkDom = FileHandler::readXMLDocument(link, false /* namespace */, true /* quiet */);
    // need eLinkResult/LinkSet/IdUrlList/IdUrlSet/ObjUrl/Url
    QDomNode linkNode = linkDom.namedItem(QStringLiteral("eLinkResult"))
                               .namedItem(QStringLiteral("LinkSet"))
                               .namedItem(QStringLiteral("IdUrlList"))
                               .namedItem(QStringLiteral("IdUrlSet"))
                               .namedItem(QStringLiteral("ObjUrl"))
                               .namedItem(QStringLiteral("Url"));
    if(!linkNode.isNull()) {
      QString u = linkNode.toElement().text();
//      myDebug() << u;
      if(!u.isEmpty()) {
        if(!coll->hasField(QStringLiteral("url"))) {
          Data::FieldPtr field(new Data::Field(QStringLiteral("url"), i18n("URL"), Data::Field::URL));
          field->setCategory(i18n("Miscellaneous"));
          coll->addField(field);
        }
        e->setField(QStringLiteral("url"), u);
      }
    }
  }

  m_entries.insert(uid_, e);
  return e;
}

void EntrezFetcher::initXSLTHandler() {
  QString xsltfile = DataFileRegistry::self()->locate(QStringLiteral("pubmed2tellico.xsl"));
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
    m_xsltHandler = nullptr;
    return;
  }
}

// without an API key, limit is 3 searches per second
// with a key, limit is 10
// https://ncbiinsights.ncbi.nlm.nih.gov/2017/11/02/new-api-keys-for-the-e-utilities/
void EntrezFetcher::markTime() {
  // not exactly the way to monitor rate over 3 or 10 calls, just a constant rate
  const int wait = m_apiKey.isEmpty() ? 350 : 110;
  while(m_idleTime.elapsed() < wait) {
    QThread::msleep(100);
  }
  m_idleTime.restart();
}

Tellico::Fetch::FetchRequest EntrezFetcher::updateRequest(Data::EntryPtr entry_) {
  QString s = entry_->field(QStringLiteral("pmid"));
  if(!s.isEmpty()) {
    return FetchRequest(PubmedID, s);
  }

  s = entry_->field(QStringLiteral("doi"));
  if(!s.isEmpty()) {
    return FetchRequest(DOI, s);
  }

  s = entry_->field(QStringLiteral("title"));
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
  hash[QStringLiteral("institution")] = i18n("Institution");
  hash[QStringLiteral("abstract")]    = i18n("Abstract");
  hash[QStringLiteral("url")]         = i18n("URL");
  return hash;
}

Tellico::Fetch::ConfigWidget* EntrezFetcher::configWidget(QWidget* parent_) const {
  return new EntrezFetcher::ConfigWidget(parent_, this);
}

EntrezFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const EntrezFetcher* fetcher_/*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* label = new QLabel(i18n("Access key: "), optionsWidget());
  l->addWidget(label, ++row, 0);

  m_apiKeyEdit = new QLineEdit(optionsWidget());
  connect(m_apiKeyEdit, &QLineEdit::textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_apiKeyEdit, row, 1);
  QString w = i18n("The default Tellico key may be used, but searching may fail due to reaching access limits.");
  label->setWhatsThis(w);
  m_apiKeyEdit->setWhatsThis(w);
  label->setBuddy(m_apiKeyEdit);

  l->setRowStretch(++row, 10);

  // now add additional fields widget
  addFieldsWidget(EntrezFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
  }
}

void EntrezFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
}

QString EntrezFetcher::ConfigWidget::preferredName() const {
  return EntrezFetcher::defaultName();
}
