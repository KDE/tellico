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

#ifndef TELLICO_MODIFYLOANS_H
#define TELLICO_MODIFYLOANS_H

#include "../borrower.h"

#include <kcommand.h>

namespace Tellico {
  namespace Command {

/**
 * @author Robby Stephenson
 */
class ModifyLoans : public KCommand  {

public:
  ModifyLoans(Data::Loan* oldLoan, Data::Loan* newLoan, bool addToCalendar);

  virtual void execute();
  virtual void unexecute();
  virtual QString name() const;

private:
  Data::LoanPtr m_oldLoan;
  Data::LoanPtr m_newLoan;
  bool m_addToCalendar : 1;
};

  } // end namespace
}

#endif
