/***************************************************************************
                             videocollection.h
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

#ifndef VIDEOCOLLECTION_H
#define VIDEOCOLLECTION_H

#include <bccollection.h>

#include <klocale.h>

/**
 * A BCCollection for videos.
 *
 * It has the following standard attributes:
 * @li Title
 * @li Year
 * @li Genre
 * @li Medium
 * @li Comments
 *
 * @author Robby Stephenson
 * @version $Id: videocollection.h,v 1.2 2003/03/08 18:24:47 robby Exp $
 */
class VideoCollection : public BCCollection  {
Q_OBJECT

public: 
  /**
   * The constructor
   *
   * @param id The id of the collection, which should be unique.
   * @param title The title of the collection
   */
  VideoCollection(int id, const QString& title = i18n("My Videos"));

  virtual BCCollection::CollectionType collectionType() const { return BCCollection::Video; };
  virtual bool isVideo() const { return true; };
};

#endif
