/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_LOANITEM_H
#define TELLICO_LOANITEM_H

#include "entryitem.h"
#include "borrower.h"

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class LoanItem : public Tellico::EntryItem {
public:
  LoanItem(GUI::CountedItem* parent, Data::LoanPtr loan);

  virtual bool isLoanItem() const { return true; }
  Data::LoanPtr loan() { return m_loan; }

private:
  Data::LoanPtr m_loan;
};

}

#endif
