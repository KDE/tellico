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
#include "bcunititem.h"
#include "bccollection.h"
#include "bookcasedoc.h"
#include "bcfilter.h"

#include <klocale.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kdebug.h>

#include <qptrlist.h>
#include <qstringlist.h>
#include <qvaluelist.h>
#include <qheader.h>

BCDetailedListView::BCDetailedListView(BookcaseDoc* doc_, QWidget* parent_, const char* name_/*=0*/)
    : KListView(parent_, name_), m_doc(doc_), m_filter(0) {
//  kdDebug() << "BCDetailedListView()" << endl;
  setAllColumnsShowFocus(true);
  setShowSortIndicator(true);
  setSelectionMode(QListView::Extended);

//  connect(this, SIGNAL(selectionChanged(QListViewItem*)),
//          SLOT(slotSelected(QListViewItem*)));
  connect(this, SIGNAL(selectionChanged()), SLOT(slotSelectionChanged()));
// if a list view item is clicked...something is modified, and then the user
// clicks on it again, no signal is sent because the selection didn't change...so the
// next connection must be made as well. The side effect is that two signals are sent when
// the user clicks on a different list view item
//  connect(this, SIGNAL(clicked(QListViewItem*)),
//          SLOT(slotSelectionChanged()));

  connect(this, SIGNAL(rightButtonPressed(QListViewItem*, const QPoint&, int)),
          SLOT(slotRMB(QListViewItem*, const QPoint&, int)));

  // header menu
  header()->setClickEnabled(true);
  header()->installEventFilter(this);
  connect(header(), SIGNAL(sizeChange(int, int, int)),
          this, SLOT(slotCacheColumnWidth(int, int, int)));
  
  m_headerMenu = new KPopupMenu(this);
  m_headerMenu->setCheckable(true);
  connect(m_headerMenu, SIGNAL(activated(int)),
          this, SLOT(slotHeaderMenuActivated(int)));
  
  // item menu
  QPixmap remove = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("remove"), KIcon::Small);
  m_itemMenu = new KPopupMenu(this);
  m_itemMenu->insertItem(remove, i18n("Delete Book"), this, SLOT(slotHandleDelete()));

  m_bookPix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("book"), KIcon::User);
  m_checkPix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("ok"), KIcon::Small);

  // need this, so that sorting is allowed
  setSorting(0, true);
}

BCDetailedListView::~BCDetailedListView() {
  delete m_filter;
  m_filter = 0;
}

void BCDetailedListView::slotAddCollection(BCCollection* coll_) {
  if(!coll_) {
    return;
  }

//  kdDebug() << "BCDetailedListView::slotAddCollection()" << endl;

  // TODO fix for multiple collections
  // if columns already exist, check to see if any need to be removed or added
  if(columns() > 0 && !coll_->attributeTitles().isEmpty()) {
    QStringList colTitles;
    for(int i = 0; i < columns(); ++i) {
      colTitles += columnText(i);
    }
    QStringList list = coll_->attributeTitles();
    QStringList::ConstIterator it;
    for(it = list.begin(); it != list.end(); ++it) {
      if(colTitles.findIndex(*it) == -1) {
        slotAddColumn(coll_, coll_->attributeByTitle(*it));
      }
      colTitles.remove(*it);
    }
    // if colTitles is not now empty, need to remove some columns
    if(!colTitles.isEmpty()) {
      for(it = colTitles.begin(); it != colTitles.end(); ++it) {
        slotRemoveColumn(coll_, *it);
      }
    }
  }
  
  QPtrListIterator<BCUnit> unitIt(coll_->unitList());
  for( ; unitIt.current(); ++unitIt) {
    slotAddItem(unitIt.current());
  }
}

void BCDetailedListView::slotReset() {
  kdDebug() << "BCDetailedListView::slotReset()" << endl;
  //clear() does not remove columns
  clear();
  m_selectedUnits.clear();
//  while(columns() > 0) {
//    removeColumn(0);
//  }
  delete m_filter;
  m_filter = 0;
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
  bool match = true;
  if(m_filter) {
    match = m_filter->matches(unit_);
    item->setVisible(match);
  }
  
  if(isUpdatesEnabled() && match) {
    sort();
    ensureItemVisible(item);
//    setSelected(item, true);
  }
}

void BCDetailedListView::slotModifyItem(BCUnit* unit_) {
  if(!unit_) {
    kdWarning() << "BCDetailedListView::slotModifyItem() - null unit pointer!" << endl;
    return;
  }

  slotClearSelection();
//  kdDebug() << "BCDetailedListView::slotModifyItem() - " << unit_->title() << endl;
  BCUnitItem* item = locateItem(unit_);
  if(item) {
    populateItem(item);
    bool match = true;
    if(m_filter) {
      match = m_filter->matches(unit_);
      item->setVisible(match);
    }
    
    if(match) {
      sort();
      ensureItemVisible(item);
    }
  } else {
    kdWarning() << "BCDetailedListView::slotModifyItem() - no item found for " << unit_->title() << endl;
    return;
  }
}

