/***************************************************************************
    Copyright (C) 2021 Robby Stephenson <robby@periapsis.org>
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

#ifndef MERGECONFLICTRESOLVER_H
#define MERGECONFLICTRESOLVER_H

#include "../datavectors.h"

/**
 * This file contains merging functions.
 *
 * @author Robby Stephenson
 */
namespace Tellico {
  namespace Merge {
    class ConflictResolver {
    public:
      enum Result { KeepFirst, KeepSecond, CancelMerge };

      ConflictResolver() {}
      virtual ~ConflictResolver() {}
      virtual Result resolve(Data::EntryPtr entry1, Data::EntryPtr entry2, Data::FieldPtr field,
                            const QString& value1 = QString(), const QString& value2 = QString()) = 0;

    private:
      Q_DISABLE_COPY(ConflictResolver)
    };

    bool mergeEntry(Data::EntryPtr entry1, Data::EntryPtr entry2, ConflictResolver* resolver=nullptr);
    // adds new fields into collection if any values in entries are not empty
    // first object is modified fields, second is new fields
    QPair<Data::FieldList, Data::FieldList> mergeFields(Data::CollPtr coll,
                                                        Data::FieldList fields,
                                                        Data::EntryList entries);
  }
}

#endif
