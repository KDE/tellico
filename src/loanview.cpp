/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "loanview.h"
#include "loanitem.h"
#include "controller.h"
#include "borrower.h"
#include "entry.h"
#include "collection.h"
#include "tellico_kernel.h"
#include "tellico_debug.h"

#include <klocale.h>
#include <kpopupmenu.h>
#include <kiconloader.h>

#include <qheader.h>

using Tellico::LoanView;

LoanView::LoanView(QWidget* parent_, const char* name_) : GUI::ListView(parent_, name_), m_notSortedYet(true) {
  addColumn(i18n("Borrower"));
  header()->setStretchEnabled(true, 0);
  setResizeMode(QListView::NoColumn);
  setRootIsDecorated(true);
  setShowSortIndicator(true);
  setTreeStepSize(15);
  setFullWidth(true);

  m_loanMenu = new KPopupMenu(this);
//  Controller::self()->plugEntryActions(m_entryMenu); // this includes a lend action, though...
  m_loanMenu->insertItem(SmallIconSet(QString::fromLatin1("2downarrow")),
                          i18n("Check-in"), this, SLOT(slotCheckIn()));
  m_loanMenu->insertItem(SmallIconSet(QString::fromLatin1("2downarrow")),
                          i18n("Modify Loan..."), this, SLOT(slotModifyLoan()));

  connect(this, SIGNAL(contextMenuRequested(QListViewItem*, const QPoint&, int)),
          SLOT(contextMenuRequested(QListViewItem*, const QPoint&, int)));
}

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

void LoanView::contextMenuRequested(QListViewItem* item_, const QPoint& point_, int) {
  if(!item_) {
    return;
  }

  GUI::ListViewItem* item = static_cast<GUI::ListViewItem*>(item_);
  if(item->isLoanItem() && m_loanMenu->count() > 0) {
    m_loanMenu->popup(point_);
  }
}

// this gets called when header() is clicked, so cycle through
void LoanView::setSorting(int col_, bool asc_) {
  if(asc_ && !m_notSortedYet) {
    if(sortStyle() == ListView::SortByText) {
      setSortStyle(ListView::SortByCount);
    } else {
      setSortStyle(ListView::SortByText);
    }
  }
  if(sortStyle() == ListView::SortByText) {
    setColumnText(0, i18n("Borrower"));
  } else {
    setColumnText(0, i18n("Borrower (Sort by Count)"));
  }
  m_notSortedYet = false;
  ListView::setSorting(col_, asc_);
}

void LoanView::addCollection(Data::Collection* coll_) {
  Data::BorrowerVec borrowers = coll_->borrowers();
  for(Data::BorrowerVec::Iterator it = borrowers.begin(); it != borrowers.end(); ++it) {
    addBorrower(it);
  }
}

void LoanView::addBorrower(Data::Borrower* borrower_) {
  if(!borrower_ || borrower_->isEmpty()) {
    return;
  }

  BorrowerItem* borrowerItem = new BorrowerItem(this, borrower_);
  m_itemDict.insert(borrower_->name(), borrowerItem);

  Data::LoanVec loans = borrower_->loans();
  for(Data::LoanVec::Iterator it = loans.begin(); it != loans.end(); ++it) {
    new LoanItem(borrowerItem, it);
  }
}

void LoanView::modifyBorrower(Data::Borrower* borrower_) {
  if(!borrower_) {
    return;
  }

  BorrowerItem* borrowerItem = m_itemDict[borrower_->name()];
  if(!borrowerItem) {
    myDebug() << "LoanView::modifyBorrower() - borrower was never added" << endl;
    return;
  }

  if(borrower_->isEmpty()) {
    m_itemDict.remove(borrower_->name());
    delete borrowerItem;
    return;
  }

  borrowerItem->clear(); // remove all children;

  Data::LoanVec loans = borrower_->loans();
  for(Data::LoanVec::Iterator it = loans.begin(); it != loans.end(); ++it) {
    new LoanItem(borrowerItem, it);
  }
}

void LoanView::slotCheckIn() {
  GUI::ListViewItem* item = selectedItems().getFirst();
  if(!item || !item->isLoanItem()) {
    return;
  }

  Data::EntryVec entries;
  // need a copy since we may be deleting
  GUI::ListViewItemList list = selectedItems();
  for(GUI::ListViewItemListIt it(list); it.current(); ++it) {
    Data::Entry* entry = static_cast<LoanItem*>(it.current())->entry();
    if(!entry) {
      myDebug() << "LoanView::slotCheckIn() - no entry!" << endl;
      continue;
    }
    entries.append(entry);
  }

  Controller::self()->slotCheckIn(entries);
  Controller::self()->slotClearSelection(); // so the checkout menu item gets disabled
}

void LoanView::slotModifyLoan() {
  GUI::ListViewItem* item = selectedItems().getFirst();
  if(!item || !item->isLoanItem()) {
    return;
  }

  Kernel::self()->modifyLoan(static_cast<LoanItem*>(item)->loan());
}

#include "loanview.moc"
