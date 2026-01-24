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
#include "../field.h"
#include "../entry.h"
#include "../tellico_debug.h"

using Tellico::EntrySortModel;

EntrySortModel::EntrySortModel(QObject* parent) : AbstractSortModel(parent) {
  setDynamicSortFilter(true);
  setSortLocaleAware(true);
  connect(this, &QAbstractItemModel::modelReset, this, &EntrySortModel::clearData);
}

void EntrySortModel::setFilter(Tellico::FilterPtr filter_) {
  if(m_filter != filter_ || (m_filter && *m_filter != *filter_)) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 9, 0))
    beginFilterChange();
#endif
    m_filter = filter_;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 10, 0))
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
    invalidateRowsFilter();
#endif
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
  if(!left_.isValid() || !right_.isValid()) {
    return false;
  }

  if(sortRole() != EntryPtrRole) {
    // for RowCount sorting, if there are no children, then sort by title
    if(sortRole() == RowCountRole) {
      const int leftCount = left_.data(RowCountRole).toInt();
      const int rightCount = right_.data(RowCountRole).toInt();
      if(leftCount == 0 && rightCount == 0) {
        // also, never sort descending by title when sorting parent by count
        const int res = left_.data().toString().localeAwareCompare(right_.data().toString());
        return sortOrder() == Qt::DescendingOrder ? (res >= 0) : (res < 0);
      }
    }
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
    const int res = doComparison(left, leftEntry, rightEntry);
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
  m_comparisons.clear();
}

int EntrySortModel::doComparison(const QModelIndex& index_, Data::EntryPtr entry1_, Data::EntryPtr entry2_) const {
  // the index is known to be valid
  auto it = m_comparisons.find(index_.column());
  if(it == m_comparisons.end()) {
    Data::FieldPtr field = index_.model()->headerData(index_.column(), Qt::Horizontal, FieldPtrRole).value<Data::FieldPtr>();
    if(field) {
      auto res = m_comparisons.insert({index_.column(), FieldComparison::create(field)});
      it = res.first;
    } else {
      myWarning() << "No valid field for comparison";
      return 0;
    }
  }
  return it->second->compare(entry1_, entry2_);
}
