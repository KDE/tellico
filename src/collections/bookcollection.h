/***************************************************************************
                              bookcollection.h
                             -------------------
    begin                : Tue Mar 4 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BOOKCOLLECTION_H
#define BOOKCOLLECTION_H

#include "../bccollection.h"

#include <klocale.h>

/**
 * A BCCollection for books.
 *
 * It has the following standard attributes:
 * @li Title
 * @li Subtitle
 * @li Author
 * @li Binding
 * @li Purchase Date
 * @li Purchase Price
 * @li Publisher
 * @li Edition
 * @li Copyright Year
 * @li Publication Year
 * @li ISBN Number
 * @li Library of Congress Catalog Number
 * @li Pages
 * @li Language
 * @li Genre
 * @li Keywords
 * @li Series
 * @li Series Number
 * @li Condition
 * @li Signed
 * @li Read
 * @li Gift
 * @li Loaned
 * @li Rating
 * @li Comments
 *
 * @author Robby Stephenson
 * @version $Id: bookcollection.h 200 2003-10-14 00:21:13Z robby $
 */
class BookCollection : public BCCollection  {
Q_OBJECT

public: 
  /**
   * The constructor
   *
   * @param addAttributes Whether to add the default attributes
   * @param title The title of the collection
   */
  BookCollection(bool addAttributes, const QString& title = QString::null);

  virtual BCCollection::CollectionType collectionType() const { return BCCollection::Book; };

  static BCAttributeList defaultAttributes();
};

#endif
