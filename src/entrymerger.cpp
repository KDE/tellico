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

using Tellico::EntryMerger;

EntryMerger::EntryMerger(Tellico::Data::EntryList entries_, QObject* parent_)
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
      match = score >= EntryComparison::ENTRY_PERFECT_MATCH;
    }
    if(match) {
      bool merge_ok = mergeEntry(baseEntry, it, false /*overwrite*/, true /*askUser*/);
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
  foreach(Data::FieldPtr it, e1->collection()->fields()) {
    QString val1 = e1->field(it);
    QString val2 = e2->field(it);
    if(val1 != val2 && !val1.isEmpty() && !val2.isEmpty()) {
      return false;
    }
  }
  return true;
}

// static
// merges values from e2 into e1
bool EntryMerger::mergeEntry(Data::EntryPtr e1, Data::EntryPtr e2, bool overwrite_, bool askUser_) {
  if(!e1 || !e2) {
    myDebug() << "bad entry pointer";
    return false;
  }
  bool ret = true;
  Data::FieldList fields = e1->collection()->fields();
  foreach(Data::FieldPtr field, fields) {
    if(e2->field(field).isEmpty()) {
      continue;
    }
//    myLog() << "reading field: " << field->name();
    if(overwrite_ || e1->field(field).isEmpty()) {
//      myLog() << e1->title() << ": updating field(" << field->name() << ") to " << e2->field(field->name());
      e1->setField(field, e2->field(field));
      ret = true;
    } else if(e1->field(field) == e2->field(field)) {
      continue;
    } else if(field->type() == Data::Field::Para) {
      // for paragraph fields, concatenate the values, if they're not equal
      e1->setField(field, e1->field(field) + QLatin1String("<br/><br/>") + e2->field(field));
      ret = true;
    } else if(field->type() == Data::Field::Table) {
      // if field F is a table-type field (album tracks, files, etc.), merge rows (keep their position)
      // if e1's F val in [row i, column j] empty, replace with e2's val at same position
      // if different (non-empty) vals at same position, CONFLICT!
      const QString sep = QLatin1String("::");
      QStringList vals1 = e1->fields(field, false);
      QStringList vals2 = e2->fields(field, false);
      while(vals1.count() < vals2.count()) {
        vals1 += QString();
      }
      for(int i = 0; i < vals2.count(); ++i) {
        if(vals2[i].isEmpty()) {
          continue;
        }
        if(vals1[i].isEmpty()) {
          vals1[i] = vals2[i];
          ret = true;
        } else {
          QStringList parts1 = vals1[i].split(sep);
          QStringList parts2 = vals2[i].split(sep);
          bool changedPart = false;
          while(parts1.count() < parts2.count()) {
            parts1 += QString();
          }
          for(int j = 0; j < parts2.count(); ++j) {
            if(parts2[j].isEmpty()) {
              continue;
            }
            if(parts1[j].isEmpty()) {
              parts1[j] = parts2[j];
              changedPart = true;
            } else if(askUser_ && parts1[j] != parts2[j]) {
              int ret = Kernel::self()->askAndMerge(e1, e2, field, parts1[j], parts2[j]);
              if(ret == 0) {
                return false; // we got cancelled
              }
              if(ret == 1) {
                parts1[j] = parts2[j];
                changedPart = true;
              }
            }
          }
          if(changedPart) {
            vals1[i] = parts1.join(sep);
            ret = true;
          }
        }
      }
      if(ret) {
        e1->setField(field, vals1.join(QLatin1String("; ")));
      }
// remove the merging due to user comments
// maybe in the future have a more intelligent way
#if 0
    } else if(field->hasFlag(Data::Field::AllowMultiple)) {
      // if field F allows multiple values and not a Table (see above case),
      // e1's F values = (e1's F values) U (e2's F values) (union)
      // replace e1's field with union of e1's and e2's values for this field
      QStringList items1 = e1->fields(field, false);
      QStringList items2 = e2->fields(field, false);
      foreach(const QString& item2, items2) {
        // possible to have one value formatted and the other one not...
        if(!items1.contains(item2) && !items1.contains(Field::format(item2, field->formatFlag()))) {
          items1.append(item2);
        }
      }
// not sure if I think it should be sorted or not
//      items1.sort();
      e1->setField(field, items1.join(QLatin1String("; ")));
      ret = true;
#endif
    } else if(askUser_ && e1->field(field) != e2->field(field)) {
      int ret = Kernel::self()->askAndMerge(e1, e2, field);
      if(ret == 0) {
        return false; // we got cancelled
      }
      if(ret == 1) {
        e1->setField(field, e2->field(field));
      }
    }
  }
  return ret;
}

#include "entrymerger.moc"
