/***************************************************************************
                             songcollection.h
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

#ifndef SONGCOLLECTION_H
#define SONGCOLLECTION_H

#include <bccollection.h>

#include <klocale.h>

/**
 * A BCCollection for songs, like CD's and cassettes.
 *
 * It has the following standard attributes:
 * @li Title
 * @li Artist
 * @li Album
 * @li Year
 * @li Genre
 * @li Comments
 *
 * @author Robby Stephenson
 * @version $Id: songcollection.h,v 1.2 2003/04/01 03:30:41 robby Exp $
 */
class SongCollection : public BCCollection  {
Q_OBJECT

public:
  /**
   * The constructor
   *
   * @param id The id of the collection, which should be unique.
   * @param title The title of the collection
   */
  SongCollection(int id, const QString& title = i18n("My Songs"));

  virtual BCCollection::CollectionType collectionType() const { return BCCollection::Song; };
  virtual bool isSong() const { return true; };
};

#endif
