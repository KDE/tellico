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
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../imagefactory.h"
#include "../tellico_kernel.h"
#include "../tellico_utils.h"
#include "../collection.h"
#include "../entry.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kio/job.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qfile.h>
#include <qwhatsthis.h>

//#define DISCOGS_TEST

namespace {
  static const int DISCOGS_MAX_RETURNS_TOTAL = 20;
  static const char* DISCOGS_API_URL = "http://www.discogs.com";
  static const char* DISCOGS_API_KEY = "de6cb96534";
}

using Tellico::Fetch::DiscogsFetcher;

DiscogsFetcher::DiscogsFetcher(QObject* parent_, const char* name_)
    : Fetcher(parent_, name_), m_xsltHandler(0),
      m_limit(DISCOGS_MAX_RETURNS_TOTAL), m_job(0), m_started(false),
      m_apiKey(QString::fromLatin1(DISCOGS_API_KEY)) {
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
  m_fetchImages = config_.readBoolEntry("Fetch Images", true);
  m_fields = config_.readListEntry("Custom Fields");
}

void DiscogsFetcher::search(FetchKey key_, const QString& value_) {
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
  KURL u(QString::fromLatin1(DISCOGS_API_URL));
  u.addQueryItem(QString::fromLatin1("f"), QString::fromLatin1("xml"));
  u.addQueryItem(QString::fromLatin1("api_key"), m_apiKey);

  if(!canFetch(Kernel::self()->collectionType())) {
    message(i18n("%1 does not allow searching for this collection type.").arg(source()), MessageHandler::Warning);
    stop();
    return;
  }

  switch(m_key) {
    case Title:
      u.setPath(QString::fromLatin1("/search"));
      u.addQueryItem(QString::fromLatin1("q"), m_value);
      u.addQueryItem(QString::fromLatin1("type"), QString::fromLatin1("release"));
      break;

    case Person:
      u.setPath(QString::fromLatin1("/artist/%1").arg(m_value));
      break;

    case Keyword:
      u.setPath(QString::fromLatin1("/search"));
      u.addQueryItem(QString::fromLatin1("q"), m_value);
      u.addQueryItem(QString::fromLatin1("type"), QString::fromLatin1("all"));
      break;

    default:
      kdWarning() << "DiscogsFetcher::search() - key not recognized: " << m_key << endl;
      stop();
      return;
  }

#ifdef DISCOGS_TEST
  u = KURL(QString::fromLatin1("/home/robby/discogs-results.xml"));
#endif
//  myDebug() << "DiscogsFetcher::search() - url: " << u.url() << endl;

  m_job = KIO::get(u, false, false);
  connect(m_job, SIGNAL(data(KIO::Job*, const QByteArray&)),
          SLOT(slotData(KIO::Job*, const QByteArray&)));
  connect(m_job, SIGNAL(result(KIO::Job*)),
          SLOT(slotComplete(KIO::Job*)));
}

void DiscogsFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }
  m_data.truncate(0);
  m_started = false;
  emit signalDone(this);
}

void DiscogsFetcher::slotData(KIO::Job*, const QByteArray& data_) {
  QDataStream stream(m_data, IO_WriteOnly | IO_Append);
  stream.writeRawBytes(data_.data(), data_.size());
}

void DiscogsFetcher::slotComplete(KIO::Job* job_) {
//  myDebug() << "DiscogsFetcher::slotComplete()" << endl;
  if(job_->error()) {
    job_->showErrorDialog(Kernel::self()->widget());
    stop();
    return;
  }

  if(m_data.isEmpty()) {
    myDebug() << "DiscogsFetcher::slotComplete() - no data" << endl;
    stop();
    return;
  }

#if 0
  kdWarning() << "Remove debug from discogsfetcher.cpp" << endl;
  QFile f(QString::fromLatin1("/tmp/test.xml"));
  if(f.open(IO_WriteOnly)) {
    QTextStream t(&f);
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << QCString(m_data, m_data.size()+1);
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
    if(!dom.setContent(m_data, false)) {
      kdWarning() << "DiscogsFetcher::slotComplete() - server did not return valid XML." << endl;
      return;
    }
    // total is /resp/searchresults/@numResults
    QDomNode n = dom.documentElement().namedItem(QString::fromLatin1("resp"))
                                      .namedItem(QString::fromLatin1("searchresults"));
    QDomElement e = n.toElement();
    if(!e.isNull()) {
      m_total = e.attribute(QString::fromLatin1("numResults")).toInt();
      myDebug() << "total = " << m_total;
    }
  }

  // assume discogs is always utf-8
  QString str = m_xsltHandler->applyStylesheet(QString::fromUtf8(m_data, m_data.size()));
  Import::TellicoImporter imp(str);
  Data::CollPtr coll = imp.collection();
  if(!coll) {
    myDebug() << "DiscogsFetcher::slotComplete() - no collection pointer" << endl;
    stop();
    return;
  }

  int count = 0;
  Data::EntryVec entries = coll->entries();
  for(Data::EntryVec::Iterator entry = entries.begin(); count < m_limit && entry != entries.end(); ++entry, ++count) {
    if(!m_started) {
      // might get aborted
      break;
    }
    QString desc = entry->field(QString::fromLatin1("artist"))
                 + QChar('/')
                 + entry->field(QString::fromLatin1("label"));

    SearchResult* r = new SearchResult(this, entry->title(), desc, QString());
    m_entries.insert(r->uid, Data::EntryPtr(entry));
    emit signalResultFound(r);
  }
  m_start = m_entries.count() + 1;
  // not sure how tospecify start in the REST url
  //  m_hasMoreResults = m_start <= m_total;

  stop(); // required
}