void BCDetailedListView::slotRemoveItem(BCUnit* unit_) {
  if(!unit_) {
    kdWarning() << "BCDetailedListView::slotRemoveItem() - null unit pointer!" << endl;
    return;
  }

//  kdDebug() << "BCDetailedListView::slotRemoveItem() - " << unit_->title() << endl;

  slotClearSelection();
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
    const BCUnit* unit = static_cast<BCUnitItem*>(item)->unit();
    next = item->nextSibling();
    if(unit->collection() == coll_) {
      delete item;
    }
    item = next;
  }

  // clear the filter, too
  delete m_filter;
  m_filter = 0;
}

void BCDetailedListView::slotHandleDelete() {
  BCUnitItem* item = static_cast<BCUnitItem*>(currentItem());
  if(!item || !item->unit()) {
    return;
  }

  emit signalDeleteUnit(item->unit());
}

void BCDetailedListView::populateItem(BCUnitItem* item_) {
  const BCUnit* unit = item_->unit();
  if(!unit) {
    return;
  }

  for(int colNum = 0; colNum < columns(); ++colNum) {
    BCAttribute* att = unit->collection()->attributeByTitle(columnText(colNum));
    if(!att) {
      kdWarning() << "BCDetailedListView::populateItem() - no attribute found for " << columnText(colNum) << endl;
      return;
    }

    setPixmapAndText(item_, colNum, att);
  }
}

void BCDetailedListView::slotRMB(QListViewItem* item_, const QPoint& point_, int) {
  if(item_ && m_itemMenu->count() > 0) {
//    setSelected(item_, true);
    m_itemMenu->popup(point_);
  }
}

void BCDetailedListView::slotSelectionChanged() {
  // all items in the listview are unitItems
  BCUnitItem* item;
  QListViewItemIterator it(this);
  for( ; it.current(); ++it) {
    item = static_cast<BCUnitItem*>(it.current());
    if(item->isSelected()) {
      if(item->unit() && !m_selectedUnits.containsRef(item->unit())) {
        // the reason I do it this way is because I want the first item in the list
        // to be the first item that was selected, so the edit widget can fill its
        // contents with the first unit
//        kdDebug() << "BCDetailedListView::slotSelectionChanged() - adding " << item->unit()->title() << endl;
        m_selectedUnits.append(item->unit());;
      }
    } else { // it's ok to call this even when unit not in list
      m_selectedUnits.removeRef(item->unit());
    }
  }
  
  emit signalUnitSelected(m_selectedUnits);
}

void BCDetailedListView::slotSetSelected(BCUnit* unit_) {
//  kdDebug() << "BCDetailedListView::slotSetSelected()" << endl;
  // if unit_ is null pointer, set no selected
  if(!unit_) {
    setSelected(currentItem(), false);
    return;
  }

  BCUnitItem* item = locateItem(unit_);

  slotClearSelection();
  blockSignals(true);
  setSelected(item, true);
  setCurrentItem(item);
  blockSignals(false);
  ensureItemVisible(item);
}

BCUnitItem* const BCDetailedListView::locateItem(const BCUnit* unit_) {
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
//  kdDebug() << "BCDetailedListView::slotClearSelection()" << endl;
  blockSignals(true);
  selectAll(false);
  setCurrentItem(0);
  blockSignals(false);
  m_selectedUnits.clear();
}

// this is JUST for the initial setup
// showing or hiding columsn should use showColumn()
void BCDetailedListView::setColumns(BCCollection* coll_, const QStringList& colNames_,
                                    const QValueList<int>& colWidths_) {
//  kdDebug() << "BCDetailedListView::setColumns() " << endl;
//  kdDebug() << "Titles: " << colNames_.join(QString::fromLatin1(";")) << endl;
//  QString widths;
//  QValueList<int>::ConstIterator wIt;
//  for(wIt = colWidths_.begin(); wIt != colWidths_.end(); ++wIt) {
//    widths += QString::number(*wIt) + ';';
//  }
//  kdDebug() << "Widths: " << widths << endl;
  
  if(!coll_) {
    return;
  }

  if(columns() > 0) {
    kdWarning() << "BCDetailedListView::setColumns() called when columns already exist!" << endl;
    return;
  }
  
  // populate header menu
  m_headerMenu->insertTitle(i18n("View Columns"));
    
  // get all the attributes in the document which are the same unit type
  // this assumes that some day, I'll get around to using a QWidgetStack
  // or something so I can show collections of different unit types
//  BCCollection::CollectionType type = coll_->collectionType();
//  BCAttributeList list = m_doc->uniqueAttributes(type);
  BCAttributeList list = coll_->attributeList();
  
  // initialize the width cache
  m_columnWidths.resize(list.count(), 0);
  
  // iterate over the all the attributes
  BCAttributeListIterator it(list);
  for( ; it.current(); ++it) {
//    kdDebug() << "BCDetailedListView::setColumns() - adding column " << it.current()->title() << endl;
    // columns gets added for every attribute
    int col = addColumn(it.current()->title());
    m_headerMenu->insertItem(it.current()->title(), col);
    // bools have a checkmark pixmap, so center it
    if(it.current()->type() == BCAttribute::Bool) {
      setColumnAlignment(col, Qt::AlignHCenter);
    }

    // if the width is -1, then leave the width alone
    // otherwise, set it
    if(colWidths_[col] > -1) {
      setColumnWidth(col, colWidths_[col]);
      setColumnWidthMode(col, QListView::Manual);
      m_columnWidths[header()->mapToSection(col)] = colWidths_[col];
    }
    
    if(colWidths_[col] == 0) {
      header()->setResizeEnabled(false, col);
    } else if(colNames_.findIndex(it.current()->name()) > -1) {
      m_headerMenu->setItemChecked(col, true);
    }
  }
}

