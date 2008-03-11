/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "yahoofetcher.h"
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

#include <qdom.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qfile.h>

namespace {
  static const int YAHOO_MAX_RETURNS_TOTAL = 20;
  static const char* YAHOO_BASE_URL = "http://search.yahooapis.com/AudioSearchService/V1/albumSearch";
  static const char* YAHOO_APP_ID = "tellico-robby";
}

using Tellico::Fetch::YahooFetcher;

YahooFetcher::YahooFetcher(QObject* parent_, const char* name_)
    : Fetcher(parent_, name_), m_xsltHandler(0),
      m_limit(YAHOO_MAX_RETURNS_TOTAL), m_job(0), m_started(false) {
}

YahooFetcher::~YahooFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = 0;
}

QString YahooFetcher::defaultName() {
  return i18n("Yahoo! Audio Search");
}

QString YahooFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool YahooFetcher::canFetch(int type) const {
  return type == Data::Collection::Album;
}

void YahooFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void YahooFetcher::search(FetchKey key_, const QString& value_) {
  m_key = key_;
  m_value = value_;
  m_started = true;
  m_start = 1;
  m_total = -1;
  doSearch();
}

void YahooFetcher::continueSearch() {
  m_started = true;
  doSearch();
}

void YahooFetcher::doSearch() {
//  myDebug() << "YahooFetcher::search() - value = " << value_ << endl;

  KURL u(QString::fromLatin1(YAHOO_BASE_URL));
  u.addQueryItem(QString::fromLatin1("appid"),   QString::fromLatin1(YAHOO_APP_ID));
  u.addQueryItem(QString::fromLatin1("type"),    QString::fromLatin1("all"));
  u.addQueryItem(QString::fromLatin1("output"),  QString::fromLatin1("xml"));
  u.addQueryItem(QString::fromLatin1("start"),   QString::number(m_start));
  u.addQueryItem(QString::fromLatin1("results"), QString::number(YAHOO_MAX_RETURNS_TOTAL));

  if(!canFetch(Kernel::self()->collectionType())) {
    message(i18n("%1 does not allow searching for this collection type.").arg(source()), MessageHandler::Warning);
    stop();
    return;
  }

  switch(m_key) {
    case Title:
      u.addQueryItem(QString::fromLatin1("album"), m_value);
      break;

    case Person:
      u.addQueryItem(QString::fromLatin1("artist"), m_value);
      break;

    // raw is used for the entry updates
    case Raw:
//      u.removeQueryItem(QString::fromLatin1("type"));
//      u.addQueryItem(QString::fromLatin1("type"), QString::fromLatin1("phrase"));
      u.setQuery(u.query() + '&' + m_value);
      break;

    default:
      kdWarning() << "YahooFetcher::search() - key not recognized: " << m_key << endl;
      stop();
      return;
  }
//  myDebug() << "YahooFetcher::search() - url: " << u.url() << endl;

  m_job = KIO::get(u, false, false);
  connect(m_job, SIGNAL(data(KIO::Job*, const QByteArray&)),
          SLOT(slotData(KIO::Job*, const QByteArray&)));
  connect(m_job, SIGNAL(result(KIO::Job*)),
          SLOT(slotComplete(KIO::Job*)));
}

void YahooFetcher::stop() {
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

void YahooFetcher::slotData(KIO::Job*, const QByteArray& data_) {
  QDataStream stream(m_data, IO_WriteOnly | IO_Append);
  stream.writeRawBytes(data_.data(), data_.size());
}

void YahooFetcher::slotComplete(KIO::Job* job_) {
//  myDebug() << "YahooFetcher::slotComplete()" << endl;
  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  if(job_->error()) {
    job_->showErrorDialog(Kernel::self()->widget());
    stop();
    return;
  }

  if(m_data.isEmpty()) {
    myDebug() << "YahooFetcher::slotComplete() - no data" << endl;
    stop();
    return;
  }

#if 0
  kdWarning() << "Remove debug from yahoofetcher.cpp" << endl;
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
      kdWarning() << "YahooFetcher::slotComplete() - server did not return valid XML." << endl;
      return;
    }
    // total is top level element, with attribute totalResultsAvailable
    QDomElement e = dom.documentElement();
    if(!e.isNull()) {
      m_total = e.attribute(QString::fromLatin1("totalResultsAvailable")).toInt();
    }
  }

  // assume yahoo is always utf-8
  QString str = m_xsltHandler->applyStylesheet(QString::fromUtf8(m_data, m_data.size()));
  Import::TellicoImporter imp(str);
  Data::CollPtr coll = imp.collection();
  if(!coll) {
    myDebug() << "YahooFetcher::slotComplete() - no collection pointer" << endl;
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
                 + entry->field(QString::fromLatin1("label"))
                 + QChar('/')
                 + entry->field(QString::fromLatin1("year"));

    SearchResult* r = new SearchResult(this, entry->title(), desc, entry->field(QString::fromLatin1("isbn")));
    m_entries.insert(r->uid, Data::EntryPtr(entry));
    emit signalResultFound(r);
  }
  m_start = m_entries.count() + 1;
  m_hasMoreResults = m_start <= m_total;
  stop(); // required
}

