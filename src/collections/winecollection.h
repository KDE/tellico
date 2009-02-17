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

#ifndef WINECOLLECTION_H
#define WINECOLLECTION_H

#include "../collection.h"

namespace Tellico {
  namespace Data {

/**
 * A wine collection.
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
class WineCollection : public Collection {
Q_OBJECT

public:
  /**
   * The constructor
   *
   * @param addFields A boolean indicating whether the default attributes should be added
   * @param title The title of the collection
   */
  WineCollection(bool addFields, const QString& title = QString());

  virtual Type type() const { return Wine; }

  static FieldList defaultFields();
};

  } // end namespace
} // end namespace
#endif
