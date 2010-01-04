/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>

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

#include "modifyentries.h"
#include "../collection.h"
#include "../controller.h"
#include "../tellico_debug.h"

#include <klocale.h>

using Tellico::Command::ModifyEntries;

ModifyEntries::ModifyEntries(Tellico::Data::CollPtr coll_, const Tellico::Data::EntryList& oldEntries_,
                             const Tellico::Data::EntryList& newEntries_, const QStringList& modifiedFields_)
    : QUndoCommand()
    , m_coll(coll_)
    , m_oldEntries(oldEntries_)
    , m_entries(newEntries_)
    , m_modifiedFields(modifiedFields_)
    , m_needToSwap(false)
{
#ifndef NDEBUG
  if(m_oldEntries.count() != m_entries.count()) {
    myWarning() << "unequal number of entries";
  }
#endif
  if(!m_entries.isEmpty()) {
    setText(m_entries.count() > 1 ? i18n("Modify Entries")
                                  : i18nc("Modify (Entry Title)", "Modify %1", m_entries[0]->title()));
  }
}

ModifyEntries::ModifyEntries(QUndoCommand* parent, Tellico::Data::CollPtr coll_, const Tellico::Data::EntryList& oldEntries_,
                             const Tellico::Data::EntryList& newEntries_, const QStringList& modifiedFields_)
    : QUndoCommand(parent)
    , m_coll(coll_)
    , m_oldEntries(oldEntries_)
    , m_entries(newEntries_)
    , m_modifiedFields(modifiedFields_)
    , m_needToSwap(false)
{
#ifndef NDEBUG
  if(m_oldEntries.count() != m_entries.count()) {
    myWarning() << "unequal number of entries";
  }
#endif
  if(!m_entries.isEmpty()) {
    setText(m_entries.count() > 1 ? i18n("Modify Entries")
                                  : i18nc("Modify (Entry Title)", "Modify %1", m_entries[0]->title()));
  }
}

void ModifyEntries::redo() {
  if(!m_coll || m_entries.isEmpty()) {
    return;
  }
  if(m_needToSwap) {
    swapValues();
    m_needToSwap = false;
  }
  // loans expose a field named "loaned", and the user might modify that without
  // checking in the loan, so verify that. Heavy-handed, yes...
  const QString loaned = QLatin1String("loaned");
  bool hasLoanField = m_coll->hasField(loaned);
  if(hasLoanField && m_modifiedFields.contains(loaned)) {
    foreach(Data::EntryPtr entry, m_entries) {
      if(entry->field(loaned).isEmpty()) {
        Data::EntryList notLoaned;
        notLoaned.append(entry);
        Controller::self()->slotCheckIn(notLoaned);
      }
    }
  }
  m_coll->updateDicts(m_entries, m_modifiedFields);
  Controller::self()->modifiedEntries(m_entries);
}

void ModifyEntries::undo() {
  if(!m_coll || m_entries.isEmpty()) {
    return;
  }
  swapValues();
  m_needToSwap = true;
  m_coll->updateDicts(m_entries, m_modifiedFields);
  Controller::self()->modifiedEntries(m_entries);
  //TODO: need to tell edit dialog that it's not modified
}

void ModifyEntries::swapValues() {
  // since things like the detailedlistview and the entryiconview hold pointers to the entries
  // can't just call Controller::modifiedEntry() on the old pointers
  for(int i = 0; i < m_entries.count(); ++i) {
    // need to swap entry values, not just pointers
    // the id gets reset when copying, so need to keep it
    const Data::ID id = m_entries[i]->id();
    Data::Entry tmp(*m_entries[i]); // tmp id becomes -1
    *m_entries[i] = *m_oldEntries[i]; // id becomes -1
    m_entries[i]->setId(id); // id becomes what was originally
    *m_oldEntries[i] = tmp; // id becomes -1
  }
}
