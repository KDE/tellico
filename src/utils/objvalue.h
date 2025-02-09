/***************************************************************************
    Copyright (C) 2025 Robby Stephenson <robby@periapsis.org>
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

#ifndef OBJVALUE_H
#define OBJVALUE_H

#include "../fieldformat.h"
#include "../tellico_debug.h"

#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>

namespace Tellico {
  QString objValue(const QJsonValue& v);

  template<typename T>
  QString objValue(const QJsonObject& obj, T name) {
    const auto v = obj.value(QLatin1StringView(name));
    if(v.isObject()) {
      myDebug() << "objValue for object" << name << "is undefined";
    }
    return objValue(v);
  }
  template<typename T, typename... Args>
  QString objValue(const QJsonObject& obj, T name, Args... args) {
    const auto v = obj.value(QLatin1StringView(name));
    if(v.isObject()) {
      return objValue(v.toObject(), args...);
    } else if(v.isArray()) {
      const auto arr = v.toArray();
      QStringList l;
      l.reserve(arr.size());
      for(const auto& v2 : arr) {
        QString s;
        if(v2.isObject()) s = objValue(v2.toObject(), args...);
        else s = objValue(v2);
        if(!s.isEmpty()) l += s;
      }
      return l.join(FieldFormat::delimiterString());
    } else {
      return objValue(v);
    }
  }
}

#endif
