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

#include "modifyloans.h"
#include "../document.h"
#include "../entry.h"
#include "../controller.h"
#include "../utils/calendarhandler.h"

#include <klocale.h>

using Tellico::Command::ModifyLoans;

ModifyLoans::ModifyLoans(Tellico::Data::LoanPtr oldLoan_, Tellico::Data::LoanPtr newLoan_, bool addToCalendar_)
    : QUndoCommand(i18n("Modify Loan"))
    , m_oldLoan(oldLoan_)
    , m_newLoan(newLoan_)
    , m_addToCalendar(addToCalendar_)
{
}

void ModifyLoans::redo() {
  if(!m_oldLoan || !m_newLoan) {
    return;
  }

  Data::BorrowerPtr b = m_oldLoan->borrower();
  b->removeLoan(m_oldLoan);
  b->addLoan(m_newLoan);
  Controller::self()->modifiedBorrower(b);

  if(m_addToCalendar && !m_oldLoan->inCalendar()) {
    Data::LoanList loans;
    loans.append(m_newLoan);
    CalendarHandler::addLoans(loans);
  } else if(!m_addToCalendar && m_oldLoan->inCalendar()) {
    Data::LoanList loans;
    loans.append(m_newLoan); // CalendarHandler checks via uid
    CalendarHandler::removeLoans(loans);
  }
}

void ModifyLoans::undo() {
  if(!m_oldLoan || !m_newLoan) {
    return;
  }

  Data::BorrowerPtr b = m_oldLoan->borrower();
  b->removeLoan(m_newLoan);
  b->addLoan(m_oldLoan);
  Controller::self()->modifiedBorrower(b);

  if(m_addToCalendar && !m_oldLoan->inCalendar()) {
    Data::LoanList loans;
    loans.append(m_newLoan);
    CalendarHandler::removeLoans(loans);
  } else if(!m_addToCalendar && m_oldLoan->inCalendar()) {
    Data::LoanList loans;
    loans.append(m_oldLoan);
    CalendarHandler::addLoans(loans);
  }
}
