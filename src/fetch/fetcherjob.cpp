/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#include "fetcherjob.h"
#include "../tellico_debug.h"

#include <QTimer>

using namespace Tellico;
using namespace Tellico::Fetch;
using Tellico::Fetch::FetcherJob;

FetcherJob::FetcherJob(QObject* parent_, Fetcher::Ptr fetcher_, const FetchRequest& request_)
    : KJob(parent_), m_fetcher(fetcher_), m_request(request_), m_maximumResults(0) {
  connect(m_fetcher.data(), &Fetcher::signalResultFound,
          this, &FetcherJob::slotResult);
  connect(m_fetcher.data(), &Fetcher::signalDone,
          this, &FetcherJob::slotDone);
}

FetcherJob::~FetcherJob() {
  qDeleteAll(m_results);
  m_results.clear();
}

Tellico::Data::EntryList FetcherJob::entries() {
  Data::EntryList list;
  foreach(FetchResult* result, m_results) {
    Data::EntryPtr entry = result->fetchEntry();
    if(entry) {
      list << entry;
    }
  }
  return list;
}

void FetcherJob::setMaximumResults(int count_) {
  Q_ASSERT(count_ >= 0);
  m_maximumResults = count_;
}

void FetcherJob::start() {
  QTimer::singleShot(0, this, &FetcherJob::startSearch);
}

void FetcherJob::startSearch() {
  m_fetcher->startSearch(m_request);
}

void FetcherJob::slotResult(Tellico::Fetch::FetchResult* result_) {
  if(!result_) {
    myDebug() << "null result";
    return;
  }
  m_results.append(result_);
  if(m_maximumResults > 0 && m_results.count() >= m_maximumResults) {
    doKill();
  }
}

void FetcherJob::slotDone() {
  // only continue if more results were specifically asked for
  if(m_fetcher->hasMoreResults() && m_results.count() < m_maximumResults) {
    m_fetcher->continueSearch();
  } else {
    emitResult();
  }
}

bool FetcherJob::doKill() {
  m_fetcher->stop();
  return true;
}
