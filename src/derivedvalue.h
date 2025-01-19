/***************************************************************************
    Copyright (C) 2001-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_DATA_DERIVEDVALUE_H
#define TELLICO_DATA_DERIVEDVALUE_H

#include "datavectors.h"
#include "entry.h"

#include <QRegularExpression>

namespace Tellico {
  namespace Data {

class DerivedValue {
public:
  DerivedValue(const QString& valueTemplate);
  DerivedValue(FieldPtr field);

  // the reason we don't use a CollPtr is because this gets
  // called when adding fields in the Collection() constructor
  // which would create a ptr then destroy it and dereference the object
  bool isRecursive(Collection* coll) const;

  QString value(EntryPtr entry, bool formatted) const;

private:
  void initRegularExpression() const;
  QStringList templateFields() const;
  QString templateKeyValue(EntryPtr entry, QStringView key, bool formatted) const;

  QString m_fieldName;
  QString m_valueTemplate;
  mutable QRegularExpression m_keyRx;
  static const QRegularExpression s_templateFieldsRx;
};

  } // end namespace
} // end namespace

#endif