bool BCDetailedListView::eventFilter(QObject* obj_, QEvent* ev_) {
  if(ev_->type() == QEvent::MouseButtonPress
      && static_cast<QMouseEvent*>(ev_)->button() == Qt::RightButton
      && obj_->isA("QHeader")) {
    m_headerMenu->popup(static_cast<QMouseEvent*>(ev_)->globalPos());
    return true;
  }
  return KListView::eventFilter(obj_, ev_);
}

// this implementation borrows from the code in the juk application
// in the file playlistsplitter.cpp by Scott Wheeler 
void BCDetailedListView::slotHeaderMenuActivated(int id_) {
//  kdDebug() << "BCDetailedListView::slotHeaderMenuActivated() - " << m_headerMenu->text(id_) << endl;
  bool checked = m_headerMenu->isItemChecked(id_);
  checked = !checked; // toggle
  m_headerMenu->setItemChecked(id_, checked);

  int col = m_headerMenu->indexOf(id_) - 1;  // subtract 1 because there's a title item

  if(checked) { // add a column
    showColumn(col);
  } else {
    hideColumn(col);
  }
}

void BCDetailedListView::slotRefresh() {
  if(childCount() == 0) {
    return;
  }
  
  // the algorithm here is to iterator over each column, then over every list item
  BCUnitItem* item = static_cast<BCUnitItem*>(firstChild());
  BCCollection* coll = item->unit()->collection();
  BCAttribute* att;

  for(int colNum = 0; colNum < columns(); ++colNum) {
    att = coll->attributeByTitle(columnText(colNum));

    // iterate over all items
    QListViewItemIterator it(this);
    for( ; it.current(); ++it) {
      item = static_cast<BCUnitItem*>(it.current());
      
      // TODO: maybe someday there will be multiple collections
//      if(item->unit()->collection() != coll) {
//        coll = item->unit()->collection();
//        att = coll->attributeByTitle(columnText(colNum));
//      }
      setPixmapAndText(item, colNum, att);
      
      // if we're doing this for the first time, go ahead and pass through filter
      if(colNum == 0) {
        if(m_filter && !m_filter->matches(item->unit())) {
          item->setVisible(false);
        } else {
          item->setVisible(true);
        }
      }
    }
  }
}

void BCDetailedListView::setPixmapAndText(BCUnitItem* item_, int col_, BCAttribute* att_) {
  // for bools, there will be no text
  // if the bool is not empty, show the checkmark pixmap
  if(att_->type() == BCAttribute::Bool) {
    QString value = item_->unit()->attribute(att_->name());
    if(value.isEmpty()) {
      item_->setPixmap(col_, QPixmap());
      item_->setText(col_, QString());
    } else {
      item_->setPixmap(col_, m_checkPix);
      // needed for sorting
      item_->setText(col_, QString::fromLatin1(" "));
    }
  } else { // for everything else, there's no pixmap, unless it's the first column
    // TODO: fix for multiple types
    if(col_ == 0) {
      item_->setPixmap(col_, m_bookPix);
    } else {
      item_->setPixmap(col_, QPixmap());
    }
    QString value = item_->unit()->attributeFormatted(att_->name(), att_->formatFlag());
    item_->setText(col_, value);
  }
}

QStringList BCDetailedListView::columnTitles() const {
  QStringList colTitles;
  // only visible if width > 0
  for(int col = 0; col < columns(); ++col) {
    colTitles += columnText(col);
  }
  return colTitles;
}

