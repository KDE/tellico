/***************************************************************************
                            bccollectionfactory.h
                             -------------------
    begin                : Fri Sep 12 2003
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

#ifndef BCCOLLECTIONFACTORY_H
#define BCCOLLECTIONFACTORY_H

#include "bccollection.h"

#include <qmap.h>

typedef QMap<BCCollection::CollectionType, QString> CollectionNameMap;

/**
 * A factor class for dealing with the different types of collections.
 *
 * @author Robby Stephenson
 * @version $Id: bccollectionpropdialog.h 112 2003-09-13 04:49:05Z robby $
 */
class BCCollectionFactory {
public: 
  static BCCollection* collection(BCCollection::CollectionType type, bool addAttributes);
  static BCCollection* collection(const QString& unitName, bool addAttributes);
  static BCAttributeList defaultAttributes(BCCollection::CollectionType type);
  static CollectionNameMap typeMap();
};

#endif
