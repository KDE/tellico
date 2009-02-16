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

#ifndef TELLICO_ADDLOANS_H
#define TELLICO_ADDLOANS_H

#include "../borrower.h"
#include "../datavectors.h"

#include <QUndoCommand>

namespace Tellico {
  namespace Command {

/**
 * @author Robby Stephenson
 */
class AddLoans : public QUndoCommand  {

public:
  AddLoans(Data::BorrowerPtr borrower, Data::LoanList loans, bool addToCalendar);

  virtual void redo();
  virtual void undo();

private:
  Data::BorrowerPtr m_borrower;
  Data::LoanList m_loans;
  bool m_addedLoanField : 1;
  bool m_addToCalendar : 1;
};

  } // end namespace
}

#endif