Tellico::Data::EntryPtr DiscogsFetcher::fetchEntry(uint uid_) {
  Data::EntryPtr entry = m_entries[uid_];
  if(!entry) {
    kdWarning() << "DiscogsFetcher::fetchEntry() - no entry in dict" << endl;
    return 0;
  }
  // one way we tell if this entry has been fully initialized is to
  // check for a cover image
  if(!entry->field(QString::fromLatin1("cover")).isEmpty()) {
    myLog() << "DiscogsFetcher::fetchEntry() - already downloaded " << entry->title() << endl;
    return entry;
  }

  QString release = entry->field(QString::fromLatin1("discogs-id"));
  if(release.isEmpty()) {
    myDebug() << "DiscogsFetcher::fetchEntry() - no discogs release found" << endl;
    return entry;
  }

#ifdef DISCOGS_TEST
  KURL u(QString::fromLatin1("/home/robby/discogs-release.xml"));
#else
  KURL u(QString::fromLatin1(DISCOGS_API_URL));
  u.setPath(QString::fromLatin1("/release/%1").arg(release));
  u.addQueryItem(QString::fromLatin1("f"), QString::fromLatin1("xml"));
  u.addQueryItem(QString::fromLatin1("api_key"), m_apiKey);
#endif
//  myDebug() << "DiscogsFetcher::fetchEntry() - url: " << u << endl;

  // quiet, utf8, allowCompressed
  QString output = FileHandler::readTextFile(u, true, true, true);
#if 0
  kdWarning() << "Remove output debug from discogsfetcher.cpp" << endl;
  QFile f(QString::fromLatin1("/tmp/test.xml"));
  if(f.open(IO_WriteOnly)) {
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
    kdWarning() << "DiscogsFetcher::fetchEntry() - no collection pointer" << endl;
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
  coll->removeField(QString::fromLatin1("discogs-id"));

  entry = coll->entries().front();
  m_entries.replace(uid_, entry);
  return entry;
}

void DiscogsFetcher::initXSLTHandler() {
  QString xsltfile = locate("appdata", QString::fromLatin1("discogs2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    kdWarning() << "DiscogsFetcher::initXSLTHandler() - can not locate discogs2tellico.xsl." << endl;
    return;
  }

  KURL u;
  u.setPath(xsltfile);

  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    kdWarning() << "DiscogsFetcher::initXSLTHandler() - error in discogs2tellico.xsl." << endl;
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

void DiscogsFetcher::updateEntry(Data::EntryPtr entry_) {
//  myDebug() << "DiscogsFetcher::updateEntry()" << endl;

  QString value;
  QString title = entry_->field(QString::fromLatin1("title"));
  if(!title.isEmpty()) {
    search(Title, value);
    return;
  }

  QString artist = entry_->field(QString::fromLatin1("artist"));
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
  QGridLayout* l = new QGridLayout(optionsWidget(), 2, 2);
  l->setSpacing(4);
  l->setColStretch(1, 10);

  int row = -1;
  QLabel* label = new QLabel(i18n("API &key: "), optionsWidget());
  l->addWidget(label, ++row, 0);

  m_apiKeyEdit = new KLineEdit(optionsWidget());
  connect(m_apiKeyEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_apiKeyEdit, row, 1);
  QString w = i18n("With your discogs.com account you receive an API key for the usage of their XML-based interface "
                   "(See http://www.discogs.com/help/api).");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_apiKeyEdit, w);
  label->setBuddy(m_apiKeyEdit);

  m_fetchImageCheck = new QCheckBox(i18n("Download cover &image"), optionsWidget());
  connect(m_fetchImageCheck, SIGNAL(clicked()), SLOT(slotSetModified()));
  ++row;
  l->addMultiCellWidget(m_fetchImageCheck, row, row, 0, 1);
  w = i18n("The cover image may be downloaded as well. However, too many large images in the "
           "collection may degrade performance.");
  QWhatsThis::add(m_fetchImageCheck, w);

  l->setRowStretch(++row, 10);

  // now add additional fields widget
  addFieldsWidget(DiscogsFetcher::customFields(), fetcher_ ? fetcher_->m_fields : QStringList());

  if(fetcher_) {
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
    m_fetchImageCheck->setChecked(fetcher_->m_fetchImages);
  } else {
    m_apiKeyEdit->setText(QString::fromLatin1(DISCOGS_API_KEY));
    m_fetchImageCheck->setChecked(true);
  }
}

void DiscogsFetcher::ConfigWidget::saveConfig(KConfigGroup& config_) {
  QString apiKey = m_apiKeyEdit->text().stripWhiteSpace();
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
  map[QString::fromLatin1("producer")] = i18n("Producer");
  map[QString::fromLatin1("nationality")] = i18n("Nationality");
  map[QString::fromLatin1("discogs")] = i18n("Discogs Link");
  return map;
}

#include "discogsfetcher.moc"
