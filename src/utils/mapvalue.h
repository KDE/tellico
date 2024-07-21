/***************************************************************************
    Copyright (C) 2023 Robby Stephenson <robby@periapsis.org>
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

#ifndef MAPVALUE_H
#define MAPVALUE_H

#include "../fieldformat.h"

#include <QMetaType>
#include <QVariant>

namespace Tellico {
  // helper methods for the QVariantMaps used by the JSON importers
  template<typename T>
  QString mapValue(const QVariantMap& map, T name) {
    const QVariant v = map.value(QLatin1String(name));
    if(v.isNull())  {
      return QString();
    } else if(v.canConvert<QString>()) {
      return v.toString();
    } else if(v.canConvert<QStringList>()) {
      return v.toStringList().join(FieldFormat::delimiterString());
    } else {
      return QString();
    }
  }
  template<typename T, typename... Args>
  QString mapValue(const QVariantMap& map, T name, Args... args) {
    const QVariant v = map.value(QLatin1String(name));
    if(v.isNull())  {
      return QString();
    } else if(v.canConvert<QVariantMap>()) {
      return mapValue(v.toMap(), args...);
    } else if(v.canConvert<QVariantList>()) {
      QStringList values;
      const auto list = v.toList();
      for(const QVariant& v : list) {
        const QString s = mapValue(v.toMap(), args...);
        if(!s.isEmpty()) values += s;
      }
      return values.join(FieldFormat::delimiterString());
    } else {
      return QString();
    }
  }
}

#endif
