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

#include "loanview.h"
#include "controller.h"
#include "borrower.h"
#include "entry.h"
#include "collection.h"
#include "tellico_kernel.h"
#include "tellico_debug.h"
#include "models/borrowermodel.h"
#include "models/entrysortmodel.h"
#include "models/models.h"
#include "gui/countdelegate.h"

#include <KLocalizedString>

#include <QMenu>
#include <QIcon>
#include <QHeaderView>
#include <QContextMenuEvent>

using Tellico::LoanView;

LoanView::LoanView(QWidget* parent_) : GUI::TreeView(parent_), m_notSortedYet(true) {
  header()->setSectionResizeMode(QHeaderView::Stretch);
  setHeaderHidden(false);
  setSelectionMode(QAbstractItemView::ExtendedSelection);

  connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
          SLOT(slotDoubleClicked(const QModelIndex&)));

  connect(header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)),
          SLOT(slotSortingChanged(int,Qt::SortOrder)));

  BorrowerModel* borrowerModel = new BorrowerModel(this);
  EntrySortModel* sortModel = new EntrySortModel(this);
  sortModel->setSourceModel(borrowerModel);
  setModel(sortModel);
  setItemDelegate(new GUI::CountDelegate(this));
  updateHeader();
}

/*
bool LoanView::isSelectable(GUI::ListViewItem* item_) const {
  if(!GUI::ListView::isSelectable(item_)) {
    return false;
  }

  // because the popup menu has modify, only
  // allow one loan item to get selected
  if(item_->isLoanItem()) {
    return selectedItems().isEmpty();
  }

  return true;
}
*/
Tellico::BorrowerModel* LoanView::sourceModel() const {
  return static_cast<BorrowerModel*>(sortModel()->sourceModel());
}

void LoanView::addCollection(Tellico::Data::CollPtr coll_) {
  m_coll = coll_;
  sourceModel()->clear();
  sourceModel()->addBorrowers(m_coll->borrowers());
}

void LoanView::slotReset() {
  sourceModel()->clear();
}

void LoanView::addBorrower(Tellico::Data::BorrowerPtr borrower_) {
  Q_ASSERT(borrower_);
  sourceModel()->addBorrower(borrower_);
}

void LoanView::modifyBorrower(Tellico::Data::BorrowerPtr borrower_) {
  Q_ASSERT(borrower_);
  sourceModel()->modifyBorrower(borrower_);
}

void LoanView::removeBorrower(Tellico::Data::BorrowerPtr borrower_) {
  Q_ASSERT(borrower_);
  sourceModel()->removeBorrower(borrower_);
}

void LoanView::contextMenuEvent(QContextMenuEvent* event_) {
  QModelIndex index = indexAt(event_->pos());
  if(!index.isValid()) {
    return;
  }

  // no parent means it's a top-level item
  if(index.parent().isValid()) {
    QMenu menu(this);
    menu.addAction(QIcon::fromTheme(QLatin1String("arrow-down-double")),
                   i18n("Check-in"), this, SLOT(slotCheckIn()));
    menu.addAction(QIcon::fromTheme(QLatin1String("arrow-down-double")),
                   i18n("Modify Loan..."), this, SLOT(slotModifyLoan()));
    menu.exec(event_->globalPos());
  }
}

void LoanView::slotDoubleClicked(const QModelIndex& index_) {
  QModelIndex realIndex = sortModel()->mapToSource(index_);
  Data::LoanPtr loan = sourceModel()->loan(realIndex);
  if(loan) {
    Kernel::self()->modifyLoan(loan);
  }
}

// this gets called when header() is clicked, so cycle through
void LoanView::slotSortingChanged(int col_, Qt::SortOrder order_) {
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

/*
BorrowerItem* item = static_cast<BorrowerItem*>(item_);
  Data::LoanList loans = item->borrower()->loans();
  foreach(Data::LoanPtr loan, loans) {
    new LoanItem(item, loan);
  }
*/

void LoanView::slotCheckIn() {
  QModelIndexList indexes = selectionModel()->selectedIndexes();
  if(indexes.isEmpty()) {
    return;
  }

  Data::EntryList entries;
  foreach(const QModelIndex& index, indexes) {
    // the indexes pointing to a borrower have no parent, skip them
    if(!index.parent().isValid()) {
      continue;
    }
    if(model()->hasChildren(index)) { //ignore items with children
      continue;
    }
    QModelIndex sourceIndex = sortModel()->mapToSource(index);
    Data::EntryPtr entry = sourceModel()->entry(sourceIndex);
    if(entry) {
      entries += entry;
    }
  }

  Controller::self()->slotCheckIn(entries);
  Controller::self()->slotClearSelection(); // so the checkout menu item gets disabled
}

void LoanView::slotModifyLoan() {
  QModelIndexList indexes = selectionModel()->selectedIndexes();
  if(indexes.isEmpty()) {
    return;
  }

  foreach(const QModelIndex& index, indexes) {
    QModelIndex sourceIndex = sortModel()->mapToSource(index);
    Data::LoanPtr loan = sourceModel()->loan(sourceIndex);
    if(loan) {
      Kernel::self()->modifyLoan(loan);
    }
  }
}

void LoanView::updateHeader() {
  if(sortModel()->sortRole() == Qt::DisplayRole) {
    model()->setHeaderData(0, Qt::Horizontal, i18n("Borrower"));
  } else {
    model()->setHeaderData(0, Qt::Horizontal, i18n("Borrower (Sort by Count)"));
  }
}
