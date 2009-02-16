/***************************************************************************
    copyright            : (C) 2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "entrysortmodel.h"
#include "models.h"
#include "../listviewcomparison.h"
#include "../field.h"
#include "../entry.h"
#include "../tellico_debug.h"

using Tellico::EntrySortModel;

EntrySortModel::EntrySortModel(QObject* parent) : AbstractSortModel(parent) {
  setDynamicSortFilter(true);
  setSortLocaleAware(true);
}

EntrySortModel::~EntrySortModel() {
  clear();
}

void EntrySortModel::setFilter(Tellico::FilterPtr filter_) {
  if(m_filter != filter_) {
    m_filter = filter_;
    invalidateFilter();
  }
}

Tellico::FilterPtr EntrySortModel::filter() const {
  return m_filter;
}

void EntrySortModel::clear() {
  m_filter = FilterPtr();
  qDeleteAll(m_comparisons);
  m_comparisons.clear();
}

bool EntrySortModel::filterAcceptsRow(int row_, const QModelIndex& parent_) const {
  if(!m_filter) {
    return true;
  }
  QModelIndex index = sourceModel()->index(row_, 0, parent_);
  Data::EntryPtr entry = sourceModel()->data(index, EntryPtrRole).value<Data::EntryPtr>();
  return m_filter->matches(entry);
}

bool EntrySortModel::lessThan(const QModelIndex& left_, const QModelIndex& right_) const {
  if(sortRole() != EntryPtrRole) {
    return AbstractSortModel::lessThan(left_, right_);
  }

  QModelIndex left = left_;
  QModelIndex right = right_;

  for(int i = 0; i < 3; ++i) {
    ListViewComparison* comp = getComparison(left);
    Data::EntryPtr leftEntry = sourceModel()->data(left, EntryPtrRole).value<Data::EntryPtr>();
    Data::EntryPtr rightEntry = sourceModel()->data(right, EntryPtrRole).value<Data::EntryPtr>();
    /*
    if (comp && t) {
      QString l, r;
      int re = 0;
      if (leftEntry) {
        l = leftEntry->field(sourceModel()->data(left, FieldPtrRole).value<Data::FieldPtr>());
      }
      if (rightEntry) {
        r = rightEntry->field(sourceModel()->data(right, FieldPtrRole).value<Data::FieldPtr>());
      }
      if (leftEntry && rightEntry) {
        re = comp->compare(leftEntry, rightEntry);
      }
      myDebug() << i + 1 << ":" << l << (re < 0 ? "< " : ">=") << r;
    }
    */
    if(comp && leftEntry && rightEntry) {
      int res = comp->compare(leftEntry, rightEntry);
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
    } else {
      if(leftEntry.isNull() && !rightEntry.isNull()) {
        return true;
      } else {
        return false;
      }
    }
  }
  return AbstractSortModel::lessThan(left_, right_);
}

Tellico::ListViewComparison* EntrySortModel::getComparison(const QModelIndex& index_) const {
  if(m_comparisons.contains(index_.column())) {
    return m_comparisons.value(index_.column());
  }
  ListViewComparison* comp = 0;
  Data::FieldPtr field = sourceModel()->data(index_, FieldPtrRole).value<Data::FieldPtr>();
  if(field) {
    comp = ListViewComparison::create(field);
    m_comparisons.insert(index_.column(), comp);
  }
  return comp;
}

#include "entrysortmodel.moc"
