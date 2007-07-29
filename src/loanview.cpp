/***************************************************************************
    copyright            : (C) 2005-2007 by Robby Stephenson
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
#include "listviewcomparison.h"

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

  connect(this, SIGNAL(contextMenuRequested(QListViewItem*, const QPoint&, int)),
          SLOT(contextMenuRequested(QListViewItem*, const QPoint&, int)));

  connect(this, SIGNAL(expanded(QListViewItem*)),
          SLOT(slotExpanded(QListViewItem*)));

  connect(this, SIGNAL(collapsed(QListViewItem*)),
          SLOT(slotCollapsed(QListViewItem*)));
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
  if(item->isLoanItem()) {
    KPopupMenu menu(this);
    menu.insertItem(SmallIconSet(QString::fromLatin1("2downarrow")),
                    i18n("Check-in"), this, SLOT(slotCheckIn()));
    menu.insertItem(SmallIconSet(QString::fromLatin1("2downarrow")),
                    i18n("Modify Loan..."), this, SLOT(slotModifyLoan()));
    menu.exec(point_);
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

void LoanView::addCollection(Data::CollPtr coll_) {
  Data::BorrowerVec borrowers = coll_->borrowers();
  for(Data::BorrowerVec::Iterator it = borrowers.begin(); it != borrowers.end(); ++it) {
    addBorrower(it);
  }
  Data::FieldPtr f = coll_->fieldByName(QString::fromLatin1("title"));
  if(f) {
    setComparison(0, ListViewComparison::create(f));
  }
}

void LoanView::addField(Data::CollPtr, Data::FieldPtr) {
  resetComparisons();
}

void LoanView::modifyField(Data::CollPtr, Data::FieldPtr, Data::FieldPtr) {
  resetComparisons();
}

void LoanView::removeField(Data::CollPtr, Data::FieldPtr) {
  resetComparisons();
}

void LoanView::addBorrower(Data::BorrowerPtr borrower_) {
  if(!borrower_ || borrower_->isEmpty()) {
    return;
  }

  BorrowerItem* borrowerItem = new BorrowerItem(this, borrower_);
  borrowerItem->setExpandable(!borrower_->loans().isEmpty());
  m_itemDict.insert(borrower_->name(), borrowerItem);
}

void LoanView::modifyBorrower(Data::BorrowerPtr borrower_) {
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

  bool open = borrowerItem->isOpen();
  borrowerItem->setOpen(false);
  borrowerItem->setOpen(open);
}

void LoanView::slotCollapsed(QListViewItem* item_) {
  // only change icon for group items
  if(static_cast<GUI::ListViewItem*>(item_)->isBorrowerItem()) {
    static_cast<GUI::ListViewItem*>(item_)->clear();
  }
}

void LoanView::slotExpanded(QListViewItem* item_) {
  // only change icon for group items
  if(!static_cast<GUI::ListViewItem*>(item_)->isBorrowerItem()) {
    kdWarning() << "GroupView::slotExpanded() - non entry group item - " << item_->text(0) << endl;
    return;
  }

  setUpdatesEnabled(false);

  BorrowerItem* item = static_cast<BorrowerItem*>(item_);
  Data::LoanVec loans = item->borrower()->loans();
  for(Data::LoanVec::Iterator it = loans.begin(); it != loans.end(); ++it) {
    new LoanItem(item, it);
  }

  setUpdatesEnabled(true);
  triggerUpdate();
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
    Data::EntryPtr entry = static_cast<LoanItem*>(it.current())->entry();
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

void LoanView::resetComparisons() {
  // this is only allowed when the view is not empty, so we can grab a collection ptr
  if(childCount() == 0) {
    return;
  }
  Data::EntryVec entries = static_cast<BorrowerItem*>(firstChild())->entries();
  if(entries.isEmpty()) {
    return;
  }
  Data::EntryPtr entry = entries[0];
  if(!entry) {
    return;
  }
  Data::CollPtr coll = entry->collection();
  if(!coll) {
    return;
  }
  Data::FieldPtr f = coll->fieldByName(QString::fromLatin1("title"));
  if(f) {
    setComparison(0, ListViewComparison::create(f));
  }
}

#include "loanview.moc"
