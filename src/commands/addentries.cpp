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

#include "addentries.h"
#include "../collection.h"
#include "../controller.h"
#include "../datavectors.h"
#include "../tellico_debug.h"

#include <klocale.h>

using Tellico::Command::AddEntries;

AddEntries::AddEntries(Data::CollPtr coll_, const Data::EntryVec& entries_)
    : KCommand()
    , m_coll(coll_)
    , m_entries(entries_)
{
}

void AddEntries::execute() {
  if(!m_coll || m_entries.isEmpty()) {
    return;
  }

  m_coll->addEntries(m_entries);
  Controller::self()->addedEntries(m_entries);
}

void AddEntries::unexecute() {
  if(!m_coll || m_entries.isEmpty()) {
    return;
  }

  m_coll->removeEntries(m_entries);
  Controller::self()->removedEntries(m_entries);
}

QString AddEntries::name() const {
  return m_entries.count() > 1 ? i18n("Add Entries")
                               : i18n("Add (Entry Title)", "Add %1").arg(m_entries.begin()->title());
}
