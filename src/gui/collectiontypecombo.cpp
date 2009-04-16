/***************************************************************************
    copyright            : (C) 2006-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "collectiontypecombo.h"
#include "../collection.h"
#include "../collectionfactory.h"

using Tellico::GUI::CollectionTypeCombo;

CollectionTypeCombo::CollectionTypeCombo(QWidget* parent_) : ComboBox(parent_) {
  reset();
}

void CollectionTypeCombo::reset() {
  clear();
  // I want to sort the collection names, so use a map
  const CollectionNameHash nameMap = CollectionFactory::nameHash();
  QHash<QString, int> rNameMap;
  for(CollectionNameHash::ConstIterator it = nameMap.constBegin(); it != nameMap.constEnd(); ++it) {
    rNameMap.insert(it.value(), it.key());
  }
  const QList<int> collTypes = rNameMap.values();
  const QStringList collNames = rNameMap.keys();
  int custom = -1;
  const int total = collTypes.count();
  // when i equals the size, then go back and do custom
  for(int i = 0; i <= total; ++i) {
    // put custom last
    if(custom > -1 && count() >= total) {
      break; // already done it!
    } else if(i == total) {
      i = custom;
    } else if(collTypes[i] == Data::Collection::Base) {
      custom = i;
      continue;
    }
    addItem(collNames[i], collTypes[i]);
  }
}

void CollectionTypeCombo::setCurrentType(int type_) {
  setCurrentData(type_);
}
