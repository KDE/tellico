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

#include "objvalue.h"

namespace Tellico {
  QString objValue(const QJsonValue& v) {
    if(v.isNull())  {
      return QString();
    } else if(v.isString()) {
      return v.toString();
    } else if(v.isDouble()) {
      // leverage QVariant::toString and qNumberVariantHelper
      return QVariant(v.toDouble()).toString();
    } else if(v.isArray()) {
      const auto arr = v.toArray();
      QStringList l;
      l.reserve(arr.size());
      for(const auto& v2 : arr) {
        const QString s = objValue(v2);
        if(!s.isEmpty()) l += s;
      }
      return l.join(FieldFormat::delimiterString());
    }
    return QString();
  }
}
