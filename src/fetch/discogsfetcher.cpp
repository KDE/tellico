/***************************************************************************
    Copyright (C) 2008-2009 Robby Stephenson <robby@periapsis.org>
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

#include "discogsfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../images/imagefactory.h"
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

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QDomDocument>
#include <QTextCodec>

//#define DISCOGS_TEST

namespace {
  static const int DISCOGS_MAX_RETURNS_TOTAL = 20;
  static const char* DISCOGS_API_URL = "http://www.discogs.com";
  static const char* DISCOGS_API_KEY = "de6cb96534";
}

using namespace Tellico;
using Tellico::Fetch::DiscogsFetcher;

DiscogsFetcher::DiscogsFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_xsltHandler(0)
    , m_limit(DISCOGS_MAX_RETURNS_TOTAL)
    , m_job(0)
    , m_started(false)
    , m_apiKey(QLatin1String(DISCOGS_API_KEY)) {
}

DiscogsFetcher::~DiscogsFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = 0;
}

QString DiscogsFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool DiscogsFetcher::canFetch(int type) const {
  return type == Data::Collection::Album;
}

void DiscogsFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key", DISCOGS_API_KEY);
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
  m_fetchImages = config_.readEntry("Fetch Images", true);
}

void DiscogsFetcher::search() {
  m_started = true;
  m_start = 1;
  m_total = -1;
  doSearch();
}

void DiscogsFetcher::continueSearch() {
  m_started = true;
  doSearch();
}

void DiscogsFetcher::doSearch() {
  KUrl u(DISCOGS_API_URL);
  u.addQueryItem(QLatin1String("f"), QLatin1String("xml"));
  u.addQueryItem(QLatin1String("api_key"), m_apiKey);

  switch(request().key) {
    case Title:
      u.setPath(QLatin1String("/search"));
      u.addQueryItem(QLatin1String("q"), request().value);
      u.addQueryItem(QLatin1String("type"), QLatin1String("releases"));
      break;

    case Person:
      u.setPath(QString::fromLatin1("/artist/%1").arg(request().value));
      break;

    case Keyword:
      u.setPath(QLatin1String("/search"));
      u.addQueryItem(QLatin1String("q"), request().value);
      u.addQueryItem(QLatin1String("type"), QLatin1String("all"));
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      stop();
      return;
  }

#ifdef DISCOGS_TEST
  u = KUrl("/home/robby/discogs-results.xml");
#endif
//  myDebug() << "url: " << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
}

void DiscogsFetcher::stop() {
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

void DiscogsFetcher::slotComplete(KJob* ) {
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
  myWarning() << "Remove debug from discogsfetcher.cpp";
  QFile f(QLatin1String("/tmp/test1.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec(QTextCodec::codecForName("UTF-8"));
    t << data;
  }
  f.close();
#endif

  if(!m_xsltHandler) {
    initXSLTHandler();
    if(!m_xsltHandler) { // probably an error somewhere in the stylesheet loading
      stop();
      return;
    }
  }

  if(m_total == -1) {
    QDomDocument dom;
    if(!dom.setContent(data, false)) {
      myWarning() << "server did not return valid XML.";
      return;
    }
    // total is /resp/fetchresults/@numResults
    QDomNode n = dom.documentElement().namedItem(QLatin1String("resp"))
                                      .namedItem(QLatin1String("fetchresults"));
    QDomElement e = n.toElement();
    if(!e.isNull()) {
      m_total = e.attribute(QLatin1String("numResults")).toInt();
      myDebug() << "total = " << m_total;
    }
  }

  // assume discogs is always utf-8
  QString str = m_xsltHandler->applyStylesheet(QString::fromUtf8(data, data.size()));
  Import::TellicoImporter imp(str);
  Data::CollPtr coll = imp.collection();
  if(!coll) {
    myDebug() << "no collection pointer";
    stop();
    return;
  }

  int count = 0;
  Data::EntryList entries = coll->entries();
  foreach(Data::EntryPtr entry, entries) {
    if(count >= m_limit) {
      break;
    }
    if(!m_started) {
      // might get aborted
      break;
    }

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, Data::EntryPtr(entry));
    emit signalResultFound(r);
    ++count;
  }
  m_start = m_entries.count() + 1;
  // not sure how to specify start in the REST url
  //  m_hasMoreResults = m_start <= m_total;

  stop(); // required
}

Tellico::Data::EntryPtr DiscogsFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries[uid_];
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }
  // one way we tell if this entry has been fully initialized is to
  // check for a cover image
  if(!entry->field(QLatin1String("cover")).isEmpty()) {
    myLog() << "already downloaded " << entry->title();
    return entry;
  }

  QString release = entry->field(QLatin1String("discogs-id"));
  if(release.isEmpty()) {
    myDebug() << "no discogs release found";
    return entry;
  }

#ifdef DISCOGS_TEST
  KUrl u("/home/robby/discogs-release.xml");
#else
  KUrl u(DISCOGS_API_URL);
  u.setPath(QString::fromLatin1("/release/%1").arg(release));
  u.addQueryItem(QLatin1String("f"), QLatin1String("xml"));
  u.addQueryItem(QLatin1String("api_key"), m_apiKey);
#endif
//  myDebug() << "url: " << u;

  // quiet, utf8, allowCompressed
  QString output = FileHandler::readTextFile(u, true, true, true);

#if 0
  myWarning() << "Remove output debug from discogsfetcher.cpp";
  QFile f(QLatin1String("/tmp/test2.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec(QTextCodec::codecForName("UTF-8"));
    t << output;
  }
  f.close();
#endif

  Import::TellicoImporter imp(m_xsltHandler->applyStylesheet(output));
  Data::CollPtr coll = imp.collection();
//  getTracks(entry);
  if(!coll) {
    myWarning() << "no collection pointer";
    return entry;
  }

  if(coll->entryCount() > 1) {
    myDebug() << "weird, more than one entry found";
  }

  // don't want to include id
  coll->removeField(QLatin1String("discogs-id"));

  entry = coll->entries().front();
  m_entries.insert(uid_, entry); // replaces old value
  return entry;
}

void DiscogsFetcher::initXSLTHandler() {
  QString xsltfile = KStandardDirs::locate("appdata", QLatin1String("discogs2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate discogs2tellico.xsl.";
    return;
  }

  KUrl u;
  u.setPath(xsltfile);

  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    myWarning() << "error in discogs2tellico.xsl.";
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

Tellico::Fetch::FetchRequest DiscogsFetcher::updateRequest(Data::EntryPtr entry_) {
//  myDebug();

  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }

  QString artist = entry_->field(QLatin1String("artist"));
  if(!artist.isEmpty()) {
    return FetchRequest(Person, artist);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* DiscogsFetcher::configWidget(QWidget* parent_) const {
  return new DiscogsFetcher::ConfigWidget(parent_, this);
}

QString DiscogsFetcher::defaultName() {
  return i18n("Discogs Audio Search");
}

QString DiscogsFetcher::defaultIcon() {
  return favIcon("http://www.discogs.com");
}

Tellico::StringHash DiscogsFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("producer")] = i18n("Producer");
  hash[QLatin1String("nationality")] = i18n("Nationality");
  hash[QLatin1String("discogs")] = i18n("Discogs Link");
  return hash;
}

DiscogsFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const DiscogsFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* al = new QLabel(i18n("Registration is required for accessing the %1 data source. "
                               "If you agree to the terms and conditions, <a href='%2'>sign "
                               "up for an account</a>, and enter your information below.")
                                .arg(preferredName(),
                                     QLatin1String("http://www.discogs.com/users/api_key")),
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

  m_fetchImageCheck = new QCheckBox(i18n("Download cover &image"), optionsWidget());
  connect(m_fetchImageCheck, SIGNAL(clicked()), SLOT(slotSetModified()));
  ++row;
  l->addWidget(m_fetchImageCheck, row, 0, 1, 2);
  w = i18n("The cover image may be downloaded as well. However, too many large images in the "
           "collection may degrade performance.");
  m_fetchImageCheck->setWhatsThis(w);

  l->setRowStretch(++row, 10);

  // now add additional fields widget
  addFieldsWidget(DiscogsFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    // only show the key if it is not the default Tellico one...
    // that way the user is prompted to apply for their own
    if(fetcher_->m_apiKey != QLatin1String(DISCOGS_API_KEY)) {
      m_apiKeyEdit->setText(fetcher_->m_apiKey);
    }
    m_fetchImageCheck->setChecked(fetcher_->m_fetchImages);
  } else {
    m_fetchImageCheck->setChecked(true);
  }
}

void DiscogsFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
  config_.writeEntry("Fetch Images", m_fetchImageCheck->isChecked());
}

QString DiscogsFetcher::ConfigWidget::preferredName() const {
  return DiscogsFetcher::defaultName();
}

#include "discogsfetcher.moc"
