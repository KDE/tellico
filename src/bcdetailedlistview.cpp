/***************************************************************************
                           bcdedtailedlistview.cpp
                             -------------------
    begin                : Tue Sep 4 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bcdetailedlistview.h"
#include "bcunit.h"
#include "bcunititem.h"
#include "bccollection.h"
#include "bookcasedoc.h"

#include <klocale.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kdebug.h>

#include <qptrlist.h>
#include <qstringlist.h>
#include <qvaluelist.h>
#include <qheader.h>

BCDetailedListView::BCDetailedListView(BookcaseDoc* doc_, QWidget* parent_, const char* name_/*=0*/)
    : KListView(parent_, name_), m_doc(doc_) {
//  kdDebug() << "BCDetailedListView()" << endl;
  setAllColumnsShowFocus(true);
  setShowSortIndicator(true);

  connect(this, SIGNAL(selectionChanged(QListViewItem*)),
          SLOT(slotSelected(QListViewItem*)));
// if a list view item is clicked...something is modified, and then the user
// clicks on it again, no signal is sent because the selection didn't change...so the
// next connection must be made as well. The side effect is that two signals are sent when
// the user clicks on a different list view item
  connect(this, SIGNAL(clicked(QListViewItem*)),
          SLOT(slotSelected(QListViewItem*)));

  connect(this, SIGNAL(rightButtonPressed(QListViewItem*, const QPoint&, int)),
          SLOT(slotRMB(QListViewItem*, const QPoint&, int)));

  // header menu
  header()->setClickEnabled(true);
  header()->installEventFilter(this);
  m_headerMenu = new KPopupMenu(this);
  m_headerMenu->insertTitle(i18n("View Columns"));
  m_headerMenu->setCheckable(true);
  connect(m_headerMenu, SIGNAL(activated(int)),
          this, SLOT(slotHeaderMenuActivated(int)));
  
  // item menu
  QPixmap remove = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("remove"), KIcon::Small);
  m_itemMenu = new KPopupMenu(this);
  m_itemMenu->insertItem(remove, i18n("Delete Book"), this, SLOT(slotHandleDelete()));

  m_bookPix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("book"), KIcon::User);
  m_checkPix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("endturn"), KIcon::Small);

  // need this, so that sorting is allowed
  setSorting(0, true);
}

// without the destructor, it crashes, for some reason
BCDetailedListView::~BCDetailedListView() {
}

const QStringList& BCDetailedListView::columnNames() const {
  return m_colNames;
}

void BCDetailedListView::slotAddCollection(BCCollection* coll_) {
  if(!coll_) {
    return;
  }

//  kdDebug() << "BCDetailedListView::slotAddCollection()"<< endl;
  
  QPtrListIterator<BCUnit> unitIt(coll_->unitList());
  for( ; unitIt.current(); ++unitIt) {
    slotAddItem(unitIt.current());
  }
}

void BCDetailedListView::slotReset() {
  kdDebug() << "BCDetailedListView::slotReset()" << endl;
  //clear() does not remove columns
  clear();
//  while(columns() > 0) {
//    removeColumn(0);
//  }
}

void BCDetailedListView::slotAddItem(BCUnit* unit_) {
  if(!unit_) {
    kdWarning() << "BCDetailedListView::slotAddItem() - null unit pointer!" << endl;
    return;
  }

//  kdDebug() << "BCDetailedListView::slotAddItem() - " << unit_->attribute("title") << endl;

  BCUnitItem* item = new BCUnitItem(this, unit_);

  if(unit_->collection()->isBook()) {
    item->setPixmap(0, m_bookPix);
  } else {
    kdDebug() << "BCDetailedListView::slotAddItem() - no pixmap for this collection type!" << endl;
  }

  populateItem(item);

  if(isUpdatesEnabled()) {
    sort();
    ensureItemVisible(item);
    setSelected(item, true);
  }
}

void BCDetailedListView::slotModifyItem(BCUnit* unit_) {
  if(!unit_) {
    kdWarning() << "BCDetailedListView::slotModifyItem() - null unit pointer!" << endl;
    return;
  }

//  kdDebug() << "BCDetailedListView::slotModifyItem() - " << unit_->title() << endl;
  BCUnitItem* item = locateItem(unit_);
  populateItem(item);
  sort();
  ensureItemVisible(item);
}

