/***************************************************************************
                              musiccollection.h
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

#ifndef MUSICCOLLECTION_H
#define MUSICCOLLECTION_H

#include "../bccollection.h"

/**
 * A BCCollection for music, like CD's and cassettes.
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
 * @version $Id: musiccollection.h 200 2003-10-14 00:21:13Z robby $
 */
class MusicCollection : public BCCollection  {
Q_OBJECT

public:
  /**
   * The constructor
   *
   * @param addAttributes Whether to add the default attributes
   * @param title The title of the collection
   */
  MusicCollection(bool addAttributes, const QString& title = QString::null);

  virtual BCCollection::CollectionType collectionType() const { return BCCollection::Album; };

  static BCAttributeList defaultAttributes();
};

#endif
