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

#ifndef COMICBOOKCOLLECTION_H
#define COMICBOOKCOLLECTION_H

#include "../collection.h"

namespace Tellico {
  namespace Data {

/**
 * A collection for comic books.
 *
 * It has the following standard attributes:
 * @li Title
 *
 * @author Robby Stephenson
 */
class ComicBookCollection : public Collection {
Q_OBJECT

public:
  /**
   * The constructor
   *
   * @param title The title of the collection
   */
  ComicBookCollection(bool addFields, const QString& title = QString::null);

  virtual Type type() const { return ComicBook; }

  static FieldVec defaultFields();
};

  } // end namespace
} // end namespace
#endif
