/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson, Steve Beattie
    email                : robby@periapsis.org, sbeattie@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BOARDGAMECOLLECTION_H
#define BOARDGAMECOLLECTION_H

#include "../collection.h"

namespace Tellico {
  namespace Data {

/**
 * A collection for board (not bored) games.
 */
class BoardGameCollection : public Collection {
Q_OBJECT

public:
  /**
   * The constructor
   *
   * @param title The title of the collection
   */
  BoardGameCollection(bool addDefaultFields, const QString& title = QString());

  virtual Type type() const { return BoardGame; }

  static FieldList defaultFields();
};

  } // end namespace
} // end namespace
#endif
