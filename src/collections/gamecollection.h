/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef GAMECOLLECTION_H
#define GAMECOLLECTION_H

#include "../collection.h"

namespace Tellico {
  namespace Data {

/**
 * A collection for games.
 */
class GameCollection : public Collection {
Q_OBJECT

public:
  /**
   * The constructor
   *
   * @param title The title of the collection
   */
  GameCollection(bool addDefaultFields, const QString& title = QString());

  virtual Type type() const { return Game; }

  static FieldList defaultFields();
};

  } // end namespace
} // end namespace
#endif
