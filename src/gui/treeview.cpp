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

#include "treeview.h"
#include "../models/entrysortmodel.h"

#include <QHeaderView>

using Tellico::GUI::TreeView;

TreeView::TreeView(QWidget* parent_) : QTreeView(parent_) {
  setSortingEnabled(true);
}

TreeView::~TreeView() {
}

void TreeView::setModel(QAbstractItemModel* model_) {
  Q_ASSERT(::qobject_cast<EntrySortModel*>(model_));
  QTreeView::setModel(model_);
}

Tellico::EntrySortModel* TreeView::sortModel() const {
  return static_cast<EntrySortModel*>(model());
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