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

#ifndef COLLECTIONFACTORY_H
#define COLLECTIONFACTORY_H

#include "datavectors.h"

#include <qmap.h>

namespace Tellico {

typedef QMap<int, QString> CollectionNameMap;

/**
 * A factory class for dealing with the different types of collections.
 *
 * @author Robby Stephenson
 */
class CollectionFactory {
public:
  static Data::CollPtr collection(int type, bool addFields);
  static Data::CollPtr collection(const QString& typeName, bool addFields);
  static CollectionNameMap nameMap();
  static QString typeName(int type);
  static bool isDefaultField(int type, const QString& name);
};

} // end namespace
#endif
