/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include "modeliterator.h"
#include "models.h"
#include "../entrygroup.h"
#include "../entry.h"

#include <QAbstractItemModel>

using Tellico::ModelIterator;

ModelIterator::ModelIterator() : m_model(0), m_row(0) {
}

ModelIterator::ModelIterator(QAbstractItemModel* model_) : m_model(model_), m_row(0) {
}

bool ModelIterator::isValid() const {
  return m_model != 0 && m_row >= 0 && m_row < m_model->rowCount();
}

ModelIterator& ModelIterator::operator++() {
  ++m_row;
  return *this;
}

Tellico::Data::EntryGroup* ModelIterator::group() const {
  if(!isValid()) {
    return 0;
  }

  return m_model->data(m_model->index(m_row, 0), GroupPtrRole).value<Data::EntryGroup*>();
}

Tellico::Data::EntryPtr ModelIterator::entry() const {
  if(!isValid()) {
    return Data::EntryPtr();
  }

  return m_model->data(m_model->index(m_row, 0), EntryPtrRole).value<Data::EntryPtr>();
}
