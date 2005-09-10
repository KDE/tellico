/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
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

namespace Tellico {

typedef QMap<Data::Collection::Type, QString> CollectionNameMap;

/**
 * A factory class for dealing with the different types of collections.
 *
 * @author Robby Stephenson
 */
class CollectionFactory {
public:
  static Data::Collection* collection(Data::Collection::Type type, bool addFields,
                                      const QString& entryTitle=QString::null);
  static Data::Collection* collection(const QString& entryName, bool addFields);
  static Data::FieldVec defaultFields(Data::Collection::Type type);
  static CollectionNameMap nameMap();
  static QString entryName(Data::Collection::Type type);
};

} // end namespace
#endif
