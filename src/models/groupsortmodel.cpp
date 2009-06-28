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
#include "../utils/fieldcomparison.h"

using Tellico::GroupSortModel;

GroupSortModel::GroupSortModel(QObject* parent) : AbstractSortModel(parent), m_titleComparison(new TitleComparison()), m_groupComparison(0) {
}

GroupSortModel::~GroupSortModel() {
  delete m_titleComparison;
  m_titleComparison = 0;
  delete m_groupComparison;
  m_groupComparison = 0;
}

bool GroupSortModel::lessThan(const QModelIndex& left_, const QModelIndex& right_) const {
  if(sortRole() != GroupPtrRole) {
    return AbstractSortModel::lessThan(left_, right_);
  }

  // if the index have parents, then they represent entries, compare by title
  QModelIndex leftParent = left_.parent();
  QModelIndex rightParent = right_.parent();
  if(leftParent.isValid() && rightParent.isValid()) {
    return m_titleComparison->compare(left_.data().toString(), right_.data().toString()) < 0;
  }

  Data::EntryGroup* leftGroup = sourceModel()->data(left_, GroupPtrRole).value<Data::EntryGroup*>();
  Data::EntryGroup* rightGroup = sourceModel()->data(right_, GroupPtrRole).value<Data::EntryGroup*>();

  const bool emptyLeft = (!leftGroup || leftGroup->hasEmptyGroupName());
  const bool emptyRight = (!rightGroup || rightGroup->hasEmptyGroupName());
  if(emptyLeft && !emptyRight) {
    return true;
  } else if(!emptyLeft && emptyRight) {
    return false;
  } else if(emptyLeft && emptyRight) {
    return false;
  }

  return AbstractSortModel::lessThan(left_, right_);
}

#include "groupsortmodel.moc"
