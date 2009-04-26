/***************************************************************************
    copyright            : (C) 2003-2009 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BOOKCOLLECTION_H
#define BOOKCOLLECTION_H

#include "../collection.h"

namespace Tellico {
  namespace Data {

/**
 * A collection for books.
 *
 * @author Robby Stephenson
 */
class BookCollection : public Collection {
Q_OBJECT

public:
  /**
   * The constructor
   *
   * @param title The title of the collection
   */
  BookCollection(bool addDefaultFields, const QString& title = QString());

  virtual Type type() const { return Book; }
  virtual int sameEntry(Data::EntryPtr entry1, Data::EntryPtr entry2) const;

  static FieldList defaultFields();
};

  } // end namespace
} // end namespace
#endif
