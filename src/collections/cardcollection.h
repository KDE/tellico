/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
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

namespace Bookcase {
  namespace Data {

/**
 * A collection for sports cards.
 *
 * It has the following standard attributes:
 * @li Title
 *
 * @author Robby Stephenson
 * @version $Id: cardcollection.h 70 2003-07-31 03:49:14Z robby $
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

  virtual CollectionType collectionType() const { return Card; }

  static FieldList defaultFields();
};

  } // end namespace
} // end namespace
#endif
