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

#include "../bccollection.h"

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
 * @version $Id: videocollection.h 200 2003-10-14 00:21:13Z robby $
 */
class VideoCollection : public BCCollection  {
Q_OBJECT

public: 
  /**
   * The constructor
   *
   * @param addAttributes Whether to add the default attributes
   * @param title The title of the collection
   */
  VideoCollection(bool addAttributes, const QString& title = QString::null);

  virtual BCCollection::CollectionType collectionType() const { return BCCollection::Video; };

  static BCAttributeList defaultAttributes();
};

#endif
