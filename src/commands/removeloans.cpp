/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#include "removeloans.h"
#include "../document.h"
#include "../entry.h"
#include "../controller.h"
#include "../utils/calendarhandler.h"
#include "../tellico_debug.h"

#include <klocale.h>

using Tellico::Command::RemoveLoans;

RemoveLoans::RemoveLoans(Tellico::Data::LoanList loans_)
    : QUndoCommand()
    , m_loans(loans_)
{
  if(!m_loans.isEmpty()) {
    setText(m_loans.count() > 1 ? i18n("Check-in Entries")
                                : i18nc("Check-in (Entry Title)", "Check-in %1", m_loans[0]->entry()->title()));
  }
}

void RemoveLoans::redo() {
  if(m_loans.isEmpty()) {
    return;
  }

  // not all of the loans might be in the calendar
  Data::LoanList calLoans;
  // remove the loans from the borrowers
  foreach(Data::LoanPtr loan, m_loans) {
    if(loan->inCalendar()) {
      calLoans.append(loan);
    }
    loan->borrower()->removeLoan(loan);
    Data::Document::self()->checkInEntry(loan->entry());
    Data::EntryList vec;
    vec.append(loan->entry());
    Controller::self()->modifiedEntries(vec);
    Controller::self()->modifiedBorrower(loan->borrower());
  }
  if(!calLoans.isEmpty()) {
    CalendarHandler::removeLoans(calLoans);
  }
}

void RemoveLoans::undo() {
  if(m_loans.isEmpty()) {
    return;
  }

  // not all of the loans might be in the calendar
  Data::LoanList calLoans;
  foreach(Data::LoanPtr loan, m_loans) {
    if(loan->inCalendar()) {
      calLoans.append(loan);
    }
    loan->borrower()->addLoan(loan);
    Data::Document::self()->checkOutEntry(loan->entry());
    Data::EntryList vec;
    vec.append(loan->entry());
    Controller::self()->modifiedEntries(vec);
    Controller::self()->modifiedBorrower(loan->borrower());
  }
  if(!calLoans.isEmpty()) {
    CalendarHandler::addLoans(calLoans);
  }
}

