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

#ifndef VIDEOCOLLECTION_H
#define VIDEOCOLLECTION_H

#include "../collection.h"

namespace Tellico {
  namespace Data {

/**
 * A collection for videos.
 *
 * @author Robby Stephenson
 */
class VideoCollection : public Collection {
Q_OBJECT

public:
  /**
   * The constructor
   *
   * @param title The title of the collection
   */
  VideoCollection(bool addDefaultFields, const QString& title = QString());

  virtual Type type() const { return Video; }
  virtual int sameEntry(Data::EntryPtr, Data::EntryPtr) const;

  static FieldList defaultFields();
};

  } // end namespace
} // end namespace
#endif