void BCDetailedListView::slotRemoveItem(BCUnit* unit_) {
  if(!unit_) {
    kdWarning() << "BCDetailedListView::slotRemoveItem() - null unit pointer!" << endl;
    return;
  }

  kdDebug() << "BCDetailedListView::slotRemoveItem() - " << unit_->title() << endl;

  delete locateItem(unit_);
}

// TODO: more efficient way to do this?
void BCDetailedListView::slotRemoveItem(BCCollection* coll_) {
  if(!coll_) {
    kdWarning() << "BCDetailedListView::slotRemoveItem() - null coll pointer!" << endl;
    return;
  }

//  kdDebug() << "BCDetailedListView::slotRemoveItem() - " << coll_->title() << endl;

  QListViewItem* item = firstChild();
  QListViewItem* next;
  while(item) {
    BCUnit* unit = static_cast<BCUnitItem*>(item)->unit();
    next = item->nextSibling();
    if(unit->collection() == coll_) {
      delete item;
    }
    item = next;
  }
}

void BCDetailedListView::slotHandleDelete() {
  BCUnitItem* item = static_cast<BCUnitItem*>(currentItem());
  if(!item || !item->unit()) {
    return;
  }

  emit signalDeleteUnit(item->unit());
}

void BCDetailedListView::populateItem(BCUnitItem* item_) {
  BCUnit* unit = item_->unit();
  if(!unit) {
    return;
  }

  unsigned colNum = 0;
  QStringList::ConstIterator it = m_colNames.begin();
  for( ; it != m_colNames.end(); ++it) {
    BCAttribute* att = unit->collection()->attributeByName(*it);

    if(att->type() == BCAttribute::Bool) {
      // for bools, the value is "1", just check to see if empty
      if(unit->attribute(att->name()).isEmpty()) {
        item_->setPixmap(colNum, QPixmap());
        item_->setText(colNum, QString());
      } else {
        item_->setPixmap(colNum, m_checkPix);
        // needed for sorting
        item_->setText(colNum, QString::fromLatin1(" "));
      }
    } else {
      item_->setText(colNum, unit->attributeFormatted(att->name(), att->flags()));
    }
    
    if(columnText(colNum) != att->title()) {
      kdDebug() << "BCDetailedListView::populateItem() - Column header does not match attribute name." << endl;
      kdDebug() << "colText = " << columnText(colNum) << ";att->title() = " << att->title() <<  endl;
    }

    colNum++;
  } // end attribute loop
}

void BCDetailedListView::slotRMB(QListViewItem* item_, const QPoint& point_, int) {
  if(item_ && m_itemMenu->count() > 0) {
    setSelected(item_, true);
    m_itemMenu->popup(point_);
  }
}

void BCDetailedListView::slotSelected(QListViewItem* item_) {
  if(!item_ && !selectedItem()) {
    emit signalClear();
    return;
  }

  // there may still be a null pointer, so set it to the selected item
  // TODO: somewhat inefficient since the selection probably did not change
  if(!item_) {
    item_ = selectedItem();
  }

  // all items in the listview are unitItems
  BCUnitItem* item = static_cast<BCUnitItem*>(item_);
  if(item->unit()) {
    emit signalUnitSelected(item->unit());
  }
}

void BCDetailedListView::slotSetSelected(BCUnit* unit_) {
//  kdDebug() << "BCDetailedListView::slotSetSelected()" << endl;
  // if unit_ is null pointer, set no selected
  if(!unit_) {
    setSelected(currentItem(), false);
    return;
  }

  BCUnitItem* item = locateItem(unit_);

  blockSignals(true);
  setSelected(item, true);
  blockSignals(false);
  ensureItemVisible(item);
}

BCUnitItem* const BCDetailedListView::locateItem(BCUnit* unit_) {
  QListViewItemIterator it(this);
  for( ; it.current(); ++it) {
    BCUnitItem* item = static_cast<BCUnitItem*>(it.current());
    if(item->unit() == unit_) {
      return item;
    }
  }

  return 0;
}

