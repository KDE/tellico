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

#ifndef COMICBOOKCOLLECTION_H
#define COMICBOOKCOLLECTION_H

#include "../collection.h"

namespace Bookcase {
  namespace Data {

/**
 * A collection for comic books.
 *
 * It has the following standard attributes:
 * @li Title
 *
 * @author Robby Stephenson
 * @version $Id: comicbookcollection.h 70 2003-07-31 03:49:14Z robby $
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

  static FieldList defaultFields();
};

  } // end namespace
} // end namespace
#endif
