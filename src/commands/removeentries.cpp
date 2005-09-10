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

#include "removeentries.h"
#include "../collection.h"
#include "../controller.h"

#include <klocale.h>

using Tellico::Command::RemoveEntries;

RemoveEntries::RemoveEntries(Data::Collection* coll_, Data::EntryVec entries_)
    : KCommand()
    , m_coll(coll_)
    , m_entries(entries_)
{
}

void RemoveEntries::execute() {
  if(!m_coll || m_entries.isEmpty()) {
    return;
  }

  for(Data::EntryVecIt entry = m_entries.begin(); entry != m_entries.end(); ++entry) {
    m_coll->removeEntry(entry);
  }
  Controller::self()->removedEntries(m_entries);
}

void RemoveEntries::unexecute() {
  if(!m_coll || m_entries.isEmpty()) {
    return;
  }

  for(Data::EntryVecIt entry = m_entries.begin(); entry != m_entries.end(); ++entry) {
    m_coll->addEntry(entry);
  }
  Controller::self()->addedEntries(m_entries);
}

QString RemoveEntries::name() const {
  return m_entries.count() > 1 ? i18n("Delete Entries")
                               : i18n("Delete (Entry Title)", "Delete %1").arg(m_entries.begin()->title());
}
