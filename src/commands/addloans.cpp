/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>

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

#include "addloans.h"
#include "../document.h"
#include "../entry.h"
#include "../collection.h"
#include "../controller.h"
#include "../tellico_debug.h"

#include <klocale.h>

using Tellico::Command::AddLoans;

AddLoans::AddLoans(Tellico::Data::BorrowerPtr borrower_, Tellico::Data::LoanList loans_, bool addToCalendar_)
    : QUndoCommand()
    , m_borrower(borrower_)
    , m_loans(loans_)
    , m_addedLoanField(false)
    , m_addToCalendar(addToCalendar_)
{
  if(m_loans.isEmpty()) {
    myWarning() << "no loans!";
  } else {
    setText(m_loans.count() > 1 ? i18n("Check-out Items")
                                : i18nc("Check-out (Entry Title)", "Check-out %1", m_loans[0]->entry()->title()));
  }
}

void AddLoans::redo() {
  if(!m_borrower || m_loans.isEmpty()) {
    return;
  }

  // if the borrower is empty, assume it's getting added, otherwise it's being modified
  // the collection actually does the real check in addBorrower
  bool wasEmpty = m_borrower->isEmpty();

  // if there's no loaned field, we'll add one
  bool loanExisted = m_loans[0]->entry()->collection()->hasField(QLatin1String("loaned"));
  m_addedLoanField = false; // assume we didn't add the field yet

  // add the loans to the borrower
  foreach(Data::LoanPtr loan, m_loans) {
    m_borrower->addLoan(loan);
    Data::Document::self()->checkOutEntry(loan->entry());
    Data::EntryList vec;
    vec.append(loan->entry());
    Controller::self()->modifiedEntries(vec);
  }
  if(!loanExisted) {
    Data::CollPtr c = m_loans[0]->entry()->collection();
    Data::FieldPtr f = c->fieldByName(QLatin1String("loaned"));
    if(f) {
      // notify everything that a new field was added
      Controller::self()->addedField(c, f);
      m_addedLoanField = true;
    }
  }
  if(m_addToCalendar) {
    myWarning() << "Add to calendar not implemented";
  }
  if(wasEmpty) {
    m_loans[0]->entry()->collection()->addBorrower(m_borrower);
    Controller::self()->addedBorrower(m_borrower);
  } else {
    // don't have to do anything to the document, it just holds a pointer
    Controller::self()->modifiedBorrower(m_borrower);
  }
}

void AddLoans::undo() {
  if(!m_borrower) {
    return;
  }

  // remove the loans from the borrower
  foreach(Data::LoanPtr loan, m_loans) {
    m_borrower->removeLoan(loan);
    Data::Document::self()->checkInEntry(loan->entry());
    Data::EntryList vec;
    vec.append(loan->entry());
    Controller::self()->modifiedEntries(vec);
  }
  if(m_addedLoanField) {
    Data::CollPtr c = m_loans[0]->entry()->collection();
    Data::FieldPtr f = c->fieldByName(QLatin1String("loaned"));
    if(f) {
      c->removeField(f);
      Controller::self()->removedField(c, f);
    }
  }
  if(m_addToCalendar) {
    myWarning() << "Add to calendar not implemented";
  }
  // the borrower object is kept in the document, it's just empty
  // it won't get saved in the document file
  // here, just notify everybody that it changed
  Controller::self()->modifiedBorrower(m_borrower);
}
