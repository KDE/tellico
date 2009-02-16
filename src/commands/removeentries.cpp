/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
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

RemoveEntries::RemoveEntries(Tellico::Data::CollPtr coll_, const Tellico::Data::EntryList& entries_)
    : QUndoCommand()
    , m_coll(coll_)
    , m_entries(entries_)
{
  if(!m_entries.isEmpty()) {
    setText(m_entries.count() > 1 ? i18n("Delete Entries")
                                  : i18nc("Delete (Entry Title)", "Delete %1", m_entries[0]->title()));
  }
}

void RemoveEntries::redo() {
  if(!m_coll || m_entries.isEmpty()) {
    return;
  }

  m_coll->removeEntries(m_entries);
  Controller::self()->removedEntries(m_entries);
}

void RemoveEntries::undo() {
  if(!m_coll || m_entries.isEmpty()) {
    return;
  }

  m_coll->addEntries(m_entries);
  Controller::self()->addedEntries(m_entries);
}
