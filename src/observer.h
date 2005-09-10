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

#ifndef TELLICO_OBSERVER_H
#define TELLICO_OBSERVER_H

namespace Tellico {
  namespace Data {
    class Borrower;
    class Collection;
    class Entry;
    class Field;
  }
  class Filter;

/**
 * @author Robby Stephenson
 */
class Observer {

public:
  virtual void    addBorrower(Data::Borrower*) {}
  virtual void modifyBorrower(Data::Borrower*) {}
  // no removeBorrower()

  virtual void    addEntry(Data::Entry*) {}
  virtual void modifyEntry(Data::Entry*) {}
  virtual void removeEntry(Data::Entry*) {}

  virtual void    addField(Data::Collection*, Data::Field*) {}
  // coll, oldfield, newfield
  virtual void modifyField(Data::Collection*, Data::Field*, Data::Field*) {}
  virtual void removeField(Data::Collection*, Data::Field*) {}

  virtual void    addFilter(Filter*) {}
  virtual void modifyFilter(Filter*) {}
  virtual void removeFilter(Filter*) {}
};

}

#endif
