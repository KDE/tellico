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

#ifndef TELLICO_REMOVELOANS_H
#define TELLICO_REMOVELOANS_H

#include "../borrower.h"

#include <kcommand.h>

namespace Tellico {
  namespace Command {

/**
 * @author Robby Stephenson
 */
class RemoveLoans : public KCommand  {

public:
  RemoveLoans(Data::LoanVec loans);

  virtual void execute();
  virtual void unexecute();
  virtual QString name() const;

private:
  Data::LoanVec m_loans;
};

  } // end namespace
}

#endif
