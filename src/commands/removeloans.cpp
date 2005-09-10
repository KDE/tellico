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

#include "removeloans.h"
#include "../document.h"
#include "../entry.h"
#include "../controller.h"
#include "../calendarhandler.h"
#include "../tellico_debug.h"

#include <klocale.h>

using Tellico::Command::RemoveLoans;

RemoveLoans::RemoveLoans(Data::LoanVec loans_)
    : KCommand()
    , m_loans(loans_)
{
}

void RemoveLoans::execute() {
  if(m_loans.isEmpty()) {
    return;
  }

  // not all of the loans might be in the calendar
  Data::LoanVec calLoans;
  // remove the loans from the borrowers
  for(Data::LoanVec::Iterator loan = m_loans.begin(); loan != m_loans.end(); ++loan) {
    if(loan->inCalendar()) {
      calLoans.append(loan);
    }
    loan->borrower()->removeLoan(loan);
    Data::Document::self()->checkInEntry(loan->entry());
    Controller::self()->modifiedEntry(loan->entry());
    Controller::self()->modifiedBorrower(loan->borrower());
  }
  if(!calLoans.isEmpty()) {
    CalendarHandler::removeLoans(calLoans);
  }
}

void RemoveLoans::unexecute() {
  if(m_loans.isEmpty()) {
    return;
  }

  // not all of the loans might be in the calendar
  Data::LoanVec calLoans;
  for(Data::LoanVec::Iterator loan = m_loans.begin(); loan != m_loans.end(); ++loan) {
    if(loan->inCalendar()) {
      calLoans.append(loan);
    }
    loan->borrower()->addLoan(loan);
    Data::Document::self()->checkOutEntry(loan->entry());
    Controller::self()->modifiedEntry(loan->entry());
    Controller::self()->modifiedBorrower(loan->borrower());
  }
  if(!calLoans.isEmpty()) {
    CalendarHandler::addLoans(calLoans);
  }
}

QString RemoveLoans::name() const {
  return m_loans.count() > 1 ? i18n("Check-in Entries")
                             : i18n("Check-in (Entry Title)", "Check-in %1").arg(m_loans.begin()->entry()->title());
}
