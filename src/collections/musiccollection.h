/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef MUSICCOLLECTION_H
#define MUSICCOLLECTION_H

#include "../collection.h"

namespace Tellico {
  namespace Data {

/**
 * A collection for music, like CD's and cassettes.
 *
 * @author Robby Stephenson
 */
class MusicCollection : public Collection {
Q_OBJECT

public:
  /**
   * The constructor
   *
   * @param title The title of the collection
   */
  MusicCollection(bool addDefaultFields, const QString& title = QString());

  virtual Type type() const { return Album; }
  virtual int sameEntry(Data::EntryPtr entry1, Data::EntryPtr entry2) const;

  static FieldList defaultFields();
};

  } // end namespace
} // end namespace
#endif
