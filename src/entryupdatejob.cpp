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

#include "entryupdatejob.h"
#include "entrycomparison.h"
#include "document.h"
#include "entry.h"
#include "collection.h"
#include "tellico_debug.h"

#include <QTimer>

using Tellico::EntryUpdateJob;

EntryUpdateJob::EntryUpdateJob(QObject* parent_, Data::EntryPtr entry_, Fetch::Fetcher::Ptr fetcher_, Mode mode_)
    : KJob(parent_), m_entry(entry_), m_fetcher(fetcher_), m_mode(mode_) {
 setCapabilities(KJob::Killable);
 connect(m_fetcher.data(), SIGNAL(signalResultFound(Tellico::Fetch::FetchResult*)),
          SLOT(slotResult(Tellico::Fetch::FetchResult*)));
  connect(m_fetcher.data(), SIGNAL(signalDone(Tellico::Fetch::Fetcher*)),
          SLOT(slotDone()));
}

void EntryUpdateJob::start() {
  QTimer::singleShot(0, this, SLOT(startUpdate()));
}

void EntryUpdateJob::startUpdate() {
  m_fetcher->startUpdate(m_entry);
}

void EntryUpdateJob::slotResult(Tellico::Fetch::FetchResult* result_) {
  if(!result_) {
    myDebug() << "null result";
    return;
  }

  Data::EntryPtr entry = result_->fetchEntry();
  Q_ASSERT(entry);

  const int match = m_entry->collection()->sameEntry(m_entry, entry);
  // we insert using the negative count, so that the sorting in the map keys
  // allows us to get the best match by taking the first key
  m_fetchResults.insert(-1 * match, entry);
  if(match > EntryComparison::ENTRY_PERFECT_MATCH) {
    doKill();
  }
}

void EntryUpdateJob::slotDone() {
  QMapIterator<int, Data::EntryPtr> i(m_fetchResults);
  // we only care about the first one, since they're sorted by negative match score
  if(i.hasNext()) {
    i.next();
    const int match = -1 * i.key();
    const int matchToBeat = (m_mode == PerfectMatchOnly ? EntryComparison::ENTRY_PERFECT_MATCH
                                                        : EntryComparison::ENTRY_GOOD_MATCH);
    if(match > matchToBeat) {
      Data::Document::mergeEntry(m_entry, i.value(), false /*overwrite*/);
    }
  }
  emitResult();
}

bool EntryUpdateJob::doKill() {
  m_fetcher->stop();
  return true;
}

#include "entryupdatejob.moc"
