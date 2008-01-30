/***************************************************************************
    copyright            : (C) 2007 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "entrymerger.h"
#include "entry.h"
#include "collection.h"
#include "tellico_kernel.h"
#include "controller.h"
#include "progressmanager.h"
#include "statusbar.h"
#include "tellico_debug.h"

#include <klocale.h>

#include <qtimer.h>

using Tellico::EntryMerger;

EntryMerger::EntryMerger(Data::EntryVec entries_, QObject* parent_)
    : QObject(parent_), m_entriesToCheck(entries_), m_origCount(entries_.count()), m_cancelled(false) {

  m_entriesLeft = m_entriesToCheck;
  Kernel::self()->beginCommandGroup(i18n("Merge Entries"));

  QString label = i18n("Merging entries...");
  ProgressItem& item = ProgressManager::self()->newProgressItem(this, label, true /*canCancel*/);
  item.setTotalSteps(m_origCount);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));

  // done if no entries to merge
  if(m_origCount < 2) {
    QTimer::singleShot(500, this, SLOT(slotCleanup()));
  } else {
    slotStartNext(); // starts fetching
  }
}

void EntryMerger::slotStartNext() {
  QString statusMsg = i18n("Total merged/scanned entries: %1/%2")
                      .arg(m_entriesToRemove.count())
                      .arg(m_origCount - m_entriesToCheck.count());
  StatusBar::self()->setStatus(statusMsg);
  ProgressManager::self()->setProgress(this, m_origCount - m_entriesToCheck.count());

  Data::EntryPtr baseEntry = m_entriesToCheck.front();
  Data::EntryVec::Iterator it = m_entriesToCheck.begin();
  ++it; // skip checking against first
  for( ; it != m_entriesToCheck.end(); ++it) {
    bool match = cleanMerge(baseEntry, &*it);
    if(!match) {
      int score = baseEntry->collection()->sameEntry(baseEntry, &*it);
      match = score >= Data::Collection::ENTRY_PERFECT_MATCH;
    }
    if(match) {
      bool merge_ok = baseEntry->collection()->mergeEntry(baseEntry, &*it, false /*overwrite*/, true /*askUser*/);
      if(merge_ok) {
        m_entriesToRemove.append(it);
        m_entriesLeft.remove(it);
      }
    }
  }
  m_entriesToCheck.remove(baseEntry);

  if(m_cancelled || m_entriesToCheck.count() < 2) {
    QTimer::singleShot(0, this, SLOT(slotCleanup()));
  } else {
    QTimer::singleShot(0, this, SLOT(slotStartNext()));
  }
}

void EntryMerger::slotCancel() {
  m_cancelled = true;
}

void EntryMerger::slotCleanup() {
  Kernel::self()->removeEntries(m_entriesToRemove);
  Controller::self()->slotUpdateSelection(0, m_entriesLeft);
  StatusBar::self()->clearStatus();
  ProgressManager::self()->setDone(this);
  Kernel::self()->endCommandGroup();
  deleteLater();
}

bool EntryMerger::cleanMerge(Data::EntryPtr e1, Data::EntryPtr e2) const {
  // figure out if there's a clean merge possible
  Data::FieldVec fields = e1->collection()->fields();
  for(Data::FieldVecIt it = fields.begin(); it != fields.end(); ++it) {
    QString val1 = e1->field(it);
    QString val2 = e2->field(it);
    if(val1 != val2 && !val1.isEmpty() && !val2.isEmpty()) {
      return false;
    }
  }
  return true;
}

#include "entrymerger.moc"
