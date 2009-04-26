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

#ifndef COINCOLLECTION_H
#define COINCOLLECTION_H

#include "../collection.h"

namespace Tellico {
  namespace Data {

/**
 * A collection for coins.
 *
 * It has the following standard attributes:
 * @li Title
 *
 * @author Robby Stephenson
 */
class CoinCollection : public Collection {
Q_OBJECT

public:
  /**
   * The constructor
   *
   * @param title The title of the collection
   */
  CoinCollection(bool addDefaultFields, const QString& title = QString());

  virtual Type type() const { return Coin; }

  static FieldList defaultFields();
};

  } // end namespace
} // end namespace
#endif