void BCDetailedListView::slotClearSelection() {
  selectAll(false);
}

void BCDetailedListView::setColumns(BCCollection* coll_, const QStringList& colNames_) {
//  kdDebug() << "BCDetailedListView::setColumns() " << endl;
  // if the header menu is empty, populate it
  // it has a title entry, so an empty menu has one item
  if(m_headerMenu->count() < 2) {
    BCCollection::CollectionType type = coll_->collectionType();
    BCAttributeList list = m_doc->uniqueAttributes(type);
    BCAttributeListIterator it(list);
    for( ; it.current(); ++it) {
      int itemNum = m_headerMenu->insertItem(it.current()->title());
      m_columnMap.insert(itemNum, it.current()->name());

      if(colNames_.contains(it.current()->name()) > 0) {
        int i = addColumn(it.current()->title());
        setColumnWidthMode(i, QListView::Manual);
        if(it.current()->type() == BCAttribute::Bool) {
          setColumnAlignment(i, Qt::AlignHCenter);
        }
        m_headerMenu->setItemChecked(itemNum, true);
      }
    }
  }

  if(!coll_ || colNames_ == m_colNames) {
    return;
  }
  
  m_colNames = colNames_;
  
  int colNum = 0;
  QStringList::ConstIterator it = colNames_.begin();
  for( ; it != colNames_.end(); ++it, ++colNum) {
    BCAttribute* att = coll_->attributeByName(*it);
    if(!att) {
      continue;
    }
    
    if(columnText(colNum) == att->title()) {
      continue;
    }

    if(colNum < columns()) {
      setColumnText(colNum, att->title());
      // if the column has a neighbor to the right with the current title
      // copy the width
      if(colNum+1 < columns() && columnText(colNum+1) == att->title()) {
        setColumnWidth(colNum, columnWidth(colNum+1));
      }
    } else {
      int i = addColumn(att->title());
      setColumnWidthMode(i, QListView::Manual);
      if(att->type() == BCAttribute::Bool) {
        setColumnAlignment(i, Qt::AlignHCenter);
      }
    }

    QListViewItemIterator it(this);
    for( ; it.current(); ++it) {
      BCUnitItem* item = static_cast<BCUnitItem*>(it.current());
      if(att->type() == BCAttribute::Bool) {
        // for bools, the value is "1", just check to see if empty
        if(item->unit()->attribute(att->name()).isEmpty()) {
          it.current()->setPixmap(colNum, QPixmap());
          it.current()->setText(colNum, QString());
        } else {
          it.current()->setPixmap(colNum, m_checkPix);
          // needed for sorting
          it.current()->setText(colNum, QString::fromLatin1(" "));
        }
      } else {
        item->setPixmap(colNum, QPixmap());
        item->setText(colNum, item->unit()->attributeFormatted(att->name(), att->flags()));
      }
    }
  }
  // if the column number is less than the number of columns, need to remove the extras
  for(int i = colNum; i < columns(); ++i) {
    removeColumn(i);
  }
}

bool BCDetailedListView::eventFilter(QObject* obj_, QEvent* ev_) {
  if(ev_->type() == QEvent::MouseButtonPress
      && static_cast<QMouseEvent*>(ev_)->button() == RightButton
      && obj_->isA("QHeader")) {
    m_headerMenu->popup(static_cast<QMouseEvent*>(ev_)->globalPos());
    return true;
  }
  return KListView::eventFilter(obj_, ev_);
}

void BCDetailedListView::slotHeaderMenuActivated(int id_) {
  bool checked = m_headerMenu->isItemChecked(id_);
  m_headerMenu->setItemChecked(id_, !checked);

  QStringList names;
  // got to be a better way than to iterate through ids
  QMap<int, QString>::Iterator it;
  for(it = m_columnMap.begin(); it != m_columnMap.end(); ++it) {
    if(m_headerMenu->isItemChecked(it.key())) {
      // the menu ids are negative, so need to prepend to list
      names.prepend(it.data());
    }
  }
  // TODO: fix
  setColumns(m_doc->collectionById(0), names);
}
