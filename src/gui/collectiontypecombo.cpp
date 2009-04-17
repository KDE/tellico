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
  const CollectionNameHash nameHash = CollectionFactory::nameHash();
  // the values go into the combobox in alphabetical order by collection type name (which should be unique)
  // with the custom collection coming last
  // use a map to sort the values
  QMap<QString, int> nameMap;
  for(CollectionNameHash::ConstIterator it = nameHash.constBegin(); it != nameHash.constEnd(); ++it) {
    // skip the custom type, we add it later
    if(it.key() != Data::Collection::Base) {
      nameMap.insert(it.value(), it.key());
    }
  }
  for(QMap<QString, int>::ConstIterator it = nameMap.constBegin(); it != nameMap.constEnd(); ++it) {
    addItem(it.key(), it.value());
  }
  // now add the custom type last
  addItem(nameHash.value(Data::Collection::Base), Data::Collection::Base);
}

void CollectionTypeCombo::setCurrentType(int type_) {
  setCurrentData(type_);
}
