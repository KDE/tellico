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

#include "entrysortmodel.h"
#include "models.h"
#include "../utils/fieldcomparison.h"
#include "../field.h"
#include "../entry.h"

using Tellico::EntrySortModel;

EntrySortModel::EntrySortModel(QObject* parent) : AbstractSortModel(parent) {
  setDynamicSortFilter(true);
  setSortLocaleAware(true);
  connect(this, SIGNAL(modelReset()), SLOT(clearData()));
}

void EntrySortModel::setFilter(Tellico::FilterPtr filter_) {
  if(m_filter != filter_ || (m_filter && *m_filter != *filter_)) {
    m_filter = filter_;
    invalidateFilter();
  }
}

Tellico::FilterPtr EntrySortModel::filter() const {
  return m_filter;
}

bool EntrySortModel::filterAcceptsRow(int row_, const QModelIndex& parent_) const {
  if(!m_filter) {
    return true;
  }
  QModelIndex index = sourceModel()->index(row_, 0, parent_);
  Q_ASSERT(index.isValid());
  Data::EntryPtr entry = index.data(EntryPtrRole).value<Data::EntryPtr>();
  Q_ASSERT(entry);
  return m_filter->matches(entry);
}

bool EntrySortModel::lessThan(const QModelIndex& left_, const QModelIndex& right_) const {
  if(sortRole() != EntryPtrRole) {
    return AbstractSortModel::lessThan(left_, right_);
  }
  Data::EntryPtr leftEntry = left_.data(EntryPtrRole).value<Data::EntryPtr>();
  Data::EntryPtr rightEntry = right_.data(EntryPtrRole).value<Data::EntryPtr>();
  if(!leftEntry) {
    if(rightEntry) {
      return true;
    } else {
      return false;
    }
  } else if(leftEntry && !rightEntry) {
    return false;
  }

  QModelIndex left = left_;
  QModelIndex right = right_;

  for(int i = 0; i < 3; ++i) {
    FieldComparison* comp = getComparison(left);
    if(!comp || !left.isValid() || !right.isValid()) {
      return false;
    }

    const int res = comp->compare(leftEntry, rightEntry);
    if(res == 0) {
      switch (i) {
        case 0:
          left = left.model()->index(left.row(), secondarySortColumn());
          right = right.model()->index(right.row(), secondarySortColumn());
          break;
        case 1:
          left = left.model()->index(left.row(), tertiarySortColumn());
          right = right.model()->index(right.row(), tertiarySortColumn());
          break;
        case 2:
          return false;
      }
    } else {
      return res < 0;
    }
  }
  return AbstractSortModel::lessThan(left_, right_);
}

void EntrySortModel::clearData() {
  m_filter = FilterPtr();
  qDeleteAll(m_comparisons);
  m_comparisons.clear();
}

Tellico::FieldComparison* EntrySortModel::getComparison(const QModelIndex& index_) const {
  if(m_comparisons.contains(index_.column())) {
    return m_comparisons.value(index_.column());
  }
  FieldComparison* comp = 0;
  if(index_.isValid()) {
    Data::FieldPtr field = index_.model()->headerData(index_.column(), Qt::Horizontal, FieldPtrRole).value<Data::FieldPtr>();
    if(field) {
      comp = FieldComparison::create(field);
      m_comparisons.insert(index_.column(), comp);
    }
  }
  return comp;
}
