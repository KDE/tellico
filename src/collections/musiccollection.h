/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
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
 * It has the following standard attributes:
 * @li Title
 * @li Artist
 * @li Album
 * @li Year
 * @li Genre
 * @li Comments
 *
 * @author Robby Stephenson
 */
class MusicCollection : public Collection {
Q_OBJECT

public:
  /**
   * The constructor
   *
   * @param addFields Whether to add the default attributes
   * @param title The title of the collection
   */
  MusicCollection(bool addFields, const QString& title = QString::null);

  virtual Type type() const { return Album; }

  static FieldVec defaultFields();
};

  } // end namespace
} // end namespace
#endif
