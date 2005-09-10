/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "addentries.h"
#include "../collection.h"
#include "../controller.h"
#include "../datavectors.h"
#include "../tellico_debug.h"

#include <klocale.h>

using Tellico::Command::AddEntries;

AddEntries::AddEntries(Data::Collection* coll_, Data::EntryVec entries_)
    : KCommand()
    , m_coll(coll_)
    , m_entries(entries_)
{
}

void AddEntries::execute() {
  if(!m_coll || m_entries.isEmpty()) {
    return;
  }

  for(Data::EntryVecIt entry = m_entries.begin(); entry != m_entries.end(); ++entry) {
    m_coll->addEntry(entry);
  }
  Controller::self()->addedEntries(m_entries);
}

void AddEntries::unexecute() {
  if(!m_coll || m_entries.isEmpty()) {
    return;
  }

  for(Data::EntryVecIt entry = m_entries.begin(); entry != m_entries.end(); ++entry) {
    m_coll->removeEntry(entry);
  }
  Controller::self()->removedEntries(m_entries);
}

QString AddEntries::name() const {
  // FIXME: after string unfreeze
//  return m_entries.count() > 1 ? i18n("Save Entries")
//                               : i18n("Save (Entry Title)", "Save %1").arg(m_entries.begin()->title());
  return i18n("Save (Entry Title)", "Save %1").arg(m_entries.begin()->title());
}
