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

#ifndef COINCOLLECTION_H
#define COINCOLLECTION_H

#include "../collection.h"

namespace Bookcase {
  namespace Data {

/**
 * A collection for coins.
 *
 * It has the following standard attributes:
 * @li Title
 *
 * @author Robby Stephenson
 * @version $Id: coincollection.h 70 2003-07-31 03:49:14Z robby $
 */
class CoinCollection : public Collection {
Q_OBJECT

public:
  /**
   * The constructor
   *
   * @param addFields Whether to add the default attributes
   * @param title The title of the collection
   */
  CoinCollection(bool addFields, const QString& title = QString::null);

  virtual CollectionType collectionType() const { return Coin; }

  static FieldList defaultFields();
};

  } // end namespace
} // end namespace
#endif
