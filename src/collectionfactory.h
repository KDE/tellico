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

#ifndef COLLECTIONFACTORY_H
#define COLLECTIONFACTORY_H

#include "collection.h"

#include <qmap.h>

namespace Bookcase {

typedef QMap<Data::Collection::Type, QString> CollectionNameMap;

/**
 * A factory class for dealing with the different types of collections.
 *
 * @author Robby Stephenson
 * @version $Id: collectionfactory.h 822 2004-08-27 23:27:34Z robby $
 */
class CollectionFactory {
public:
  static Data::Collection* collection(Data::Collection::Type type, bool addFields);
  static Data::Collection* collection(const QString& entryName, bool addFields);
  static Data::FieldList defaultFields(Data::Collection::Type type);
  static CollectionNameMap nameMap();
  static QString entryName(Data::Collection::Type type);
};

} // end namespace
#endif
