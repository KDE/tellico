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

#include "groupsortmodel.h"
#include "models.h"
#include "stringcomparison.h"
#include "../field.h"
#include "../entrygroup.h"
#include "../document.h"
#include "../tellico_debug.h"

using Tellico::GroupSortModel;

GroupSortModel::GroupSortModel(QObject* parent) : AbstractSortModel(parent)
     , m_titleComparison(new TitleComparison())
     , m_groupComparison(nullptr) {
}

GroupSortModel::~GroupSortModel() {
  delete m_titleComparison;
  m_titleComparison = nullptr;
  delete m_groupComparison;
  m_groupComparison = nullptr;
}

void GroupSortModel::setSourceModel(QAbstractItemModel* sourceModel_) {
  AbstractSortModel::setSourceModel(sourceModel_);
  if(sourceModel_) {
    connect(sourceModel_, &QAbstractItemModel::modelReset,
            this, &GroupSortModel::clearGroupComparison);
  }
}

bool GroupSortModel::lessThan(const QModelIndex& left_, const QModelIndex& right_) const {
  // if the index have parents, then they represent entries, compare by title
  // calling index.parent() is expensive for the EntryGroupModel
  /// all we really need to know is whether the parent is valid
  const bool leftParentValid = sourceModel()->data(left_, ValidParentRole).toBool();
  const bool rightParentValid = sourceModel()->data(right_, ValidParentRole).toBool();
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

    bool reverseOrder = (sortOrder() == Qt::DescendingOrder);

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
      m_groupComparison = getComparison(leftGroup);
    }
    if(m_groupComparison) {
      return m_groupComparison->compare(leftGroup->groupName(), rightGroup->groupName()) < 0;
    }
    // couldn't determine the type or it's a type we want to sort
    // alphabetically, so sort by locale
    return left_.data().toString().localeAwareCompare(right_.data().toString()) < 0;
  }

  // for ordinary entries, just compare with title comparison
  return m_titleComparison->compare(left_.data().toString(), right_.data().toString()) < 0;
}

void GroupSortModel::clearGroupComparison() {
  delete m_groupComparison;
  m_groupComparison = nullptr;
}

// if 'group_' contains a type of field that merits a non-alphabetic
// sort, return a pointer to the proper sort function.
Tellico::StringComparison* GroupSortModel::getComparison(Data::EntryGroup* group_) const {
  StringComparison* comp = nullptr;
  if(group_) {
    Data::CollPtr coll = Data::Document::self()->collection();
    if(coll && coll->hasField(group_->fieldName())) {
      comp = StringComparison::create(coll->fieldByName(group_->fieldName()));
    }
  }
  return comp;
}
