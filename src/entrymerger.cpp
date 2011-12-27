/***************************************************************************
    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
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

#include "entrymerger.h"
#include "entry.h"
#include "entrycomparison.h"
#include "collection.h"
#include "tellico_kernel.h"
#include "controller.h"
#include "progressmanager.h"
#include "gui/statusbar.h"
#include "tellico_debug.h"

#include <klocale.h>

#include <QTimer>

using namespace Tellico;
using Tellico::AskUserResolver;
using Tellico::EntryMerger;

Tellico::MergeConflictResolver::Result AskUserResolver::resolve(Data::EntryPtr entry1, Data::EntryPtr entry2, Data::FieldPtr field,
                                                                const QString& value1, const QString& value2) {
  return static_cast<MergeConflictResolver::Result>(Kernel::self()->askAndMerge(entry1, entry2, field, value1, value2));
}

EntryMerger::EntryMerger(Tellico::Data::EntryList entries_, QObject* parent_)
    : QObject(parent_), m_entriesToCheck(entries_), m_origCount(entries_.count()), m_cancelled(false)
    , m_resolver(new AskUserResolver) {

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

EntryMerger::~EntryMerger() {
  delete m_resolver;
}

void EntryMerger::slotStartNext() {
  QString statusMsg = i18n("Total merged/scanned entries: %1/%2",
                           m_entriesToRemove.count(),
                           m_origCount - m_entriesToCheck.count());
  StatusBar::self()->setStatus(statusMsg);
  ProgressManager::self()->setProgress(this, m_origCount - m_entriesToCheck.count());

  Data::EntryPtr baseEntry = m_entriesToCheck[0];
  for(int i = 1; i < m_entriesToCheck.count(); ++i) {  // skip checking against first
    Data::EntryPtr it = m_entriesToCheck[i];
    bool match = cleanMerge(baseEntry, it);
    if(!match) {
      int score = baseEntry->collection()->sameEntry(baseEntry, it);
      match = score >= EntryComparison::ENTRY_GOOD_MATCH;
    }
    if(match) {
      bool merge_ok = Data::Document::mergeEntry(baseEntry, it, m_resolver);
      if(merge_ok) {
        m_entriesToRemove.append(it);
        m_entriesLeft.removeAll(it);
      }
    }
  }
  m_entriesToCheck.removeAll(baseEntry);

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

bool EntryMerger::cleanMerge(Tellico::Data::EntryPtr e1, Tellico::Data::EntryPtr e2) const {
  // figure out if there's a clean merge possible
  foreach(Data::FieldPtr field, e1->collection()->fields()) {
    // do not care about id and dates
    if(field->name() == QLatin1String("id") ||
       field->name() == QLatin1String("cdate") ||
       field->name() == QLatin1String("mdate")) {
      continue;
    }
    QString val1 = e1->field(field);
    QString val2 = e2->field(field);
    if(val1 != val2 && !val1.isEmpty() && !val2.isEmpty()) {
      return false;
    }
  }
  return true;
}

#include "entrymerger.moc"
