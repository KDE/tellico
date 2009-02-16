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

#ifndef TELLICO_REMOVELOANS_H
#define TELLICO_REMOVELOANS_H

#include "../borrower.h"

#include <QUndoCommand>

namespace Tellico {
  namespace Command {

/**
 * @author Robby Stephenson
 */
class RemoveLoans : public QUndoCommand  {

public:
  RemoveLoans(Data::LoanList loans);

  virtual void redo();
  virtual void undo();

private:
  Data::LoanList m_loans;
};

  } // end namespace
}

#endif
