/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "modifyentries.h"
#include "../collection.h"
#include "../controller.h"
#include "../tellico_debug.h"

#include <klocale.h>

using Tellico::Command::ModifyEntries;

ModifyEntries::ModifyEntries(Tellico::Data::CollPtr coll_, const Tellico::Data::EntryList& oldEntries_, const Tellico::Data::EntryList& newEntries_)
    : QUndoCommand()
    , m_coll(coll_)
    , m_oldEntries(oldEntries_)
    , m_entries(newEntries_)
    , m_needToSwap(false)
{
#ifndef NDEBUG
  if(m_oldEntries.count() != m_entries.count()) {
    kWarning() << "ModifyEntriesCommand() - unequal number of entries";
  }
#endif
  if(!m_entries.isEmpty()) {
    setText(m_entries.count() > 1 ? i18n("Modify Entries")
                                  : i18nc("Modify (Entry Title)", "Modify %1", m_entries[0]->title()));
  }
}

ModifyEntries::ModifyEntries(QUndoCommand* parent, Tellico::Data::CollPtr coll_, const Tellico::Data::EntryList& oldEntries_, const Tellico::Data::EntryList& newEntries_)
    : QUndoCommand(parent)
    , m_coll(coll_)
    , m_oldEntries(oldEntries_)
    , m_entries(newEntries_)
    , m_needToSwap(false)
{
#ifndef NDEBUG
  if(m_oldEntries.count() != m_entries.count()) {
    kWarning() << "ModifyEntriesCommand() - unequal number of entries";
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
  const QString loaned = QString::fromLatin1("loaned");
  bool hasLoanField = m_coll->hasField(loaned);
  if(hasLoanField) {
    foreach(Data::EntryPtr entry, m_entries) {
      if(entry->field(loaned).isEmpty()) {
        Data::EntryList notLoaned;
        notLoaned.append(entry);
        Controller::self()->slotCheckIn(notLoaned);
      }
    }
  }
  m_coll->updateDicts(m_entries);
  Controller::self()->modifiedEntries(m_entries);
}

void ModifyEntries::undo() {
  if(!m_coll || m_entries.isEmpty()) {
    return;
  }
  swapValues();
  m_needToSwap = true;
  m_coll->updateDicts(m_entries);
  Controller::self()->modifiedEntries(m_entries);
  //TODO: need to tell edit dialog that it's not modified
}

void ModifyEntries::swapValues() {
  // since things like the detailedlistview and the entryiconview hold pointers to the entries
  // can't just call Controller::modifiedEntry() on the old pointers
  for(int i = 0; i < m_entries.count(); ++i) {
    // need to swap entry values, not just pointers
    // the id gets reset when copying, so need to keep it
    long id = m_entries[i]->id();
    Data::Entry tmp(*m_entries[i]); // tmp id becomes -1
    *m_entries[i] = *m_oldEntries[i]; // id becomes -1
    m_entries[i]->setId(id); // id becomes what was originally
    *m_oldEntries[i] = tmp; // id becomes -1
  }
}
