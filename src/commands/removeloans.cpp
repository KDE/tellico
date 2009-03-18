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

