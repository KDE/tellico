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

#ifndef VIDEOCOLLECTION_H
#define VIDEOCOLLECTION_H

#include "../collection.h"

namespace Bookcase {
  namespace Data {

/**
 * A collection for videos.
 *
 * It has the following standard attributes:
 * @li Title
 * @li Year
 * @li Genre
 * @li Medium
 * @li Comments
 *
 * @author Robby Stephenson
 * @version $Id: videocollection.h 652 2004-05-11 04:57:03Z robby $
 */
class VideoCollection : public Collection {
Q_OBJECT

public: 
  /**
   * The constructor
   *
   * @param addFields Whether to add the default attributes
   * @param title The title of the collection
   */
  VideoCollection(bool addFields, const QString& title = QString::null);

  virtual Type type() const { return Video; }

  static FieldList defaultFields();
};

  } // end namespace
} // end namespace
#endif
