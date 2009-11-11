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
#include "../field.h"
#include "../entrygroup.h"
#include "../utils/stringcomparison.h"
#include "../tellico_debug.h"

using Tellico::GroupSortModel;

GroupSortModel::GroupSortModel(QObject* parent) : AbstractSortModel(parent)
     , m_titleComparison(new TitleComparison()) {
}

GroupSortModel::~GroupSortModel() {
  delete m_titleComparison;
  m_titleComparison = 0;
}

bool GroupSortModel::lessThan(const QModelIndex& left_, const QModelIndex& right_) const {
  // if the index have parents, then they represent entries, compare by title
  QModelIndex leftParent = left_.parent();
  QModelIndex rightParent = right_.parent();
  if(!leftParent.isValid() || !rightParent.isValid()) {
    // we're dealing with groups
    Data::EntryGroup* leftGroup = sourceModel()->data(left_, GroupPtrRole).value<Data::EntryGroup*>();
    Data::EntryGroup* rightGroup = sourceModel()->data(right_, GroupPtrRole).value<Data::EntryGroup*>();

    // no matter what the sortRole is, the empty group is always first
    const bool emptyLeft = (!leftGroup || leftGroup->hasEmptyGroupName());
    const bool emptyRight = (!rightGroup || rightGroup->hasEmptyGroupName());

    bool reverseOrder = false;
    // sortOrder() was added in qt 4.5
#if (QT_VERSION >= QT_VERSION_CHECK(4, 5, 0))
    if (sortOrder() == Qt::DescendingOrder) reverseOrder = true;
#endif

    // yeah, I should figure out some bit-wise operations...whatever
    if(emptyLeft && !emptyRight) {
      return reverseOrder ? false : true;
    } else if(!emptyLeft && emptyRight) {
      return reverseOrder ? true: false;
    } else if(emptyLeft && emptyRight) {
      return reverseOrder ? true: false;
    }

    // now if we're using count, just pass up the line
    if(sortRole() == RowCountRole) {
      return AbstractSortModel::lessThan(left_, right_);
    }

    // for now, just sort case-insensitive, localeAware
    return left_.data().toString().toUpper().localeAwareCompare(right_.data().toString().toUpper()) < 0;
//  return AbstractSortModel::lessThan(left_, right_);
  }

  // for ordinary entries, just compare with title comparison
  return m_titleComparison->compare(left_.data().toString(), right_.data().toString()) < 0;
}

#include "groupsortmodel.moc"
