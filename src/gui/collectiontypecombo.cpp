/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#include "collectiontypecombo.h"
#include "../collection.h"
#include "../collectionfactory.h"

#include <QIcon>

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
    if(it.key() != Data::Collection::Base &&
       (m_includedTypes.isEmpty() || m_includedTypes.contains(it.key()))) {
      nameMap.insert(it.value(), it.key());
    }
  }
  for(QMap<QString, int>::ConstIterator it = nameMap.constBegin(); it != nameMap.constEnd(); ++it) {
    addItem(it.key(), it.value());
  }
  // now add the custom type last
  if(m_includedTypes.isEmpty() || m_includedTypes.contains(Data::Collection::Base)) {
    ComboBox::addItem(QIcon::fromTheme(QStringLiteral("document-new")),
                      nameHash.value(Data::Collection::Base), Data::Collection::Base);
  }
}

void CollectionTypeCombo::setCurrentType(int type_) {
  setCurrentData(type_);
}

void CollectionTypeCombo::setIncludedTypes(const QList<int>& types_) {
  m_includedTypes = types_;
  reset();
}

void CollectionTypeCombo::addItem(const QString& value_, int collType_) {
  ComboBox::addItem(QIcon(QLatin1String(":/icons/") + CollectionFactory::typeName(collType_)), value_, collType_);
}
