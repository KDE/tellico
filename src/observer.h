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

#ifndef TELLICO_OBSERVER_H
#define TELLICO_OBSERVER_H

#include "datavectors.h"

namespace Tellico {
  class Filter;

/**
 * @author Robby Stephenson
 */
class Observer {

public:
  virtual ~Observer() {}

  virtual void    addBorrower(Data::BorrowerPtr) {}
  virtual void modifyBorrower(Data::BorrowerPtr) {}
  // no removeBorrower()

  virtual void    addEntries(Data::EntryVec) {}
  virtual void modifyEntries(Data::EntryVec) {}
  virtual void removeEntries(Data::EntryVec) {}

  virtual void    addField(Data::CollPtr, Data::FieldPtr) {}
  // coll, oldfield, newfield
  virtual void modifyField(Data::CollPtr, Data::FieldPtr, Data::FieldPtr) {}
  virtual void removeField(Data::CollPtr, Data::FieldPtr) {}

  virtual void    addFilter(FilterPtr) {}
  virtual void modifyFilter(FilterPtr) {}
  virtual void removeFilter(FilterPtr) {}
};

}

#endif