void BCDetailedListView::showColumn(int col_) {
//  kdDebug() << "BCDetailedListView::showColumn() - " << columnText(col_) << endl;
  int w = m_columnWidths[col_]; // this should be safe - all were initialized to 0
  if(w == 0) {
    setColumnWidthMode(col_, QListView::Maximum);
    QListViewItemIterator it(this);
    for( ; it.current(); ++it) {
      w = QMAX(it.current()->width(fontMetrics(), this, col_), w);
    }
    w = QMAX(w, 50);
  }

  setColumnWidth(col_, w);
  header()->setResizeEnabled(true, col_);
  triggerUpdate();
}

void BCDetailedListView::hideColumn(int col_) {
//  kdDebug() << "BCDetailedListView::hideColumn() - " << columnText(col_) << endl;
  setColumnWidthMode(col_, QListView::Manual);
  setColumnWidth(col_, 0);
  header()->setResizeEnabled(false, col_);
//  setResizeMode(QListView::LastColumn);
  triggerUpdate();
}

void BCDetailedListView::slotCacheColumnWidth(int section_, int oldSize_, int newSize_) {
  // if the old size was 0, update the menu
  if(oldSize_ == 0 && newSize_ > 0) {
    m_headerMenu->setItemChecked(m_headerMenu->idAt(section_+1), true); // add 1 for title item
  }

  m_columnWidths[section_] = newSize_;
  setColumnWidthMode(section_, QListView::Manual);
}

void BCDetailedListView::setFilter(const BCFilter* filter_) {
  delete m_filter;
  m_filter = filter_;

  // iterate over all items
  BCUnitItem* item;
  QListViewItemIterator it(this);
  for( ; it.current(); ++it) {
    item = static_cast<BCUnitItem*>(it.current());
    if(m_filter && !m_filter->matches(item->unit())) {
      item->setVisible(false);
    } else {
      item->setVisible(true);
    }
  }
}

const BCFilter* BCDetailedListView::filter() const {
  return m_filter;
}

// don't care about collection right now
// TODO: fix when multiple collections supported
void BCDetailedListView::slotAddColumn(BCCollection*, BCAttribute* att_) {
//  kdDebug() << "BCDetailedListView::slotAddColumn() - " << att_->title() << endl;
  int col = addColumn(att_->title());
  int id = m_headerMenu->insertItem(att_->title());

  m_headerMenu->setItemChecked(id, false);
  // bools have a checkmark pixmap, so center it
  if(att_->type() == BCAttribute::Bool) {
    setColumnAlignment(col, Qt::AlignHCenter);
  }

  slotRefresh();
//  m_columnWidths.push_back(columnWidth(col));
  m_columnWidths.push_back(0);
  // by default, keep the new column hidden
  hideColumn(col);
}

void BCDetailedListView::slotModifyColumn(BCCollection*, BCAttribute* newAtt_, BCAttribute* oldAtt_) {
  int sec; // I need it for after the loop
  for(sec = 0; sec < columns(); ++sec) {
    if(header()->label(sec) == oldAtt_->title()) {
      break;
    }
  }

  // I thought this would have to be mapped to index, but not the case
  setColumnText(sec, newAtt_->title());
  m_headerMenu->changeItem(m_headerMenu->idAt(sec+1), newAtt_->title()); // add 1 since menu has title
}

void BCDetailedListView::slotRemoveColumn(BCCollection* coll_, BCAttribute* att_) {
  slotRemoveColumn(coll_, att_->title());
}

// don't care about collection right now
// TODO: fix when multiple collections supported
void BCDetailedListView::slotRemoveColumn(BCCollection*, const QString& title_) {
//  kdDebug() << "BCDetailedListView::slotRemoveColumn() - " << title_ << endl;
  
  int sec; // I need it for after the loop
  for(sec = 0; sec < columns(); ++sec) {
    if(header()->label(sec) == title_) {
//      kdDebug() << "Removing section " << sec << endl;
      break;
    }
  }

  if(sec == columns()) {
    kdWarning() << "BCDetailedListView::slotRemoveColumn() - no column named " << title_ << endl;
    return;
  }

  m_headerMenu->removeItem(m_headerMenu->idAt(sec+1)); // add 1 since menu has title

  m_columnWidths.erase(&m_columnWidths[sec]);

  // I thought this would have to be mapped to index, but not the case
  removeColumn(sec);

  for(int i = sec; i < columns(); ++i) {
    header()->setResizeEnabled(columnWidth(i) > 0, header()->mapToSection(i));
  }
  triggerUpdate();
}

int BCDetailedListView::prevSortedColumn() const {
  return m_prevSortColumn;
}

void BCDetailedListView::setPrevSortedColumn(int column_) {
  m_prevSortColumn = column_;
}

void BCDetailedListView::setSorting(int column_, bool ascending_/*=true*/) {
  if(column_ != columnSorted()) {
    setPrevSortedColumn(columnSorted());
  }
  KListView::setSorting(column_, ascending_);
}
