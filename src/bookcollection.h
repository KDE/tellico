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

#include <bccollection.h>

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
 * @version $Id: bookcollection.h,v 1.2 2003/04/01 03:30:41 robby Exp $
 */
class BookCollection : public BCCollection  {
Q_OBJECT

public: 
  /**
   * The constructor
   *
   * @param id The id of the collection, which should be unique.
   * @param title The title of the collection
   */
  BookCollection(int id, const QString& title = i18n("My Books"));

  virtual BCCollection::CollectionType collectionType() const { return BCCollection::Book; };
  virtual bool isBook() const { return true; };

  static QStringList defaultViewAttributes();
  static QStringList defaultPrintAttributes();
  static QString defaultTitle();
  static QString defaultUnitTitle();
};

#endif
