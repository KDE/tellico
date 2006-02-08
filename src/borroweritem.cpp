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

#include "borroweritem.h"
#include "borrower.h"

#include "kiconloader.h"

using Tellico::BorrowerItem;

BorrowerItem::BorrowerItem(GUI::ListView* parent_, Data::BorrowerPtr borrower_)
    : GUI::CountedItem(parent_), m_borrower(borrower_) {
  setText(0, borrower_->name());
  setPixmap(0, SmallIcon(QString::fromLatin1("kaddressbook")));
}

int BorrowerItem::count() const {
  return m_borrower ? m_borrower->count() : GUI::CountedItem::count();
}
