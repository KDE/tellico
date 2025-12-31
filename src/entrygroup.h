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

#ifndef TELLICO_ENTRYGROUP_H
#define TELLICO_ENTRYGROUP_H

#include "datavectors.h"

namespace Tellico {

  namespace Data {

/**
 * The EntryGroup is simply a vector of entries which knows the name of its group,
 * and the name of the field to which that group belongs.
 *
 * An example for a book collection would be a group of books, all written by
 * David Weber. The @ref groupName() would be "Weber, David" and the
 * @ref fieldName() would be "author".
 *
 * @author Robby Stephenson
 */
class EntryGroup : public EntryList {

public:
  EntryGroup(const QString& group, const QString& field);
  ~EntryGroup();

  QString groupName() const;
  QString fieldName() const;

  bool hasEmptyGroupName() const;
  static QString emptyGroupName();

private:
  Q_DISABLE_COPY(EntryGroup)

  QString m_group;
  QString m_field;
};

  } // end namespace
} // end namespace

Q_DECLARE_METATYPE(Tellico::Data::EntryGroup*)

#endif
