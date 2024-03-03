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
#include "../utils/guiproxy.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KJobWidgets/KJobWidgets>

#include <QLabel>
#include <QVBoxLayout>
#include <QFile>
#include <QTextCodec>
#include <QUrlQuery>

namespace {
  static const int GOOGLE_MAX_RETURNS_TOTAL = 20;
  static const char* SCHOLAR_BASE_URL = "https://scholar.google.com/scholar";
  static const char* SCHOLAR_SET_CONFIG_URL = "https://scholar.google.com/scholar_settings?sciifh=1&hl=en&as_sdt=0,47";
  static const char* SCHOLAR_SET_BIBTEX_URL = "https://scholar.google.com/scholar_setprefs?hl=en&num=100&scis=yes&scisf=4&submit=";
}

using namespace Tellico;
using Tellico::Fetch::GoogleScholarFetcher;

GoogleScholarFetcher::GoogleScholarFetcher(QObject* parent_)
    : Fetcher(parent_),
      m_limit(GOOGLE_MAX_RETURNS_TOTAL), m_start(0), m_total(0), m_job(nullptr), m_started(false)
    , m_cookieIsSet(false) {
}

GoogleScholarFetcher::~GoogleScholarFetcher() {
}

QString GoogleScholarFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool GoogleScholarFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == Keyword;
}

bool GoogleScholarFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void GoogleScholarFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void GoogleScholarFetcher::search() {
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
  QUrl u(QString::fromLatin1(SCHOLAR_BASE_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("start"), QString::number(m_start));

  QString value = request().value();
  if(!value.startsWith(QLatin1Char('"'))) {
    value = QLatin1Char('"') + value;
  }
  if(!value.endsWith(QLatin1Char('"'))) {
    value += QLatin1Char('"');
  }
  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("q"), QStringLiteral("allintitle:%1").arg(request().value()));
      break;

    case Keyword:
      q.addQueryItem(QStringLiteral("q"), request().value());
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  u.setQuery(q);
//  myDebug() << "url: " << u.url();

  if(!m_cookieIsSet) {
    setBibtexCookie();
  }

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result,
          this, &GoogleScholarFetcher::slotComplete);
}

void GoogleScholarFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
    m_job = nullptr;
  }
  m_started = false;
  emit signalDone(this);
}

void GoogleScholarFetcher::slotComplete(KJob*) {
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

  const QString text = QString::fromUtf8(data.constData(), data.size());

#if 0
  myWarning() << "Remove debug from googlescholarfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << text;
  }
  f.close();
#endif

  static const QRegularExpression bibtexRx(QStringLiteral("<a\\s.*?href\\s*=\\s*\"([^>]*scholar\\.bib[^>]*?)\""));
  QString bibtex;
  int count = 0;
  for(QRegularExpressionMatchIterator i = bibtexRx.globalMatch(text); count < m_limit && i.hasNext(); ++count) {
    QRegularExpressionMatch match = i.next();
    // for some reason, KIO and google don't return bibtex when '&' is escaped
    QString url = match.captured(1).replace(QLatin1String("&amp;"), QLatin1String("&"));
    QUrl bibtexUrl = QUrl(QString::fromLatin1(SCHOLAR_BASE_URL)).resolved(QUrl(url));
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

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, Data::EntryPtr(entry));
    emit signalResultFound(r);
    ++count;
  }
  m_start = m_entries.count();
//  m_hasMoreResults = m_start <= m_total;
  m_hasMoreResults = false; // for now, no continued searches

  stop(); // required
}

Tellico::Data::EntryPtr GoogleScholarFetcher::fetchEntryHook(uint uid_) {
  return m_entries[uid_];
}

Tellico::Fetch::FetchRequest GoogleScholarFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* GoogleScholarFetcher::configWidget(QWidget* parent_) const {
  return new GoogleScholarFetcher::ConfigWidget(parent_, this);
}

QString GoogleScholarFetcher::defaultName() {
  // no i18n
  return QStringLiteral("Google Scholar");
}

QString GoogleScholarFetcher::defaultIcon() {
  return favIcon("http://scholar.google.com");
}

void GoogleScholarFetcher::setBibtexCookie() {
  // it appears that the series of url reads are necessary to get the correct cookie set
  // have to set preferences to have bibtex output
  const QString text = FileHandler::readTextFile(QUrl(QString::fromLatin1(SCHOLAR_SET_CONFIG_URL)), true);
  // find hidden input variables
  static const QRegularExpression inputRx(QLatin1String("<input\\s+[^>]*?\\s*?type\\s*?=\\s*?\"?hidden\"?\\s+?[^>]+?>"));
  static const QRegularExpression pairRx(QLatin1String("([^=\\s<]+?)\\s*=\\s*\"?([^=\\s\">]+?)\"?"));
  QHash<QString, QString> nameValues;
  auto i = inputRx.globalMatch(text);
  while(i.hasNext()) {
    auto match = i.next();
    const auto input = match.captured(0);
    QString name, value;
    auto i2 = pairRx.globalMatch(input);
    while(i2.hasNext()) {
      const auto match2 = i2.next();
      if(match2.captured(1).toLower() == QLatin1String("name")) {
        name = match2.captured(2);
      } else if(match2.captured(1).toLower() == QLatin1String("value")) {
        value = match2.captured(2);
      }
    }
    if(!name.isEmpty() && !value.isEmpty()) {
      nameValues.insert(name, value);
    }
  }
  QString newUrl = QLatin1String(SCHOLAR_SET_BIBTEX_URL);
  for(QHash<QString, QString>::const_iterator i = nameValues.constBegin(); i != nameValues.constEnd(); ++i) {
    newUrl += QLatin1Char('&') + i.key() + QLatin1Char('=') + i.value();
  }
  FileHandler::readTextFile(QUrl(newUrl), true);
  m_cookieIsSet = true;
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
