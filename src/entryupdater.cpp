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

#include <klocale.h>
#include <kiconloader.h>
#include <kapplication.h>

#include <QTimer>

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
    connect(fetcher.data(), SIGNAL(signalResultFound(Tellico::Fetch::FetchResult*)),
            SLOT(slotResult(Tellico::Fetch::FetchResult*)));
    connect(fetcher.data(), SIGNAL(signalDone(Tellico::Fetch::Fetcher*)),
            SLOT(slotDone()));
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
    connect(f.data(), SIGNAL(signalResultFound(Tellico::Fetch::FetchResult*)),
            SLOT(slotResult(Tellico::Fetch::FetchResult*)));
    connect(f.data(), SIGNAL(signalDone(Tellico::Fetch::Fetcher*)),
            SLOT(slotDone()));
  }
  init();
}

EntryUpdater::~EntryUpdater() {
  foreach(const UpdateResult& res, m_results) {
    delete res.first;
  }
  m_results.clear();
}

void EntryUpdater::init() {
  m_fetchIndex = 0;
  m_origEntryCount = m_entriesToUpdate.count();
  QString label;
  if(m_entriesToUpdate.count() == 1) {
    label = i18n("Updating %1...", m_entriesToUpdate.front()->title());
  } else {
    label = i18n("Updating entries...");
  }
  Kernel::self()->beginCommandGroup(i18n("Update Entries"));
  ProgressItem& item = ProgressManager::self()->newProgressItem(this, label, true /*canCancel*/);
  item.setTotalSteps(m_fetchers.count() * m_origEntryCount);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));

  // done if no fetchers available
  if(m_fetchers.isEmpty()) {
    QTimer::singleShot(500, this, SLOT(slotCleanup()));
  } else {
    slotStartNext(); // starts fetching
  }
}

void EntryUpdater::slotStartNext() {
  StatusBar::self()->setStatus(i18n("Updating <b>%1</b>...", m_entriesToUpdate.front()->title()));
  ProgressManager::self()->setProgress(this, m_fetchers.count() * (m_origEntryCount - m_entriesToUpdate.count()) + m_fetchIndex);

  Fetch::Fetcher::Ptr f = m_fetchers[m_fetchIndex];
//  myDebug() << "starting " << f->source();
  f->startUpdate(m_entriesToUpdate.front());
}

void EntryUpdater::slotDone() {
  if(m_cancelled) {
    myLog() << "cancelled";
    QTimer::singleShot(500, this, SLOT(slotCleanup()));
    return;
  }

  if(!m_results.isEmpty()) {
    handleResults();
  }

  m_results.clear();
  ++m_fetchIndex;
//  myDebug() << m_fetchIndex;
  if(m_fetchIndex == m_fetchers.count()) {
    m_fetchIndex = 0;
    // we've gone through the loop for the first entry in the vector
    // pop it and move on
    m_entriesToUpdate.removeAll(m_entriesToUpdate.front());
    // if there are no more entries, and this is the last fetcher, time to delete
    if(m_entriesToUpdate.isEmpty()) {
      QTimer::singleShot(500, this, SLOT(slotCleanup()));
      return;
    }
  }
  kapp->processEvents();
  // so the entry updater can clean up a bit
  QTimer::singleShot(500, this, SLOT(slotStartNext()));
}

void EntryUpdater::slotResult(Tellico::Fetch::FetchResult* result_) {
  if(!result_ || m_cancelled) {
    return;
  }

//  myDebug() << result_->title << " [" << result_->fetcher->source() << "]";
  m_results.append(UpdateResult(result_, m_fetchers[m_fetchIndex]->updateOverwrite()));
  Data::EntryPtr e = result_->fetchEntry();
  if(e) {
    m_fetchedEntries.append(e);
    int match = m_coll->sameEntry(m_entriesToUpdate.front(), e);
    if(match > EntryComparison::ENTRY_PERFECT_MATCH) {
      result_->fetcher->stop();
    }
  }
  kapp->processEvents();
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
  Data::EntryPtr entry = m_entriesToUpdate.front();
  int best = 0;
  ResultList matches;
  foreach(const UpdateResult& res, m_results) {
    Data::EntryPtr e = res.first->fetchEntry();
    if(!e) {
      continue;
    }
    m_fetchedEntries.append(e);
    int match = m_coll->sameEntry(entry, e);
    if(match) {
//      myDebug() << e->title() << "matches by" << match;
    }
    // if the match is GOOD but not PERFECT, keep all of them
    if(match >= EntryComparison::ENTRY_PERFECT_MATCH) {
      if(match > best) {
        best = match;
        matches.clear();
        matches.append(res);
      } else if(match == best) {
        matches.append(res);
      }
    } else if(match >= EntryComparison::ENTRY_GOOD_MATCH) {
      best = qMax(best, match);
      // keep all the results that don't exceed the perfect match
      matches.append(res);
    } else if(match > best) {
      best = match;
      matches.clear();
      matches.append(res);
    }
  }
  if(best < EntryComparison::ENTRY_GOOD_MATCH) {
    if(best > 0) {
      myDebug() << "no good match (score > 10), best match =" << best << "(" << matches.count() << "matches)";
    }
    return;
  }
//  myDebug() << "best match = " << best << " (" << matches.count() << " matches)";
  UpdateResult match(0, true);
  if(matches.count() == 1) {
    match = matches.front();
  } else if(matches.count() > 1) {
    match = askUser(matches);
  }
  // askUser() could come back with nil
  if(match.first) {
    mergeCurrent(match.first->fetchEntry(), match.second);
  }
}

Tellico::EntryUpdater::UpdateResult EntryUpdater::askUser(const ResultList& results) {
  EntryMatchDialog dlg(Kernel::self()->widget(), m_entriesToUpdate.front(),
                       m_fetchers[m_fetchIndex], results);

  if(dlg.exec() != QDialog::Accepted) {
    return UpdateResult(0, false);
  }
  return dlg.updateResult();
}

void EntryUpdater::mergeCurrent(Tellico::Data::EntryPtr entry_, bool overWrite_) {
  Data::EntryPtr currEntry = m_entriesToUpdate.front();
  if(entry_) {
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
}

void EntryUpdater::slotCleanup() {
  StatusBar::self()->clearStatus();
  ProgressManager::self()->setDone(this);
  Kernel::self()->endCommandGroup();
  deleteLater();
}

#include "entryupdater.moc"
