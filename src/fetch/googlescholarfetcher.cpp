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

#include "googlescholarfetcher.h"
#include "../core/filehandler.h"
#include "../translators/bibteximporter.h"
#include "../collections/bibtexcollection.h"
#include "../entry.h"
#include "../gui/guiproxy.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <KConfigGroup>
#include <kio/job.h>
#include <kio/jobuidelegate.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QFile>
#include <QTextCodec>

namespace {
  static const int GOOGLE_MAX_RETURNS_TOTAL = 20;
  static const char* SCHOLAR_BASE_URL = "http://scholar.google.com/scholar";
  static const char* SCHOLAR_SET_BIBTEX_URL = "http://scholar.google.com/scholar_setprefs?num=100&scis=yes&scisf=4&submit=Save+Preferences";
}

using Tellico::Fetch::GoogleScholarFetcher;

GoogleScholarFetcher::GoogleScholarFetcher(QObject* parent_)
    : Fetcher(parent_),
      m_limit(GOOGLE_MAX_RETURNS_TOTAL), m_start(0), m_job(0), m_started(false),
      m_cookieIsSet(false) {
  m_bibtexRx = QRegExp(QLatin1String("<a\\s.*href\\s*=\\s*\"([^>]*scholar\\.bib[^>]*)\""));
  m_bibtexRx.setMinimal(true);
}

GoogleScholarFetcher::~GoogleScholarFetcher() {
}

QString GoogleScholarFetcher::defaultName() {
  // no i18n
  return QLatin1String("Google Scholar");
}

QString GoogleScholarFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool GoogleScholarFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void GoogleScholarFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void GoogleScholarFetcher::search() {
  if(!m_cookieIsSet) {
    // have to set preferences to have bibtex output
    FileHandler::readTextFile(KUrl(SCHOLAR_SET_BIBTEX_URL), true);
    m_cookieIsSet = true;
  }
  m_started = true;
  m_start = 0;
  m_total = -1;
  doSearch();
}

void GoogleScholarFetcher::continueSearch() {
  m_started = true;
  doSearch();
}

void GoogleScholarFetcher::doSearch() {
//  myDebug() << "value = " << value_;

  KUrl u(SCHOLAR_BASE_URL);
  u.addQueryItem(QLatin1String("start"), QString::number(m_start));

  switch(request().key) {
    case Title:
      u.addQueryItem(QLatin1String("q"), QString::fromLatin1("allintitle:%1").arg(request().value));
      break;

    case Keyword:
      u.addQueryItem(QLatin1String("q"), request().value);
      break;

    case Person:
      u.addQueryItem(QLatin1String("q"), QString::fromLatin1("author:%1").arg(request().value));
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

void GoogleScholarFetcher::stop() {
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

void GoogleScholarFetcher::slotComplete(KJob*) {
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

  const QString text = QString::fromUtf8(data, data.size());

#if 0
  myWarning() << "Remove debug from googlescholarfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec(QTextCodec::codecForName("UTF-8"));
    t << text;
  }
  f.close();
#endif

  QString bibtex;
  int count = 0;
  for(int pos = m_bibtexRx.indexIn(text); count < m_limit && pos > -1; pos = m_bibtexRx.indexIn(text, pos+m_bibtexRx.matchedLength()), ++count) {
    // for some reason, KIO and google don't return bibtex when '&' is escaped
    QString url = m_bibtexRx.cap(1).replace(QLatin1String("&amp;"), QLatin1String("&"));
    KUrl bibtexUrl(KUrl(SCHOLAR_BASE_URL), url);
//    myDebug() << bibtexUrl;
    bibtex += FileHandler::readTextFile(bibtexUrl, true);
  }

  Import::BibtexImporter imp(bibtex);
  // quiet warnings...
  imp.setCurrentCollection(Data::CollPtr(new Data::BibtexCollection(true)));
  Data::CollPtr coll = imp.collection();
  if(!coll) {
    myDebug() << "no collection pointer";
    stop();
    return;
  }

  count = 0;
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
  m_start = m_entries.count();
//  m_hasMoreResults = m_start <= m_total;
  m_hasMoreResults = false; // for now, no continued searches

  stop(); // required
}

Tellico::Data::EntryPtr GoogleScholarFetcher::fetchEntry(uint uid_) {
  return m_entries[uid_];
}

Tellico::Fetch::FetchRequest GoogleScholarFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* GoogleScholarFetcher::configWidget(QWidget* parent_) const {
  return new GoogleScholarFetcher::ConfigWidget(parent_, this);
}

GoogleScholarFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const GoogleScholarFetcher* /*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

QString GoogleScholarFetcher::ConfigWidget::preferredName() const {
  return GoogleScholarFetcher::defaultName();
}

#include "googlescholarfetcher.moc"
