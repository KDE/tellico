/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "filterview.h"
#include "controller.h"
#include "entry.h"
#include "collection.h"
#include "tellico_kernel.h"
#include "../tellico_debug.h"
#include "models/filtermodel.h"
#include "models/entrysortmodel.h"
#include "models/models.h"
#include "gui/countdelegate.h"

#include <klocale.h>
#include <kmenu.h>
#include <kicon.h>

#include <QHeaderView>
#include <QContextMenuEvent>

using Tellico::FilterView;

FilterView::FilterView(QWidget* parent_)
    : GUI::TreeView(parent_), m_notSortedYet(true) {
  header()->setResizeMode(QHeaderView::Stretch);
  setHeaderHidden(false);
  setSelectionMode(QAbstractItemView::ExtendedSelection);

  connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
          SLOT(slotDoubleClicked(const QModelIndex&)));

  connect(header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)),
          SLOT(slotSortingChanged(int,Qt::SortOrder)));

  FilterModel* filterModel = new FilterModel(this);
  EntrySortModel* sortModel = new EntrySortModel(this);
  sortModel->setSourceModel(filterModel);
  setModel(sortModel);
  setItemDelegate(new GUI::CountDelegate(this));
}

/*
bool FilterView::isSelectable(GUI::ListViewItem* item_) const {
  if(!GUI::ListView::isSelectable(item_)) {
    return false;
  }

  // because the popup menu has modify and delete, only
  // allow one filter item to get selected
  if(item_->isFilterItem()) {
    return selectedItems().isEmpty();
  }

  return true;
}
*/
Tellico::FilterModel* FilterView::sourceModel() const {
  return static_cast<FilterModel*>(sortModel()->sourceModel());
}

void FilterView::addCollection(Tellico::Data::CollPtr coll_) {
  m_coll = coll_;
  sourceModel()->clear();
  sourceModel()->addFilters(m_coll->filters());
}

void FilterView::addEntries(Tellico::Data::EntryList entries_) {
  invalidate(entries_);
}

void FilterView::modifyEntries(Tellico::Data::EntryList entries_) {
  invalidate(entries_);
}

void FilterView::removeEntries(Tellico::Data::EntryList entries_) {
  invalidate(entries_);
}

void FilterView::slotReset() {
  sourceModel()->clear();
}

void FilterView::addFilter(Tellico::FilterPtr filter_) {
  Q_ASSERT(filter_);
  sourceModel()->addFilter(filter_);
}

void FilterView::slotModifyFilter() {
  QModelIndex index = currentIndex();
  // parent means a top-level item
  if(!index.isValid() || index.parent().isValid()) {
    return;
  }

  QModelIndex realIndex = sortModel()->mapToSource(index);
  Kernel::self()->modifyFilter(sourceModel()->filter(realIndex));
}

void FilterView::slotDeleteFilter() {
  QModelIndex index = currentIndex();
  // parent means a top-level item
  if(!index.isValid() || index.parent().isValid()) {
    return;
  }

  QModelIndex realIndex = sortModel()->mapToSource(index);
  Kernel::self()->removeFilter(sourceModel()->filter(realIndex));
}

void FilterView::removeFilter(Tellico::FilterPtr filter_) {
  Q_ASSERT(filter_);
  sourceModel()->removeFilter(filter_);
}

void FilterView::contextMenuEvent(QContextMenuEvent* event_) {
  QModelIndex index = indexAt(event_->pos());
  if(!index.isValid()) {
    return;
  }

  KMenu menu(this);
  // no parent means it's a top-level item
  if(!index.parent().isValid()) {
    menu.addAction(KIcon(QString::fromLatin1("view-filter")),
                    i18n("Modify Filter"), this, SLOT(slotModifyFilter()));
    menu.addAction(KIcon(QString::fromLatin1("edit-delete")),
                    i18n("Delete Filter"), this, SLOT(slotDeleteFilter()));
    menu.exec(event_->globalPos());
  } else {
    Controller::self()->plugEntryActions(&menu);
  }
  menu.exec(event_->globalPos());
}

void FilterView::selectionChanged(const QItemSelection& selected_, const QItemSelection& deselected_) {
//  DEBUG_BLOCK;
  QAbstractItemView::selectionChanged(selected_, deselected_);
  // ignore the selected and deselected variables
  // we want to grab all the currently selected ones
  QSet<Data::EntryPtr> entries;
  foreach(const QModelIndex& index, selectionModel()->selectedIndexes()) {
    QModelIndex realIndex = sortModel()->mapToSource(index);
    Data::EntryPtr entry = sourceModel()->entry(realIndex);
    if(entry) {
      entries += entry;
    } else {
      QModelIndex child = realIndex.child(0, 0);
      for( ; child.isValid(); child = child.sibling(child.row()+1, 0)) {
        entry = sourceModel()->entry(child);
        if(entry) {
          entries += entry;
        }
      }
    }
  }
  Controller::self()->slotUpdateSelection(this, entries.toList());
}

void FilterView::slotDoubleClicked(const QModelIndex& index_) {
  QModelIndex realIndex = sortModel()->mapToSource(index_);
  Data::EntryPtr entry = sourceModel()->entry(realIndex);
  if(entry) {
    Controller::self()->editEntry(entry);
  }
}

// this gets called when header() is clicked, so cycle through
void FilterView::slotSortingChanged(int col_, Qt::SortOrder order_) {
  Q_UNUSED(col_);
  if(order_ == Qt::AscendingOrder && !m_notSortedYet) { // cycle through after ascending
    if(sortModel()->sortRole() == RowCountRole) {
      sortModel()->setSortRole(Qt::DisplayRole);
    } else {
      sortModel()->setSortRole(RowCountRole);
    }
  }

  updateHeader();
  m_notSortedYet = false;
}

void FilterView::updateHeader() {
  if(sortModel()->sortRole() == Qt::DisplayRole) {
    model()->setHeaderData(0, Qt::Horizontal, i18n("Filter"));
  } else {
    model()->setHeaderData(0, Qt::Horizontal, i18n("Filter (Sort by Count)"));
  }
}

void FilterView::invalidate(Tellico::Data::EntryList entries_) {
  const int rows = model()->rowCount();
  for(int row = 0; row < rows; ++row) {
    QModelIndex index = sourceModel()->index(row, 0);
    FilterPtr filter = sourceModel()->filter(index);
    if(!filter) {
      continue;
    }
    foreach(Data::EntryPtr entry, entries_) {
      if(filter->matches(entry)) {
        sourceModel()->invalidate(index);
        break;
      }
    }
  }
}

#include "filterview.moc"
