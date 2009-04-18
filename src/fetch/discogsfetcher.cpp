/***************************************************************************
    copyright            : (C) 2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "discogsfetcher.h"
#include "messagehandler.h"
#include "searchresult.h"
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

//#define DISCOGS_TEST

namespace {
  static const int DISCOGS_MAX_RETURNS_TOTAL = 20;
  static const char* DISCOGS_API_URL = "http://www.discogs.com";
  static const char* DISCOGS_API_KEY = "de6cb96534";
}

using Tellico::Fetch::DiscogsFetcher;

DiscogsFetcher::DiscogsFetcher(QObject* parent_)
    : Fetcher(parent_), m_xsltHandler(0),
      m_limit(DISCOGS_MAX_RETURNS_TOTAL), m_job(0), m_started(false),
      m_apiKey(QLatin1String(DISCOGS_API_KEY)) {
}

DiscogsFetcher::~DiscogsFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = 0;
}

QString DiscogsFetcher::defaultName() {
  return i18n("Discogs Audio Search");
}

QString DiscogsFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool DiscogsFetcher::canFetch(int type) const {
  return type == Data::Collection::Album;
}

void DiscogsFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key");
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
  m_fetchImages = config_.readEntry("Fetch Images", true);
  m_fields = config_.readEntry("Custom Fields", QStringList());
}

void DiscogsFetcher::search(Tellico::Fetch::FetchKey key_, const QString& value_) {
  m_key = key_;
  m_value = value_;
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

  if(!canFetch(collectionType())) {
    message(i18n("%1 does not allow searching for this collection type.", source()), MessageHandler::Warning);
    stop();
    return;
  }

  switch(m_key) {
    case Title:
      u.setPath(QLatin1String("/search"));
      u.addQueryItem(QLatin1String("q"), m_value);
      u.addQueryItem(QLatin1String("type"), QLatin1String("release"));
      break;

    case Person:
      u.setPath(QString::fromLatin1("/artist/%1").arg(m_value));
      break;

    case Keyword:
      u.setPath(QLatin1String("/search"));
      u.addQueryItem(QLatin1String("q"), m_value);
      u.addQueryItem(QLatin1String("type"), QLatin1String("all"));
      break;

    default:
      kWarning() << "DiscogsFetcher::search() - key not recognized: " << m_key;
      stop();
      return;
  }

#ifdef DISCOGS_TEST
  u = KUrl("/home/robby/discogs-results.xml");
#endif
//  myDebug() << "DiscogsFetcher::search() - url: " << u.url() << endl;

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
//  myDebug() << "DiscogsFetcher::slotComplete()" << endl;
  if(m_job->error()) {
    m_job->ui()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "DiscogsFetcher::slotComplete() - no data" << endl;
    stop();
    return;
  }

#if 0
  kWarning() << "Remove debug from discogsfetcher.cpp";
  QFile f(QLatin1String("/tmp/test.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setEncoding(QTextStream::UnicodeUTF8);
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
      kWarning() << "DiscogsFetcher::slotComplete() - server did not return valid XML.";
      return;
    }
    // total is /resp/searchresults/@numResults
    QDomNode n = dom.documentElement().namedItem(QLatin1String("resp"))
                                      .namedItem(QLatin1String("searchresults"));
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
    myDebug() << "DiscogsFetcher::slotComplete() - no collection pointer" << endl;
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

    SearchResult* r = new SearchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, Data::EntryPtr(entry));
    emit signalResultFound(r);
    ++count;
  }
  m_start = m_entries.count() + 1;
  // not sure how tospecify start in the REST url
  //  m_hasMoreResults = m_start <= m_total;

  stop(); // required
}

Tellico::Data::EntryPtr DiscogsFetcher::fetchEntry(uint uid_) {
  Data::EntryPtr entry = m_entries[uid_];
  if(!entry) {
    kWarning() << "DiscogsFetcher::fetchEntry() - no entry in dict";
    return Data::EntryPtr();
  }
  // one way we tell if this entry has been fully initialized is to
  // check for a cover image
  if(!entry->field(QLatin1String("cover")).isEmpty()) {
    myLog() << "DiscogsFetcher::fetchEntry() - already downloaded " << entry->title() << endl;
    return entry;
  }

  QString release = entry->field(QLatin1String("discogs-id"));
  if(release.isEmpty()) {
    myDebug() << "DiscogsFetcher::fetchEntry() - no discogs release found" << endl;
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
//  myDebug() << "DiscogsFetcher::fetchEntry() - url: " << u << endl;

  // quiet, utf8, allowCompressed
  QString output = FileHandler::readTextFile(u, true, true, true);
#if 0
  kWarning() << "Remove output debug from discogsfetcher.cpp";
  QFile f(QLatin1String("/tmp/test.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << output;
  }
  f.close();
#endif

  Import::TellicoImporter imp(m_xsltHandler->applyStylesheet(output));
  Data::CollPtr coll = imp.collection();
//  getTracks(entry);
  if(!coll) {
    kWarning() << "DiscogsFetcher::fetchEntry() - no collection pointer";
    return entry;
  }

  if(coll->entryCount() > 1) {
    myDebug() << "DiscogsFetcher::fetchEntry() - weird, more than one entry found" << endl;
  }

  const StringMap customFields = this->customFields();
  for(StringMap::ConstIterator it = customFields.begin(); it != customFields.end(); ++it) {
    if(!m_fields.contains(it.key())) {
      coll->removeField(it.key());
    }
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
    kWarning() << "DiscogsFetcher::initXSLTHandler() - can not locate discogs2tellico.xsl.";
    return;
  }

  KUrl u;
  u.setPath(xsltfile);

  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    kWarning() << "DiscogsFetcher::initXSLTHandler() - error in discogs2tellico.xsl.";
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

void DiscogsFetcher::updateEntry(Tellico::Data::EntryPtr entry_) {
//  myDebug() << "DiscogsFetcher::updateEntry()" << endl;

  QString value;
  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    search(Title, value);
    return;
  }

  QString artist = entry_->field(QLatin1String("artist"));
  if(!artist.isEmpty()) {
    search(Person, artist);
    return;
  }

  myDebug() << "DiscogsFetcher::updateEntry() - insufficient info to search" << endl;
  emit signalDone(this); // always need to emit this if not continuing with the search
}

Tellico::Fetch::ConfigWidget* DiscogsFetcher::configWidget(QWidget* parent_) const {
  return new DiscogsFetcher::ConfigWidget(parent_, this);
}

DiscogsFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const DiscogsFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;
  QLabel* label = new QLabel(i18n("API &key: "), optionsWidget());
  l->addWidget(label, ++row, 0);

  m_apiKeyEdit = new KLineEdit(optionsWidget());
  connect(m_apiKeyEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_apiKeyEdit, row, 1);
  QString w = i18n("With your discogs.com account you receive an API key for the usage of their XML-based interface "
                   "(See http://www.discogs.com/help/api).");
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
  addFieldsWidget(DiscogsFetcher::customFields(), fetcher_ ? fetcher_->m_fields : QStringList());

  if(fetcher_) {
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
    m_fetchImageCheck->setChecked(fetcher_->m_fetchImages);
  } else {
    m_apiKeyEdit->setText(QLatin1String(DISCOGS_API_KEY));
    m_fetchImageCheck->setChecked(true);
  }
}

void DiscogsFetcher::ConfigWidget::saveConfig(KConfigGroup& config_) {
  QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
  config_.writeEntry("Fetch Images", m_fetchImageCheck->isChecked());

  saveFieldsConfig(config_);
  slotSetModified(false);
}

QString DiscogsFetcher::ConfigWidget::preferredName() const {
  return DiscogsFetcher::defaultName();
}

Tellico::StringMap DiscogsFetcher::customFields() {
  StringMap map;
  map[QLatin1String("producer")] = i18n("Producer");
  map[QLatin1String("nationality")] = i18n("Nationality");
  map[QLatin1String("discogs")] = i18n("Discogs Link");
  return map;
}

#include "discogsfetcher.moc"
