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

#include "xmlfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../gui/guiproxy.h"
#include "../utils/xmlhandler.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"

#include <kstandarddirs.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>

#include <QFile>
#include <QTextStream>
#include <QTextCodec>

using Tellico::Fetch::XMLFetcher;

XMLFetcher::XMLFetcher(QObject* parent_) : Fetcher(parent_)
    , m_xsltHandler(0)
    , m_started(false)
    , m_limit(0) {
}

XMLFetcher::~XMLFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = 0;
}

void XMLFetcher::search() {
  m_started = true;
  resetSearch();
  doSearch();
}

void XMLFetcher::continueSearch() {
  m_started = true;
  doSearch();
}

void XMLFetcher::doSearch() {
  const KUrl u = searchUrl();
  Q_ASSERT(!u.isEmpty());
  if(u.isEmpty()) {
    stop();
    return;
  }
//  myDebug() << "url: " << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
}

void XMLFetcher::stop() {
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

void XMLFetcher::slotComplete(KJob* ) {
//  myDebug();
  if(m_job->error()) {
    m_job->ui()->showErrorMessage();
    stop();
    return;
  }

  if(!m_xsltHandler) {
    initXSLTHandler();
    if(!m_xsltHandler) { // probably an error somewhere in the stylesheet loading
      stop();
      return;
    }
  }

  QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }

#if 0
  myWarning() << "Remove debug from xmlfetcher.cpp";
  QFile f(QLatin1String("/tmp/test.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("utf-8");
    t << data;
  }
  f.close();
#endif

  parseData(data);

  const QString str = m_xsltHandler->applyStylesheet(XMLHandler::readXMLData(data));
#if 0
  myWarning() << "Remove debug from xmlfetcher.cpp";
  QFile f2(QLatin1String("/tmp/test-tellico.xml"));
  if(f2.open(QIODevice::WriteOnly)) {
    QTextStream t(&f2);
    t.setCodec("utf-8");
    t << str;
  }
  f2.close();
#endif
  Import::TellicoImporter imp(str);
  // be quiet when loading images
  imp.setOptions(imp.options() ^ Import::ImportShowImageErrors);
  Data::CollPtr coll = imp.collection();
  if(!coll) {
    myDebug() << "no collection pointer";
    stop();
    return;
  }

  if(m_limit < 1) {
    myDebug() << "Limit < 1, changing to 1";
    m_limit = 1;
  }

  int count = 0;
  foreach(Data::EntryPtr entry, coll->entries()) {
    if(count >= m_limit) {
      break;
    }
    if(!m_started) {
      // might get aborted
      break;
    }

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
    ++count;
  }

  checkMoreResults(m_entries.count());
  stop(); // required
}

Tellico::Data::EntryPtr XMLFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries[uid_];
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  // the fetcher might end up creating a new entry
  return fetchEntryHookData(entry);
}

void XMLFetcher::initXSLTHandler() {
  Q_ASSERT(!m_xsltFilename.isEmpty());
  QString xsltfile = KStandardDirs::locate("appdata", m_xsltFilename);
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate" << m_xsltFilename;
    return;
  }

  KUrl u;
  u.setPath(xsltfile);

  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    myWarning() << "error in" << m_xsltFilename;
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

void XMLFetcher::setXSLTFilename(const QString& filename_) {
  if(!filename_.isEmpty() && filename_ != m_xsltFilename) {
    m_xsltFilename = filename_;
    delete m_xsltHandler;
    m_xsltHandler = 0;
  }
}

void XMLFetcher::setLimit(int limit_) {
  Q_ASSERT(limit_ > 0);
  m_limit = limit_;
}

Tellico::XSLTHandler* XMLFetcher::xsltHandler() {
  Q_ASSERT(m_xsltHandler);
  return m_xsltHandler;
}

#include "xmlfetcher.moc"
