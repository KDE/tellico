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

ModifyEntries::ModifyEntries(Data::CollPtr coll_, const Data::EntryVec& oldEntries_, const Data::EntryVec& newEntries_)
    : KCommand()
    , m_coll(coll_)
    , m_oldEntries(oldEntries_)
    , m_entries(newEntries_)
    , m_needToSwap(false)
{
#ifndef NDEBUG
  if(m_oldEntries.count() != m_entries.count()) {
    kdDebug() << "ModifyEntriesCommand() - unequal number of entries" << endl;
  }
#endif
}

void ModifyEntries::execute() {
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
  for(Data::EntryVecIt entry = m_entries.begin(); hasLoanField && entry != m_entries.end(); ++entry) {
    if(entry->field(loaned).isEmpty()) {
      Data::EntryVec notLoaned;
      notLoaned.append(entry);
      Controller::self()->slotCheckIn(notLoaned);
    }
  }
  m_coll->updateDicts(m_entries);
  Controller::self()->modifiedEntries(m_entries);
}

void ModifyEntries::unexecute() {
  if(!m_coll || m_entries.isEmpty()) {
    return;
  }
  swapValues();
  m_needToSwap = true;
  m_coll->updateDicts(m_entries);
  Controller::self()->modifiedEntries(m_entries);
  //TODO: need to tell edit dialog that it's not modified
}

QString ModifyEntries::name() const {
  return m_entries.count() > 1 ? i18n("Modify Entries")
                               : i18n("Modify (Entry Title)", "Modify %1").arg(m_entries.begin()->title());
}

void ModifyEntries::swapValues() {
  // since things like the detailedlistview and the entryiconview hold pointers to the entries
  // can't just call Controller::modifiedEntry() on the old pointers
  for(size_t i = 0; i < m_entries.count(); ++i) {
    // need to swap entry values, not just pointers
    // the id gets reset when copying, so need to keep it
    int id = m_entries.at(i)->id();
    Data::Entry tmp(*m_entries.at(i)); // tmp id becomes -1
    *m_entries.at(i) = *m_oldEntries.at(i); // id becomes -1
    m_entries.at(i)->setId(id); // id becomes what was originally
    *m_oldEntries.at(i) = tmp; // id becomes -1
  }
}
