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

#ifndef TELLICO_BORROWERITEM_H
#define TELLICO_BORROWERITEM_H

#include "gui/counteditem.h"
#include "datavectors.h"

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class BorrowerItem : public GUI::CountedItem {
public:
  BorrowerItem(GUI::ListView* parent, Data::BorrowerPtr filter);

  virtual bool isBorrowerItem() const { return true; }
  Data::BorrowerPtr borrower() { return m_borrower; }

  virtual int count() const;
  virtual Data::EntryVec entries() const;

private:
  Data::BorrowerPtr m_borrower;
};

}

#endif
