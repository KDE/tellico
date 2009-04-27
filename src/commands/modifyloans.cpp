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
