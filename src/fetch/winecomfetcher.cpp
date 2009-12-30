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

#include "winecomfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../gui/guiproxy.h"
#include "../tellico_utils.h"
#include "../collection.h"
#include "../entry.h"
#include "../images/imagefactory.h"
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
  static const int WINECOM_RETURNS_PER_REQUEST = 25;
  static const int WINECOM_MAX_RETURNS_TOTAL = 100;
  static const char* WINECOM_BASE_URL = "http://services.wine.com";
}

using Tellico::Fetch::WineComFetcher;

WineComFetcher::WineComFetcher(QObject* parent_)
    : Fetcher(parent_), m_xsltHandler(0),
      m_limit(WINECOM_MAX_RETURNS_TOTAL), m_page(1), m_total(-1), m_offset(0),
      m_job(0), m_started(false) {
}

WineComFetcher::~WineComFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = 0;
}

QString WineComFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool WineComFetcher::canFetch(int type) const {
  return type == Data::Collection::Wine;
}

void WineComFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key", QString());
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
}

void WineComFetcher::search() {
  m_started = true;
  m_page = 1;
  m_total = -1;
  m_numResults = 0;
  m_offset = 0;

  doSearch();
}

void WineComFetcher::continueSearch() {
  m_started = true;
  doSearch();
}

void WineComFetcher::doSearch() {
//  myDebug() << "value = " << value_;

  KUrl u(WINECOM_BASE_URL);
  // change beta for a different versionn
  u.setPath(QLatin1String("/api/beta/service.svc/XML/catalog"));
  u.addQueryItem(QLatin1String("apikey"), m_apiKey);
  u.addQueryItem(QLatin1String("offset"), QString::number((m_page-1) * WINECOM_RETURNS_PER_REQUEST));
  u.addQueryItem(QLatin1String("size"), QString::number(WINECOM_RETURNS_PER_REQUEST));

  switch(request().key) {
    case Keyword:
      u.addQueryItem(QLatin1String("search"), request().value);
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

void WineComFetcher::stop() {
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

void WineComFetcher::slotComplete(KJob*) {
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
  myWarning() << "Remove debug from winecomfetcher.cpp";
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
    QDomNode n = dom.documentElement().namedItem(QLatin1String("Catalog"))
                                      .namedItem(QLatin1String("Products"))
                                      .namedItem(QLatin1String("Total"));
    QDomElement e = n.toElement();
    if(!e.isNull()) {
      m_total = e.attribute(QLatin1String("total_results"), QString::number(-1)).toInt();
      myDebug() << m_total;
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
  m_hasMoreResults = m_page * WINECOM_RETURNS_PER_REQUEST < m_total;

  const int currentTotal = qMin(m_total, m_limit);
  if(m_page * WINECOM_RETURNS_PER_REQUEST < currentTotal) {
    int foundCount = (m_page-1) * WINECOM_RETURNS_PER_REQUEST + coll->entryCount();
    message(i18n("Results from %1: %2/%3", source(), foundCount, m_total), MessageHandler::Status);
    ++m_page;
    doSearch();
  } else {
    ++m_page; // need to go to next page
    stop(); // required
  }
}

Tellico::Data::EntryPtr WineComFetcher::fetchEntry(uint uid_) {
  Data::EntryPtr entry = m_entries[uid_];
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  return entry;
}

void WineComFetcher::initXSLTHandler() {
  QString xsltfile = KStandardDirs::locate("appdata", QLatin1String("winecom2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate winecom2tellico.xsl.";
    return;
  }

  KUrl u;
  u.setPath(xsltfile);

  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    myWarning() << "error in winecom2tellico.xsl.";
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

Tellico::Fetch::FetchRequest WineComFetcher::updateRequest(Data::EntryPtr entry_) {
  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  QString t = entry_->field(QLatin1String("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Keyword, t);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* WineComFetcher::configWidget(QWidget* parent_) const {
  return new WineComFetcher::ConfigWidget(parent_, this);
}

QString WineComFetcher::defaultName() {
  return QLatin1String("Wine.com"); // no translation
}

QString WineComFetcher::defaultIcon() {
  return favIcon("http://www.wine.com");
}

WineComFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const WineComFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;
  QLabel* al = new QLabel(i18n("Registration is required for accessing the %1 data source. "
                               "If you agree to the terms and conditions, <a href='%2'>sign "
                               "up for an account</a>, and enter your information below.")
                                .arg(preferredName(),
                                     QLatin1String("http://api.wine.com/plans")),
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
  label->setBuddy(m_apiKeyEdit);

  l->setRowStretch(++row, 10);

  if(fetcher_) {
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
  }
}

void WineComFetcher::ConfigWidget::saveConfig(KConfigGroup& config_) {
  QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }

  saveFieldsConfig(config_);
  slotSetModified(false);
}

QString WineComFetcher::ConfigWidget::preferredName() const {
  return WineComFetcher::defaultName();
}

#include "winecomfetcher.moc"
