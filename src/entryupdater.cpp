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

#include "entryupdater.h"
#include "entry.h"
#include "entrycomparison.h"
#include "collection.h"
#include "tellico_kernel.h"
#include "progressmanager.h"
#include "gui/statusbar.h"
#include "document.h"
#include "fetch/fetchresult.h"
#include "entrymatchdialog.h"
#include "tellico_debug.h"

#include <KLocalizedString>

#include <QTimer>
#include <QApplication>

namespace {
  static const int CHECK_COLLECTION_IMAGES_STEP_SIZE = 10;
}

using Tellico::EntryUpdater;

// for each entry, we loop over all available fetchers
// then we loop over all entries
EntryUpdater::EntryUpdater(Tellico::Data::CollPtr coll_, Tellico::Data::EntryList entries_, QObject* parent_)
    : QObject(parent_)
    , m_coll(coll_)
    , m_entriesToUpdate(entries_)
    , m_cancelled(false) {
  // for now, we're assuming all entries are same collection type
  m_fetchers = Fetch::Manager::self()->createUpdateFetchers(m_coll->type());
  foreach(Fetch::Fetcher::Ptr fetcher, m_fetchers) {
    connect(fetcher.data(), &Fetch::Fetcher::signalResultFound,
            this, &EntryUpdater::slotResult);
    connect(fetcher.data(), &Fetch::Fetcher::signalDone,
            this, &EntryUpdater::slotDone);
  }
  init();
}

EntryUpdater::EntryUpdater(const QString& source_, Tellico::Data::CollPtr coll_, Tellico::Data::EntryList entries_, QObject* parent_)
    : QObject(parent_)
    , m_coll(coll_)
    , m_entriesToUpdate(entries_)
    , m_cancelled(false) {
  // for now, we're assuming all entries are same collection type
  Fetch::Fetcher::Ptr f = Fetch::Manager::self()->createUpdateFetcher(m_coll->type(), source_);
  if(f) {
    m_fetchers.append(f);
    connect(f.data(), &Fetch::Fetcher::signalResultFound,
            this, &EntryUpdater::slotResult);
    connect(f.data(), &Fetch::Fetcher::signalDone,
            this, &EntryUpdater::slotDone);
  }
  init();
}

EntryUpdater::~EntryUpdater() {
  foreach(const UpdateResult& res, m_results) {
    delete res.result;
  }
  m_results.clear();
}

void EntryUpdater::init() {
  m_fetchIndex = 0;
  m_origEntryCount = m_entriesToUpdate.count();
  QString label;
  if(m_entriesToUpdate.count() == 1) {
    label = i18n("Updating <b>%1</b>...", m_entriesToUpdate.front()->title());
  } else {
    label = i18n("Updating entries...");
  }
  Kernel::self()->beginCommandGroup(i18n("Update Entries"));
  ProgressItem& item = ProgressManager::self()->newProgressItem(this, label, true /*canCancel*/);
  item.setTotalSteps(m_fetchers.count() * m_origEntryCount);
  connect(&item, &Tellico::ProgressItem::signalCancelled,
          this, &Tellico::EntryUpdater::slotCancel);

  // done if no fetchers available
  if(m_fetchers.isEmpty()) {
    QTimer::singleShot(500, this, &EntryUpdater::slotCleanup);
  } else {
    slotStartNext(); // starts fetching
  }
}

void EntryUpdater::slotStartNext() {
  Fetch::Fetcher::Ptr f = m_fetchers[m_fetchIndex];
  Q_ASSERT(f);
  StatusBar::self()->setStatus(i18n("Updating <b>%1</b> from <i>%2</i>...",
                                    m_entriesToUpdate.front()->title(),
                                    f->source()));
  ProgressManager::self()->setProgress(this, m_fetchers.count() * (m_origEntryCount - m_entriesToUpdate.count()) + m_fetchIndex);

  f->startUpdate(m_entriesToUpdate.front());
}

void EntryUpdater::slotDone() {
  if(m_cancelled) {
    QTimer::singleShot(500, this, &EntryUpdater::slotCleanup);
    return;
  }

  if(m_results.isEmpty()) {
    Fetch::Fetcher::Ptr f = m_fetchers[m_fetchIndex];
    Q_ASSERT(f);
    myLog() << "No search results found to update entry from" << f->source();
  } else {
    handleResults();
  }

  m_results.clear();
  ++m_fetchIndex;
  if(m_fetchIndex == m_fetchers.count()) {
    m_fetchIndex = 0;
    // we've gone through the loop for the first entry in the vector
    // pop it and move on
    m_entriesToUpdate.removeAll(m_entriesToUpdate.front());
    // if there are no more entries, and this is the last fetcher, time to delete
    if(m_entriesToUpdate.isEmpty()) {
      QTimer::singleShot(500, this, &EntryUpdater::slotCleanup);
      return;
    }
  }
  qApp->processEvents();
  // so the entry updater can clean up a bit
  QTimer::singleShot(500, this, &EntryUpdater::slotStartNext);
}

