/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

using namespace Tellico;
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
    menu.addAction(KIcon(QLatin1String("view-filter")),
                    i18n("Modify Filter"), this, SLOT(slotModifyFilter()));
    menu.addAction(KIcon(QLatin1String("edit-delete")),
                    i18n("Delete Filter"), this, SLOT(slotDeleteFilter()));
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
  FilterPtr filter;
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
      if(filter) {
        myWarning() << "Only one filter can be applied";
      } else {
        filter = sourceModel()->filter(realIndex);
      }
    }
  }
  if(filter) {
    emit signalUpdateFilter(filter);
  }
  Controller::self()->slotUpdateSelection(this, entries.toList());
}

void FilterView::slotDoubleClicked(const QModelIndex& index_) {
  QModelIndex realIndex = sortModel()->mapToSource(index_);
  Data::EntryPtr entry = sourceModel()->entry(realIndex);
  if(entry) {
    Controller::self()->editEntry(entry);
  } else {
    FilterPtr filter = sourceModel()->filter(realIndex);
    if(filter) {
      Kernel::self()->modifyFilter(filter);
    }
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
    // two cases: if the filter used to match the entry and no longer does, then check the children indexes
    // if the filter matches now, check the actual match
    foreach(Data::EntryPtr entry, entries_) {
      if(indexContainsEntry(index, entry) || filter->matches(entry)) {
        sourceModel()->invalidate(index);
        break;
      }
    }
  }
}

bool FilterView::indexContainsEntry(const QModelIndex& parent_, Data::EntryPtr entry_) const {
  QModelIndex entryIndex = sourceModel()->index(0, 0, parent_);
  while(entryIndex.isValid()) {
    if(sourceModel()->entry(entryIndex) == entry_) {
      return true;
    }
    entryIndex = entryIndex.sibling(entryIndex.row()+1, 0);
  }
  return false;
}

#include "filterview.moc"
