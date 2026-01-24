/***************************************************************************
    Copyright (C) 2008-2020 Robby Stephenson <robby@periapsis.org>
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

#include "groupsortmodel.h"
#include "models.h"
#include "stringcomparison.h"
#include "fieldcomparison.h"
#include "../field.h"
#include "../entrygroup.h"
#include "../tellico_debug.h"

using namespace Tellico;
using Tellico::GroupSortModel;

GroupSortModel::GroupSortModel(QObject* parent) : AbstractSortModel(parent) {
}

GroupSortModel::~GroupSortModel() {
  clearComparisons();
}

void GroupSortModel::setSourceModel(QAbstractItemModel* sourceModel_) {
  AbstractSortModel::setSourceModel(sourceModel_);
  if(sourceModel_) {
    clearComparisons();
    connect(sourceModel_, &QAbstractItemModel::modelReset,
            this, &GroupSortModel::clearComparisons);
  }
}

QString GroupSortModel::entrySortField() const {
  return m_entryComparison ? m_entryComparison->field()->name() : QString();
}

void GroupSortModel::setEntrySortField(const QString& fieldName_) {
  // only have to update if the field name is different than existing
  if(m_entryComparison && m_entryComparison->field() &&
     m_entryComparison->field()->name() == fieldName_) {
    return;
  }
  // can only update the sort field if the model is not empty
  QModelIndex groupIndex = index(0, 0);
  if(!groupIndex.isValid()) {
    return;
  }
  QModelIndex entryIndex = index(0, 0, groupIndex);
  if(!entryIndex.isValid()) {
    return;
  }
  // possible that field name does not exist in this collection
  auto newComp = getEntryComparison(entryIndex, fieldName_);
  if(!newComp) {
    return;
  }
  Q_EMIT layoutAboutToBeChanged(QList<QPersistentModelIndex>(), QAbstractItemModel::VerticalSortHint);
  std::swap(m_entryComparison, newComp);
  Q_EMIT layoutChanged(QList<QPersistentModelIndex>(), QAbstractItemModel::VerticalSortHint);
  // emitting layoutChanged does not cause the sorting to be refreshed. I can't figure out why
  // but calling invalidate() does. <shrug>
  invalidate();
}

bool GroupSortModel::lessThan(const QModelIndex& left_, const QModelIndex& right_) const {
  // if the index have parents, then they represent entries, compare by title
  // calling index.parent() is expensive for the EntryGroupModel
  // all we really need to know is whether the parent is valid
  const bool leftParentValid = sourceModel()->data(left_, ValidParentRole).toBool();
  const bool rightParentValid = sourceModel()->data(right_, ValidParentRole).toBool();
  const bool reverseOrder = (sortOrder() == Qt::DescendingOrder);
  if(!leftParentValid || !rightParentValid) {
    // now if we're using count, just pass up the line
    if(sortRole() == RowCountRole) {
      return AbstractSortModel::lessThan(left_, right_);
    }

    // we're dealing with groups
    Data::EntryGroup* leftGroup = sourceModel()->data(left_, GroupPtrRole).value<Data::EntryGroup*>();
    Data::EntryGroup* rightGroup = sourceModel()->data(right_, GroupPtrRole).value<Data::EntryGroup*>();

    // no matter what the sortRole is, the empty group is always first
    const bool emptyLeft = (!leftGroup || leftGroup->hasEmptyGroupName());
    const bool emptyRight = (!rightGroup || rightGroup->hasEmptyGroupName());

    // yeah, I should figure out some bit-wise operations...whatever
    if(emptyLeft && !emptyRight) {
      return reverseOrder ? false : true;
    } else if(!emptyLeft && emptyRight) {
      return reverseOrder ? true : false;
    } else if(emptyLeft && emptyRight) {
      return reverseOrder ? true : false;
    }

    // if we can get the fields' type, then for certain non-text-only
    // types use the sort defined for that type.
    if(!m_groupComparison) {
      if(leftGroup && !leftGroup->isEmpty()) {
        // depend on the group NOT being empty which allows access to the first entry
        Data::CollPtr coll = leftGroup->first()->collection();
        if(coll && coll->hasField(leftGroup->fieldName())) {
          auto f = coll->fieldByName(leftGroup->fieldName());
          m_groupComparison = StringComparison::create(f);
        }
      }
    }
    if(m_groupComparison) {
      return m_groupComparison->compare(leftGroup->groupName(), rightGroup->groupName()) < 0;
    }
    // couldn't determine the type or it's a type we want to sort
    // alphabetically, so sort by locale
    return left_.data().toString().localeAwareCompare(right_.data().toString()) < 0;
  }

  if(!m_entryComparison) {
    // default to using the title field for sorting by calling without a field name
    m_entryComparison = getEntryComparison(left_, QString());
  }
  if(!m_entryComparison) {
    return left_.data().toString().localeAwareCompare(right_.data().toString()) < 0;
  }
  // entries always sort ascending, despite whatever the group order is
  const bool res =  m_entryComparison->compare( left_.data(EntryPtrRole).value<Data::EntryPtr>(),
                                                right_.data(EntryPtrRole).value<Data::EntryPtr>()) < 0;
  return reverseOrder ? !res : res;
}

void GroupSortModel::clearComparisons() {
  m_entryComparison.reset();
  m_groupComparison.reset();
}

std::unique_ptr<Tellico::FieldComparison> GroupSortModel::getEntryComparison(const QModelIndex& index_, const QString& fieldName_) const {
  // depend on the index pointing to an entry from which we can get a collection
  Data::EntryPtr entry = index_.data(EntryPtrRole).value<Data::EntryPtr>();
  if(entry) {
    Data::CollPtr coll = entry->collection();
    if(coll) {
      // by default, always sort by title
      const auto fieldName = fieldName_.isEmpty() ? coll->titleField() : fieldName_;
      return FieldComparison::create(coll->fieldByName(fieldName));
    }
  }
  return nullptr;
}
