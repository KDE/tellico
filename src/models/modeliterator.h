/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_MODELITERATOR_H
#define TELLICO_GROUPITERATOR_H

#include "../datavectors.h"

class QAbstractItemModel;

namespace Tellico {
  namespace Data {
    class EntryGroup;
  }

/**
 * @author Robby Stephenson
 */
class ModelIterator {
public:
  ModelIterator();
  ModelIterator(QAbstractItemModel* model);

  bool isValid() const;

  ModelIterator& operator++();

  Data::EntryGroup* group() const;
  Data::EntryPtr entry() const;

private:
  QAbstractItemModel* m_model;
  int m_row;
};

}

#endif
