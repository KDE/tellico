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

#include "addloans.h"
#include "../document.h"
#include "../entry.h"
#include "../collection.h"
#include "../controller.h"
#include "../calendarhandler.h"
#include "../tellico_debug.h"

#include <klocale.h>

using Tellico::Command::AddLoans;

AddLoans::AddLoans(Data::Borrower* borrower_, Data::LoanVec loans_, bool addToCalendar_)
    : KCommand()
    , m_borrower(borrower_)
    , m_loans(loans_)
    , m_addedLoanField(false)
    , m_addToCalendar(addToCalendar_)
{
}

void AddLoans::execute() {
  if(!m_borrower || m_loans.isEmpty()) {
    return;
  }

  // if the borrower is empty, assume it's getting added, otherwise it's being modified
  bool wasEmpty = m_borrower->isEmpty();

  // if there's no loaned field, we'll add one
  bool loanExisted = m_loans.begin()->entry()->collection()->hasField(QString::fromLatin1("loaned"));
  m_addedLoanField = false; // assume we didn't add the field yet

  // add the loans to the borrower
  for(Data::LoanVec::Iterator loan = m_loans.begin(); loan != m_loans.end(); ++loan) {
    m_borrower->addLoan(loan);
    Data::Document::self()->checkOutEntry(loan->entry());
    Controller::self()->modifiedEntry(loan->entry());
  }
  if(!loanExisted) {
    Data::Collection* c = m_loans.begin()->entry()->collection();
    Data::Field* f = c->fieldByName(QString::fromLatin1("loaned"));
    if(f) {
      // notify everything that a new field was added
      Controller::self()->addedField(c, f);
      m_addedLoanField = true;
    }
  }
  if(m_addToCalendar) {
    CalendarHandler::addLoans(m_loans);
  }
  if(wasEmpty) {
    m_loans.begin()->entry()->collection()->addBorrower(m_borrower);
    Controller::self()->addedBorrower(m_borrower);
  } else {
    // don't have to do anything to the document, it just holds a pointer
    myDebug() << "AddLoansCommand::execute() - modifying an existing borrower! " << endl;
    Controller::self()->modifiedBorrower(m_borrower);
  }
}

void AddLoans::unexecute() {
  if(!m_borrower) {
    return;
  }

  // remove the loans from the borrower
  for(Data::LoanVec::Iterator loan = m_loans.begin(); loan != m_loans.end(); ++loan) {
    m_borrower->removeLoan(loan);
    Data::Document::self()->checkInEntry(loan->entry());
    Controller::self()->modifiedEntry(loan->entry());
  }
  if(m_addedLoanField) {
    Data::Collection* c = m_loans.begin()->entry()->collection();
    Data::Field* f = c->fieldByName(QString::fromLatin1("loaned"));
    if(f) {
      c->removeField(f);
      Controller::self()->removedField(c, f);
    }
  }
  if(m_addToCalendar) {
    CalendarHandler::removeLoans(m_loans);
  }
  // the borrower object is kept in the document, it's just empty
  // it won't get saved in the document file
  // here, just notify everybody that it changed
  Controller::self()->modifiedBorrower(m_borrower);
}

QString AddLoans::name() const {
  return m_loans.count() > 1 ? i18n("Check-out Items")
                             : i18n("Check-out (Entry Title)", "Check-out %1").arg(m_loans.begin()->entry()->title());
}
