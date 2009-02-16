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

#ifndef STAMPCOLLECTION_H
#define STAMPCOLLECTION_H

#include "../collection.h"

namespace Tellico {
  namespace Data {

/**
 * A stamp collection.
 *
 * It has the following standard attributes:
 * @li Title
 * @li Description
 * @li Denomination
 * @li Country
 * @li Year
 * @li Color
 * @li Scott
 * @li Grade
 * @li Cancelled
 * @li Hinged
 * @li Centering
 * @li Gummed
 * @li Pur_date
 * @li Pur_price
 * @li Location
 * @li Gift
 * @li Image
 * @li Comments
 *
 * @author Robby Stephenson
 */
class StampCollection : public Collection {
Q_OBJECT

public:
  /**
   * The constructor
   *
   * @param addFields A boolean indicating whether the default attributes should be added
   * @param title The title of the collection
   */
  StampCollection(bool addFields, const QString& title = QString::null);

  virtual Type type() const { return Stamp; }

  static FieldList defaultFields();
};

  } // end namespace
} // end namespace
#endif
