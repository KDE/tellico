/***************************************************************************
    Copyright (C) 2008-2009 Robby Stephenson <robby@periapsis.org>
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

#include "abstractsortmodel.h"

using Tellico::AbstractSortModel;

AbstractSortModel::AbstractSortModel(QObject* parent) : QSortFilterProxyModel(parent)
    , m_sortColumn(-1), m_secondarySortColumn(-1), m_tertiarySortColumn(-1), m_sortOrder(Qt::AscendingOrder) {
}

AbstractSortModel::~AbstractSortModel() {
}

int AbstractSortModel::sortColumn() const {
  return m_sortColumn;
}

int AbstractSortModel::secondarySortColumn() const {
  return m_secondarySortColumn;
}

int AbstractSortModel::tertiarySortColumn() const {
  return m_tertiarySortColumn;
}

void AbstractSortModel::setSortColumn(int col) {
  m_sortColumn = col;
}

void AbstractSortModel::setSecondarySortColumn(int col) {
  m_secondarySortColumn = col;
}

void AbstractSortModel::setTertiarySortColumn(int col) {
  m_tertiarySortColumn = col;
}

Qt::SortOrder AbstractSortModel::sortOrder() const {
  return m_sortOrder;
}

void AbstractSortModel::setSortOrder(Qt::SortOrder order_) {
  if(order_ != m_sortOrder) {
    sort(m_sortColumn, order_);
  }
}

void AbstractSortModel::sort(int col_, Qt::SortOrder order_) {
  if(col_ != m_sortColumn) {
    m_tertiarySortColumn = m_secondarySortColumn;
    m_secondarySortColumn = m_sortColumn;
    m_sortColumn = col_;
  }
  m_sortOrder = order_;
  QSortFilterProxyModel::sort(col_, order_);
}

