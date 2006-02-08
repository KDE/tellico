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

#include "loanitem.h"
#include "entry.h"

using Tellico::LoanItem;

LoanItem::LoanItem(GUI::CountedItem* parent_, Tellico::Data::LoanPtr loan_)
 : Tellico::EntryItem(parent_, loan_->entry()), m_loan(loan_) {
}
