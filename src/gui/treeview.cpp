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

#include "treeview.h"
#include "../models/abstractsortmodel.h"

#include <QHeaderView>

using Tellico::GUI::TreeView;

TreeView::TreeView(QWidget* parent_) : QTreeView(parent_) {
  setSortingEnabled(true);
}

TreeView::~TreeView() {
}

void TreeView::setModel(QAbstractItemModel* model_) {
  Q_ASSERT(::qobject_cast<AbstractSortModel*>(model_));
  QTreeView::setModel(model_);
}

Tellico::AbstractSortModel* TreeView::sortModel() const {
  return static_cast<AbstractSortModel*>(model());
}

bool TreeView::isEmpty() const {
  return model()->rowCount() == 0;
}

void TreeView::setSorting(Qt::SortOrder order_, int role_) {
  // no need to call sortByColumn() since that ends up sorting twice
  header()->setSortIndicator(0, order_);
  sortModel()->setSortRole(role_);
}

Qt::SortOrder TreeView::sortOrder() const {
  return header()->sortIndicatorOrder();
}

int TreeView::sortRole() const {
  return sortModel()->sortRole();
}

#include "treeview.moc"