void EntryUpdater::slotResult(Tellico::Fetch::FetchResult* result_) {
  if(!result_ || m_cancelled) {
    return;
  }
  auto fetcher = m_fetchers[m_fetchIndex];
  if(!fetcher || !fetcher->isSearching()) {
    return;
  }

  Data::EntryPtr matchEntry = result_->fetchEntry();
  if(matchEntry && !m_entriesToUpdate.isEmpty()) {
    m_fetchedEntries.append(matchEntry);
    const int match = m_coll->sameEntry(m_entriesToUpdate.front(), matchEntry);
    m_results.append(UpdateResult(result_, match));
    myLog() << "Found match:" << matchEntry->title() << "- score =" << match;
    if(match >= EntryComparison::ENTRY_PERFECT_MATCH) {
      myLog() << "Score exceeds high confidence threshold, stopping search";
      fetcher->stop();
    }
  }
  qApp->processEvents();
}

void EntryUpdater::slotCancel() {
  m_cancelled = true;
  Fetch::Fetcher::Ptr f = m_fetchers[m_fetchIndex];
  if(f) {
    f->stop(); // ends up calling slotDone();
  } else {
    slotDone();
  }
}

void EntryUpdater::handleResults() {
  Data::EntryPtr entryToUpdate = m_entriesToUpdate.front();
  int bestScore = 0;
  ResultList matches;
  foreach(const UpdateResult& res, m_results) {
    Data::EntryPtr matchEntry = res.result->fetchEntry();
    if(!matchEntry) {
      continue;
    }
    const int match = res.matchScore;
    // if the match is GOOD but not PERFECT, keep all of them
    if(match >= EntryComparison::ENTRY_PERFECT_MATCH) {
      if(match > bestScore) {
        bestScore = match;
        matches.clear();
        matches.append(res);
      } else if(match == bestScore) {
        // multiple "perfect" matches
        matches.append(res);
      }
    } else if(match >= EntryComparison::ENTRY_GOOD_MATCH) {
      myLog() << "Found good match:" << matchEntry->title() << "- score=" << match;
      bestScore = qMax(bestScore, match);
      // keep all the results that don't exceed the perfect match
      matches.append(res);
    } else if(match > bestScore) {
      myLog() << "Found better match:" << matchEntry->title() << "- score=" << match;
      bestScore = match;
      matches.clear();
      matches.append(res);
    } else if(m_results.count() == 1 && bestScore == 0 && entryToUpdate->title().isEmpty()) {
      // special case for updates which may backfire, but let's go with it
      // if there is a single result AND the best match is zero AND title is empty
      // let's assume it's a case where an entry with a single url or link was updated
      myLog() << "Updating entry with 0 score and empty title:" << matchEntry->title();
      bestScore = EntryComparison::ENTRY_PERFECT_MATCH;
      matches.append(res);
    }
  }
  if(bestScore < EntryComparison::ENTRY_GOOD_MATCH) {
    if(bestScore > 0) {
      myLog() << "Best match is not good enough, not updating the entry";
    }
    return;
  }
  UpdateResult match;
  if(matches.count() == 1) {
    match = matches.front();
  } else if(matches.count() > 1) {
    myLog() << "Found" << matches.count() << "good results";
    match = askUser(matches);
  }
  // askUser() could come back with nil
  if(match.result) {
    myLog() << "Best match is good enough, updating the entry";
    mergeCurrent(match.result->fetchEntry(), match.result->fetcher()->updateOverwrite());
  }
}

Tellico::EntryUpdater::UpdateResult EntryUpdater::askUser(const ResultList& results) {
  EntryMatchDialog dlg(Kernel::self()->widget(), m_entriesToUpdate.front(),
                       m_fetchers[m_fetchIndex].data(), results);

  if(dlg.exec() != QDialog::Accepted) {
    return UpdateResult();
  }
  return dlg.updateResult();
}

void EntryUpdater::mergeCurrent(Tellico::Data::EntryPtr entry_, bool overWrite_) {
  if(!entry_) {
    return;
  }

  Data::EntryPtr currEntry = m_entriesToUpdate.front();
  m_matchedEntries.append(entry_);
  Kernel::self()->updateEntry(currEntry, entry_, overWrite_);
  if(m_entriesToUpdate.count() % CHECK_COLLECTION_IMAGES_STEP_SIZE == 1) {
    // I don't want to remove any images in the entries that are getting
    // updated since they'll reference them later and the command isn't
    // executed until the command history group is finished
    // so remove pointers to matched entries
    Data::EntryList nonUpdatedEntries = m_fetchedEntries;
    foreach(Data::EntryPtr match, m_matchedEntries) {
      nonUpdatedEntries.removeAll(match);
    }
    Data::Document::self()->removeImagesNotInCollection(nonUpdatedEntries, m_matchedEntries);
  }
}

void EntryUpdater::slotCleanup() {
  ProgressManager::self()->setDone(this);
  StatusBar::self()->clearStatus();
  Kernel::self()->endCommandGroup();
  deleteLater();
}
