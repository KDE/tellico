/***************************************************************************
    Copyright (C) 2005-2011 Robby Stephenson <robby@periapsis.org>
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

#include "removeentries.h"
#include "removeloans.h"
#include "../collection.h"
#include "../controller.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

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

  // also need to allow for removing entries that might be loaned out
  // nothing for it but to do full-blown iterative search
  Data::LoanList loans;
  foreach(Data::BorrowerPtr borrower, m_coll->borrowers()) {
    foreach(Data::EntryPtr entry, m_entries) {
      if(borrower->hasEntry(entry)) {
        loans += borrower->loan(entry);
      }
    }
  }
  if(!loans.isEmpty()) {
    new RemoveLoans(loans, this);
  }
}

void RemoveEntries::redo() {
  if(!m_coll || m_entries.isEmpty()) {
    return;
  }

  m_coll->removeEntries(m_entries);
  Controller::self()->removedEntries(m_entries);

  QUndoCommand::redo();
}

void RemoveEntries::undo() {
  if(!m_coll || m_entries.isEmpty()) {
    return;
  }

  m_coll->addEntries(m_entries);
  Controller::self()->addedEntries(m_entries);

  QUndoCommand::undo();
}
