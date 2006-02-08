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

#ifndef CARDCOLLECTION_H
#define CARDCOLLECTION_H

#include "../collection.h"

namespace Tellico {
  namespace Data {

/**
 * A collection for sports cards.
 *
 * It has the following standard attributes:
 * @li Title
 *
 * @author Robby Stephenson
 */
class CardCollection : public Collection {
Q_OBJECT

public:
  /**
   * The constructor
   *
   * @param addFields A boolean indicating whether the default attributes should be added
   * @param title The title of the collection
   */
  CardCollection(bool addFields, const QString& title = QString::null);

  virtual Type type() const { return Card; }

  static FieldVec defaultFields();
};

  } // end namespace
} // end namespace
#endif
