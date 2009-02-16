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

#include "addentries.h"
#include "../collection.h"
#include "../controller.h"
#include "../datavectors.h"
#include "../tellico_debug.h"

#include <klocale.h>

using Tellico::Command::AddEntries;

AddEntries::AddEntries(Tellico::Data::CollPtr coll_, const Tellico::Data::EntryList& entries_)
    : QUndoCommand()
    , m_coll(coll_)
    , m_entries(entries_)
{
  if(!m_entries.isEmpty()) {
    setText(m_entries.count() > 1 ? i18n("Add Entries")
                                : i18nc("Add (Entry Title)", "Add %1", m_entries[0]->title()));
  }
}

void AddEntries::redo() {
  if(!m_coll || m_entries.isEmpty()) {
    return;
  }

  m_coll->addEntries(m_entries);
  // now check for default values
  foreach(Data::FieldPtr field, m_coll->fields()) {
    const QString defaultValue = field->defaultValue();
    if(!defaultValue.isEmpty()) {
      foreach(Data::EntryPtr entry, m_entries) {
        if(entry->field(field).isEmpty()) {
          entry->setField(field->name(), defaultValue);
        }
      }
    }
  }
  Controller::self()->addedEntries(m_entries);
}

void AddEntries::undo() {
  if(!m_coll || m_entries.isEmpty()) {
    return;
  }

  m_coll->removeEntries(m_entries);
  Controller::self()->removedEntries(m_entries);
}