Tellico::Data::EntryPtr YahooFetcher::fetchEntry(uint uid_) {
  Data::EntryPtr entry = m_entries[uid_];
  if(!entry) {
    kdWarning() << "YahooFetcher::fetchEntry() - no entry in dict" << endl;
    return 0;
  }

  KURL imageURL = entry->field(QString::fromLatin1("image"));
  if(!imageURL.isEmpty()) {
    QString id = ImageFactory::addImage(imageURL, true);
    if(id.isEmpty()) {
    // rich text causes layout issues
//      emit signalStatus(i18n("<qt>The cover image for <i>%1</i> could not be loaded.</qt>").arg(
//                              entry->field(QString::fromLatin1("title"))));
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    } else {
      entry->setField(QString::fromLatin1("cover"), id);
    }
  }

  getTracks(entry);

  // don't want to show image urls in the fetch dialog
  entry->setField(QString::fromLatin1("image"),  QString::null);
  // no need for album id now ?
  entry->setField(QString::fromLatin1("yahoo"),  QString::null);
  return entry;
}

void YahooFetcher::initXSLTHandler() {
  QString xsltfile = locate("appdata", QString::fromLatin1("yahoo2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    kdWarning() << "YahooFetcher::initXSLTHandler() - can not locate yahoo2tellico.xsl." << endl;
    return;
  }

  KURL u;
  u.setPath(xsltfile);

  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    kdWarning() << "YahooFetcher::initXSLTHandler() - error in yahoo2tellico.xsl." << endl;
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

void YahooFetcher::getTracks(Data::EntryPtr entry_) {
  // get album id
  if(!entry_ || entry_->field(QString::fromLatin1("yahoo")).isEmpty()) {
    return;
  }

  const QString albumid = entry_->field(QString::fromLatin1("yahoo"));

  KURL u(QString::fromLatin1(YAHOO_BASE_URL));
  u.setFileName(QString::fromLatin1("songSearch"));
  u.addQueryItem(QString::fromLatin1("appid"),   QString::fromLatin1(YAHOO_APP_ID));
  u.addQueryItem(QString::fromLatin1("type"),    QString::fromLatin1("all"));
  u.addQueryItem(QString::fromLatin1("output"),  QString::fromLatin1("xml"));
  // go ahesad and ask for all results, since there might well be more than 10 songs on the CD
  u.addQueryItem(QString::fromLatin1("results"), QString::number(50));
  u.addQueryItem(QString::fromLatin1("albumid"), albumid);

//  myDebug() << "YahooFetcher::getTracks() - url: " << u.url() << endl;
  QDomDocument dom = FileHandler::readXMLFile(u, false /*no namespace*/, true /*quiet*/);
  if(dom.isNull()) {
    myDebug() << "YahooFetcher::getTracks() - null dom returned" << endl;
    return;
  }

#if 0
  kdWarning() << "Remove debug from yahoofetcher.cpp" << endl;
  QFile f(QString::fromLatin1("/tmp/test.xml"));
  if(f.open(IO_WriteOnly)) {
    QTextStream t(&f);
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << dom.toString();
  }
  f.close();
#endif

  const QString track = QString::fromLatin1("track");

  QDomNodeList nodes = dom.documentElement().childNodes();
  for(uint i = 0; i < nodes.count(); ++i) {
    QDomElement e = nodes.item(i).toElement();
    if(e.isNull()) {
      continue;
    }
    QString t = e.namedItem(QString::fromLatin1("Title")).toElement().text();
    QString n = e.namedItem(QString::fromLatin1("Track")).toElement().text();
    bool ok;
    int trackNum = Tellico::toUInt(n, &ok);
    // trackNum might be 0
    if(t.isEmpty() || !ok || trackNum < 1) {
      continue;
    }
    QString a = e.namedItem(QString::fromLatin1("Artist")).toElement().text();
    QString l = e.namedItem(QString::fromLatin1("Length")).toElement().text();

    int len = Tellico::toUInt(l, &ok);
    QString value = t + "::" + a;
    if(ok && len > 0) {
      value += + "::" + Tellico::minutes(len);
    }
    entry_->setField(track, insertValue(entry_->field(track), value, trackNum));
  }
}

// not zero-based
QString YahooFetcher::insertValue(const QString& str_, const QString& value_, uint pos_) {
  QStringList list = Data::Field::split(str_, true);
  for(uint i = list.count(); i < pos_; ++i) {
    list += QString::null;
  }
  bool write = true;
  if(!list[pos_-1].isNull()) {
    // for some reason, some songs are repeated from yahoo, with 0 length, don't overwrite that
    if(value_.contains(QString::fromLatin1("::")) < 2) { // means no length value
      write = false;
    }
  }
  if(!value_.isEmpty() && write) {
    list[pos_-1] = value_;
  }
  return list.join(QString::fromLatin1("; "));
}

void YahooFetcher::updateEntry(Data::EntryPtr entry_) {
//  myDebug() << "YahooFetcher::updateEntry()" << endl;
  // limit to top 5 results
  m_limit = 5;

  QString value;
  QString title = entry_->field(QString::fromLatin1("title"));
  if(!title.isEmpty()) {
    value += QString::fromLatin1("album=") + title;
  }
  QString artist = entry_->field(QString::fromLatin1("artist"));
  if(!artist.isEmpty()) {
    if(!value.isEmpty()) {
      value += '&';
    }
    value += QString::fromLatin1("artist=") + artist;
  }
  if(!value.isEmpty()) {
    search(Fetch::Raw, value);
    return;
  }

  myDebug() << "YahooFetcher::updateEntry() - insufficient info to search" << endl;
  emit signalDone(this); // always need to emit this if not continuing with the search
}

Tellico::Fetch::ConfigWidget* YahooFetcher::configWidget(QWidget* parent_) const {
  return new YahooFetcher::ConfigWidget(parent_, this);
}

YahooFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const YahooFetcher*/*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

QString YahooFetcher::ConfigWidget::preferredName() const {
  return YahooFetcher::defaultName();
}

#include "yahoofetcher.moc"
