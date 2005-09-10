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

#ifndef TELLICO_ADDLOANS_H
#define TELLICO_ADDLOANS_H

#include "../borrower.h"
#include "../datavectors.h"

#include <kcommand.h>

namespace Tellico {
  namespace Command {

/**
 * @author Robby Stephenson
 */
class AddLoans : public KCommand  {

public:
  AddLoans(Data::Borrower* borrower, Data::LoanVec loans, bool addToCalendar);

  virtual void execute();
  virtual void unexecute();
  virtual QString name() const;

private:
  Data::BorrowerPtr m_borrower;
  Data::LoanVec m_loans;
  bool m_addedLoanField : 1;
  bool m_addToCalendar : 1;
};

  } // end namespace
}

#endif
