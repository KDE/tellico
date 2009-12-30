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

#include "yahoofetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../images/imagefactory.h"
#include "../gui/guiproxy.h"
#include "../tellico_utils.h"
#include "../collection.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kstandarddirs.h>
#include <KConfigGroup>
#include <kio/job.h>
#include <kio/jobuidelegate.h>

#include <QDomDocument>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>

namespace {
  static const int YAHOO_MAX_RETURNS_TOTAL = 20;
  static const char* YAHOO_BASE_URL = "http://search.yahooapis.com/AudioSearchService/V1/albumSearch";
  static const char* YAHOO_APP_ID = "tellico-robby";
}

using Tellico::Fetch::YahooFetcher;

YahooFetcher::YahooFetcher(QObject* parent_)
    : Fetcher(parent_), m_xsltHandler(0),
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

void YahooFetcher::search() {
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
//  myDebug() << "value = " << value_;

  KUrl u(YAHOO_BASE_URL);
  u.addQueryItem(QLatin1String("appid"),   QLatin1String(YAHOO_APP_ID));
  u.addQueryItem(QLatin1String("type"),    QLatin1String("all"));
  u.addQueryItem(QLatin1String("output"),  QLatin1String("xml"));
  u.addQueryItem(QLatin1String("start"),   QString::number(m_start));
  u.addQueryItem(QLatin1String("results"), QString::number(YAHOO_MAX_RETURNS_TOTAL));

  switch(request().key) {
    case Title:
      u.addQueryItem(QLatin1String("album"), request().value);
      break;

    case Person:
      u.addQueryItem(QLatin1String("artist"), request().value);
      break;

    // raw is used for the entry updates
    case Raw:
//      u.removeQueryItem(QLatin1String("type"));
//      u.addQueryItem(QLatin1String("type"), QLatin1String("phrase"));
      u.setQuery(u.query() + QLatin1Char('&') + request().value);
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

void YahooFetcher::stop() {
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

void YahooFetcher::slotComplete(KJob*) {
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
  myWarning() << "Remove debug from yahoofetcher.cpp";
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
      myWarning() << "server did not return valid XML.";
      return;
    }
    // total is top level element, with attribute totalResultsAvailable
    QDomElement e = dom.documentElement();
    if(!e.isNull()) {
      m_total = e.attribute(QLatin1String("totalResultsAvailable")).toInt();
    }
  }

  // assume yahoo is always utf-8
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
  m_hasMoreResults = m_start <= m_total;
  stop(); // required
}

Tellico::Data::EntryPtr YahooFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries[uid_];
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  KUrl imageURL = entry->field(QLatin1String("image"));
  if(!imageURL.isEmpty()) {
    QString id = ImageFactory::addImage(imageURL, true);
    if(id.isEmpty()) {
    // rich text causes layout issues
//      emit signalStatus(i18n("<qt>The cover image for <i>%1</i> could not be loaded.</qt>",
//                              entry->field(QLatin1String("title"))));
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    } else {
      entry->setField(QLatin1String("cover"), id);
    }
  }

  getTracks(entry);

  // don't want to show image urls in the fetch dialog
  entry->setField(QLatin1String("image"),  QString());
  // no need for album id now ?
  entry->setField(QLatin1String("yahoo"),  QString());
  return entry;
}

void YahooFetcher::initXSLTHandler() {
  QString xsltfile = KStandardDirs::locate("appdata", QLatin1String("yahoo2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate yahoo2tellico.xsl.";
    return;
  }

  KUrl u;
  u.setPath(xsltfile);

  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    myWarning() << "error in yahoo2tellico.xsl.";
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

void YahooFetcher::getTracks(Tellico::Data::EntryPtr entry_) {
  // get album id
  if(!entry_ || entry_->field(QLatin1String("yahoo")).isEmpty()) {
    return;
  }

  const QString albumid = entry_->field(QLatin1String("yahoo"));

  KUrl u(YAHOO_BASE_URL);
  u.setFileName(QLatin1String("songSearch"));
  u.addQueryItem(QLatin1String("appid"),   QLatin1String(YAHOO_APP_ID));
  u.addQueryItem(QLatin1String("type"),    QLatin1String("all"));
  u.addQueryItem(QLatin1String("output"),  QLatin1String("xml"));
  // go ahesad and ask for all results, since there might well be more than 10 songs on the CD
  u.addQueryItem(QLatin1String("results"), QString::number(50));
  u.addQueryItem(QLatin1String("albumid"), albumid);

//  myDebug() << "url: " << u.url();
  QDomDocument dom = FileHandler::readXMLFile(u, false /*no namespace*/, true /*quiet*/);
  if(dom.isNull()) {
    myDebug() << "null dom returned";
    return;
  }

#if 0
  myWarning() << "Remove debug from yahoofetcher.cpp";
  QFile f(QLatin1String("/tmp/test.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << dom.toString();
  }
  f.close();
#endif

  const QString track = QLatin1String("track");

  QDomNodeList nodes = dom.documentElement().childNodes();
  for(int i = 0; i < nodes.count(); ++i) {
    QDomElement e = nodes.item(i).toElement();
    if(e.isNull()) {
      continue;
    }
    QString t = e.namedItem(QLatin1String("Title")).toElement().text();
    QString n = e.namedItem(QLatin1String("Track")).toElement().text();
    bool ok;
    int trackNum = Tellico::toUInt(n, &ok);
    // trackNum might be 0
    if(t.isEmpty() || !ok || trackNum < 1) {
      continue;
    }
    QString a = e.namedItem(QLatin1String("Artist")).toElement().text();
    QString l = e.namedItem(QLatin1String("Length")).toElement().text();

    int len = Tellico::toUInt(l, &ok);
    QString value = t + FieldFormat::columnDelimiterString() + a;
    if(ok && len > 0) {
      value += FieldFormat::columnDelimiterString() + Tellico::minutes(len);
    }
    entry_->setField(track, insertValue(entry_->field(track), value, trackNum));
  }
}

// not zero-based
QString YahooFetcher::insertValue(const QString& str_, const QString& value_, int pos_) {
  QStringList list = FieldFormat::splitTable(str_);
  for(int i = list.count(); i < pos_; ++i) {
    list += QString();
  }
  bool write = true;
  if(!list[pos_-1].isNull()) {
    // for some reason, some songs are repeated from yahoo, with 0 length, don't overwrite that
    if(value_.count(FieldFormat::columnDelimiterString()) < 2) { // means no length value
      write = false;
    }
  }
  if(!value_.isEmpty() && write) {
    list[pos_-1] = value_;
  }
  return list.join(FieldFormat::rowDelimiterString());
}

Tellico::Fetch::FetchRequest YahooFetcher::updateRequest(Data::EntryPtr entry_) {
  QString value;
  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    value += QLatin1String("album=") + title;
  }
  QString artist = entry_->field(QLatin1String("artist"));
  if(!artist.isEmpty()) {
    if(!value.isEmpty()) {
      value += QLatin1Char('&');
    }
    value += QLatin1String("artist=") + artist;
  }
  if(!value.isEmpty()) {
    return FetchRequest(Fetch::Raw, value);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* YahooFetcher::configWidget(QWidget* parent_) const {
  return new YahooFetcher::ConfigWidget(parent_, this);
}

YahooFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const YahooFetcher* /*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

QString YahooFetcher::ConfigWidget::preferredName() const {
  return YahooFetcher::defaultName();
}

#include "yahoofetcher.moc"
