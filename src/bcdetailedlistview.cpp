/***************************************************************************
                           bcdedtailedlistview.cpp
                             -------------------
    begin                : Tue Sep 4 2001
    copyright            : (C) 2001, 2002, 2003 by Robby Stephenson
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
#include "bcfilter.h"
#include "bookcase.h"
#include "bcutils.h"

#include <klocale.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kapplication.h>
#include <kaction.h>

#include <qptrlist.h>
#include <qstringlist.h>
#include <qvaluelist.h>
#include <qheader.h>

static const int MIN_COL_WIDTH = 50;

BCDetailedListView::BCDetailedListView(QWidget* parent_, const char* name_/*=0*/)
    : KListView(parent_, name_), m_filter(0),
    m_prevSortColumn(-1), m_prev2SortColumn(-1), m_firstSection(-1) {
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
  connect(header(), SIGNAL(indexChange(int, int, int)),
          SLOT(slotUpdatePixmap()));

  // header menu
  header()->setClickEnabled(true);
  header()->installEventFilter(this);
  connect(header(), SIGNAL(sizeChange(int, int, int)),
          this, SLOT(slotCacheColumnWidth(int, int, int)));
  
  m_headerMenu = new KPopupMenu(this);
  m_headerMenu->setCheckable(true);
  m_headerMenu->insertTitle(i18n("View Columns"));
  connect(m_headerMenu, SIGNAL(activated(int)),
          this, SLOT(slotHeaderMenuActivated(int)));
  
  Bookcase* bookcase = static_cast<Bookcase*>(QObjectAncestor(parent_, "Bookcase"));
  m_itemMenu = new KPopupMenu(this);
  bookcase->action("edit_delete")->plug(m_itemMenu);

  m_checkPix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("ok"), KIcon::Small);
}

BCDetailedListView::~BCDetailedListView() {
  delete m_filter;
  m_filter = 0;
}

void BCDetailedListView::addCollection(BCCollection* coll_) {
  if(!coll_) {
    return;
  }

//  kdDebug() << "BCDetailedListView::addCollection()" << endl;

  KConfig* config = kapp->config();
  config->setGroup(QString::fromLatin1("Options - %1").arg(coll_->unitName()));

  QStringList colNames = config->readListEntry("ColumnNames");
  if(colNames.isEmpty()) {
    colNames = coll_->defaultViewAttributes();
  }

  // this block compensates for the chance that the user added an attribute and it wasn't
  // written to the widths. Also compensates for 0.5.x to 0.6.x column layout changes
  QValueList<int> colWidths = config->readIntListEntry("ColumnWidths");
  if(colWidths.empty()) {
    colWidths.insert(colWidths.begin(), colNames.count(), -1); // automatic width
  }

  QValueList<int> colOrder = config->readIntListEntry("ColumnOrder");

  // need to remove values for fields which don't exist in the current collection
  QStringList newCols;
  QValueList<int> newWidths, removeCols;
  for(unsigned i = 0; i < colNames.count(); ++i) {
    if(coll_->attributeByName(colNames[i]) != 0) {
      newCols += colNames[i];
      newWidths += colWidths[i];
    } else {
      removeCols += i;
    }
  }
  colNames = newCols;
  colWidths = newWidths;

  qHeapSort(removeCols);
  // now need to shift values in the order if any columns were removed
  // only need to shift by number of "holes"
  QValueList<int> newOrder;
  for(QValueList<int>::ConstIterator it = colOrder.begin(); it != colOrder.end(); ++it) {
    if(removeCols.findIndex(*it) == -1) {
      int i = *it;
      for(unsigned j = 0; j < removeCols.count() && removeCols[j] < i; ++j) {
        --i;
      }
      newOrder += i;
    }
  }
  colOrder = newOrder;

  bool none = true;
  for(BCAttributeListIterator attIt(coll_->attributeList()); attIt.current(); ++attIt) {
    if(colNames.findIndex(attIt.current()->name()) > -1 && colWidths.count() > 0) {
      addAttribute(attIt.current(), colWidths.front());
      if(none && colWidths.front() != 0) {
        none = false;
      }
      colWidths.pop_front();
    } else {
      addAttribute(attIt.current(), 0);
    }
  }
  if(none && columns() > 0) {
    showColumn(coll_->attributeNames().findIndex(colNames[0]));
  }

  QValueList<int>::ConstIterator it = colOrder.begin();
  for(int i = 0; it != colOrder.end(); ++it) {
    header()->moveSection(i++, *it);
  }
  slotUpdatePixmap();

  int sortCol = config->readNumEntry("SortColumn", 0);
  bool sortAsc = config->readBoolEntry("SortAscending", true);
  setSorting(sortCol, sortAsc);
  int prevSortCol = config->readNumEntry("PrevSortColumn", -1);
  int prev2SortCol = config->readNumEntry("Prev2SortColumn", -1);
  setPrevSortedColumn(prevSortCol, prev2SortCol);
  
  triggerUpdate();
  kapp->processEvents();
  setUpdatesEnabled(false);

  unsigned count = coll_->unitList().count();
  BCUnitListIterator unitIt(coll_->unitList());
  for(unsigned j = 0; unitIt.current(); ++unitIt, ++j) {
    addItem(unitIt.current());

    // magic number here
    if(j%20 == 0) {
      emit signalFractionDone(static_cast<float>(j)/static_cast<float>(count));
    }
  }

  setUpdatesEnabled(true);
  triggerUpdate();
}

void BCDetailedListView::slotReset() {
//  kdDebug() << "BCDetailedListView::slotReset()" << endl;
  //clear() does not remove columns
  clear();
  m_selectedUnits.clear();
//  while(columns() > 0) {
//    removeColumn(0);
//  }
  delete m_filter;
  m_filter = 0;
}

void BCDetailedListView::addItem(BCUnit* unit_) {
  if(!unit_) {
    kdWarning() << "BCDetailedListView::slotAddItem() - null unit pointer!" << endl;
    return;
  }
  
  if(m_unitPix.isNull()) {
    m_unitPix = KGlobal::iconLoader()->loadIcon(unit_->collection()->unitName(), KIcon::User);
  }


//  kdDebug() << "BCDetailedListView::slotAddItem() - " << unit_->attribute("title") << endl;

  BCUnitItem* item = new BCUnitItem(this, unit_);

  populateItem(item);
  bool match = true;
  if(m_filter) {
    match = m_filter->matches(unit_);
    item->setVisible(match);
  }
  
  if(isUpdatesEnabled() && match) {
    sort();
    ensureItemVisible(item);
    setCurrentItem(item);
    if(m_selectedUnits.count() > 0) {
      blockSignals(true);
      clearSelection();
      setSelected(item, true);
      blockSignals(false);
    }
  }
}

void BCDetailedListView::modifyItem(BCUnit* unit_) {
  if(!unit_) {
    kdWarning() << "BCDetailedListView::modifyItem() - null unit pointer!" << endl;
    return;
  }

//  kdDebug() << "BCDetailedListView::slotModifyItem() - " << unit_->title() << endl;
  BCUnitItem* item = locateItem(unit_);
  if(item) {
    populateItem(item);
    bool match = true;
    if(m_filter) {
      match = m_filter->matches(unit_);
      item->setVisible(match);
    }
    
    if(isUpdatesEnabled() && match) {
      sort();
      setCurrentItem(item);
      ensureItemVisible(item);
    }
    
    if(!item->isSelected() && m_selectedUnits.count() > 0) {
      blockSignals(true);
      clearSelection();
      setSelected(item, true);
      blockSignals(false);
    }
  } else {
    kdWarning() << "BCDetailedListView::modifyItem() - no item found for " << unit_->title() << endl;
    return;
  }
}

void BCDetailedListView::removeItem(BCUnit* unit_) {
  if(!unit_) {
    kdWarning() << "BCDetailedListView::removeItem() - null unit pointer!" << endl;
    return;
  }

//  kdDebug() << "BCDetailedListView::removeItem() - " << unit_->title() << endl;

  clearSelection();
  delete locateItem(unit_);
}

// TODO: more efficient way to do this?
void BCDetailedListView::removeCollection(BCCollection* coll_) {
  if(!coll_) {
    kdWarning() << "BCDetailedListView::removeCollection() - null coll pointer!" << endl;
    return;
  }

//  kdDebug() << "BCDetailedListView::removeCollection() - " << coll_->title() << endl;

  saveConfig(coll_);

  clear();

  while(columns() > 0) {
    removeColumn(0);
  }
  
  m_headerMenu->clear();
  m_headerMenu->insertTitle(i18n("View Columns"));

  m_columnWidths.clear();
  m_isNumber.clear();
  m_unitPix = QPixmap();

  m_selectedUnits.clear();

  // clear the filter, too
  delete m_filter;
  m_filter = 0;
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
  
  emit signalUnitSelected(this, m_selectedUnits);
}

// don't shadow QListView::setSelected
void BCDetailedListView::setUnitSelected(BCUnit* unit_) {
//  kdDebug() << "BCDetailedListView::slotSetSelected()" << endl;
  clearSelection();
  // if unit_ is null pointer, just return
  if(!unit_) {
    return;
  }

  BCUnitItem* item = locateItem(unit_);

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

void BCDetailedListView::clearSelection() {
//  kdDebug() << "BCDetailedListView::clearSelection()" << endl;
  blockSignals(true);
  selectAll(false);
  setCurrentItem(0);
  blockSignals(false);
  m_selectedUnits.clear();
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
  slotUpdatePixmap();
}

void BCDetailedListView::slotRefresh() {
  if(childCount() == 0) {
    return;
  }

  // the algorithm here is to iterate over each column, then over every list item
  BCCollection* coll = static_cast<BCUnitItem*>(firstChild())->unit()->collection();
  BCAttribute* att;
  BCUnitItem* item;

  for(int colNum = 0; colNum < columns(); ++colNum) {
    att = coll->attributeByTitle(columnText(colNum));

    // iterate over all items
    QListViewItemIterator it(this);
    for( ; it.current(); ++it) {
      item = static_cast<BCUnitItem*>(it.current());
      
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
  if(!item_) {
    return;
  }

  // if the bool is not empty, show the checkmark pixmap
  if(att_->type() == BCAttribute::Bool) {
    QString value = item_->unit()->attribute(att_->name());
    item_->setPixmap(col_, value.isEmpty() ? QPixmap() : m_checkPix);
    item_->setText(col_, QString::null);
  } else { // for everything else, there's no pixmap, unless it's the first column
    if(col_ == m_firstSection) {
      item_->setPixmap(col_, m_unitPix);
    } else {
      item_->setPixmap(col_, QPixmap());
    }
    QString value = item_->unit()->attributeFormatted(att_->name(), att_->formatFlag());
    item_->setText(col_, value);
  }
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
    w = QMAX(w, MIN_COL_WIDTH);
  } else {
    setColumnWidthMode(col_, QListView::Manual);
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

void BCDetailedListView::addAttribute(BCAttribute* att_, int width_/*=-1*/) {
//  kdDebug() << "BCDetailedListView::slotAddColumn() - " << att_->title() << endl;
  int col = addColumn(att_->title());

  // bools have a checkmark pixmap, so center it. Numbers too
  if(att_->type() == BCAttribute::Bool || att_->type() == BCAttribute::Number) {
    setColumnAlignment(col, Qt::AlignHCenter);
  }

  // width might be -1, which means set the width to maximum
  // but m_columnWidths is the cached width, so just set it to 0
  m_columnWidths.push_back(QMAX(width_, 0));
  
  m_isNumber.push_back(att_->type() == BCAttribute::Number);
  
  int id = m_headerMenu->insertItem(att_->title());
  if(width_ == 0) {
    m_headerMenu->setItemChecked(id, false);
    hideColumn(col);
  } else {
    m_headerMenu->setItemChecked(id, true);
    showColumn(col);
  }
}

void BCDetailedListView::modifyAttribute(BCAttribute* newAtt_, BCAttribute* oldAtt_) {
  int sec; // I need it for after the loop
  for(sec = 0; sec < columns(); ++sec) {
    if(header()->label(sec) == oldAtt_->title()) {
      break;
    }
  }

  // I thought this would have to be mapped to index, but not the case
  setColumnText(sec, newAtt_->title());
  m_isNumber[sec] = (newAtt_->type() == BCAttribute::Number);
  if(newAtt_->type() == BCAttribute::Bool || newAtt_->type() == BCAttribute::Number) {
    setColumnAlignment(sec, Qt::AlignHCenter);
  } else {
    setColumnAlignment(sec, Qt::AlignLeft);
  }
  m_headerMenu->changeItem(m_headerMenu->idAt(sec+1), newAtt_->title()); // add 1 since menu has title
}

void BCDetailedListView::removeAttribute(BCAttribute* att_) {
//  kdDebug() << "BCDetailedListView::slotRemoveColumn() - " << att_->title() << endl;

  int sec; // I need it for after the loop
  for(sec = 0; sec < columns(); ++sec) {
    if(header()->label(sec) == att_->title()) {
//      kdDebug() << "Removing section " << sec << endl;
      break;
    }
  }

  if(sec == columns()) {
    kdWarning() << "BCDetailedListView::slotRemoveColumn() - no column named " << att_->title() << endl;
    return;
  }

  m_headerMenu->removeItem(m_headerMenu->idAt(sec+1)); // add 1 since menu has title

  m_columnWidths.erase(&m_columnWidths[sec]);
  m_isNumber.erase(&m_isNumber[sec]);

  // I thought this would have to be mapped to index, but not the case
  removeColumn(sec);

  // sometimes resizeEnabled gets messed up
  for(int i = sec; i < columns(); ++i) {
    header()->setResizeEnabled(columnWidth(i) > 0, header()->mapToSection(i));
  }
  slotUpdatePixmap();
  triggerUpdate();
}

void BCDetailedListView::reorderAttributes(const BCAttributeList& list_) {
//  kdDebug() << "BCDetailedListView::reorderAttributes()" << endl;
  // find the first out of place field
  int sec = 0;
  BCAttributeListIterator it(list_);
  for(sec = 0; it.current() && sec < columns(); ++sec, ++it) {
    if(header()->label(sec) != it.current()->title()) {
      break;
    }
  }

  QStringList visible = visibleColumns();
  for( ; it.current() && sec < columns();  ++sec, ++it) {
    header()->setLabel(sec, it.current()->title());
    bool isVisible = (visible.findIndex(it.current()->title()) > -1);
    m_headerMenu->changeItem(m_headerMenu->idAt(sec+1), it.current()->title());
    m_headerMenu->setItemChecked(m_headerMenu->idAt(sec+1), isVisible);
    m_columnWidths[sec] = 0;
    if(it.current()->type() == BCAttribute::Bool || it.current()->type() == BCAttribute::Number) {
      setColumnAlignment(header()->mapToIndex(sec), Qt::AlignHCenter);
    } else {
      setColumnAlignment(header()->mapToIndex(sec), Qt::AlignLeft);
    }
    m_isNumber[sec] = (it.current()->type() == BCAttribute::Number);

    if(isVisible) {
      showColumn(sec);
    } else {
      hideColumn(sec);
    }
  }

  slotRefresh();
  slotUpdatePixmap();
  triggerUpdate();
}

int BCDetailedListView::prevSortedColumn() const {
  return m_prevSortColumn;
}

int BCDetailedListView::prev2SortedColumn() const {
  return m_prev2SortColumn;
}

void BCDetailedListView::setPrevSortedColumn(int prev1_, int prev2_) {
  m_prevSortColumn = prev1_;
  m_prev2SortColumn = prev2_;
}

void BCDetailedListView::setSorting(int column_, bool ascending_/*=true*/) {
  if(column_ != columnSorted()) {
    m_prev2SortColumn = m_prevSortColumn;
    m_prevSortColumn = columnSorted();
  }
  KListView::setSorting(column_, ascending_);
}

// it's possible to have a zero-length vector and have this called, so check bounds
bool BCDetailedListView::isNumber(int column_) const {
  return column_ < static_cast<int>(m_isNumber.size()) && m_isNumber[column_];
}

void BCDetailedListView::updateFirstSection() {
  for(int numCol = 0; numCol < columns(); ++numCol) {
    if(columnWidth(numCol) > 0) {
      m_firstSection = header()->mapToSection(numCol);
      return;
    }
  }
}

void BCDetailedListView::slotUpdatePixmap() {
  int oldSection = m_firstSection;
  updateFirstSection();
  if(oldSection == m_firstSection) {
    return;
  }
  
  QListViewItemIterator it(this);
  for( ; it.current(); ++it) {
    it.current()->setPixmap(oldSection, QPixmap());
    it.current()->setPixmap(m_firstSection, m_unitPix);
  }
}

void BCDetailedListView::saveConfig(BCCollection* coll_) {
  // only save if not-empty, or not initial startup collection
  if(childCount() == 0 &&
     static_cast<Bookcase*>(QObjectAncestor(parent(), "Bookcase"))->isNewDocument()) {
    return;
  }

  KConfig* config = kapp->config();
  config->setGroup(QString::fromLatin1("Options - %1").arg(coll_->unitName()));

  QValueList<int> widths, order;
  for(int i = 0; i < columns(); ++i) {
    if(columnWidthMode(i) == QListView::Manual) {
      widths += columnWidth(i);
    } else {
      widths += -1; // Maximum width mode
    }
    order += header()->mapToIndex(i);
  }
  config->writeEntry("ColumnWidths", widths);
  config->writeEntry("ColumnOrder", order);
  config->writeEntry("SortColumn", columnSorted());
  config->writeEntry("SortAscending", ascendingSort());
  config->writeEntry("PrevSortColumn", prevSortedColumn());
  config->writeEntry("Prev2SortColumn", prev2SortedColumn());

  QStringList colNames;
  for(int col = 0; col < columns(); ++col) {
    colNames += coll_->attributeNameByTitle(columnText(col));
  }
  config->writeEntry("ColumnNames", colNames);
}

QString BCDetailedListView::sortColumnTitle1() const {
  return columnSorted() == -1 ? QString::null : header()->label(columnSorted());
}

QString BCDetailedListView::sortColumnTitle2() const {
  return prevSortedColumn() == -1 ? QString::null : header()->label(prevSortedColumn());
}

QString BCDetailedListView::sortColumnTitle3() const {
  return prev2SortedColumn() == -1 ? QString::null : header()->label(prev2SortedColumn());
}

QStringList BCDetailedListView::visibleColumns() const {
  QStringList titles;
  for(int i = 0; i < columns(); ++i) {
    if(columnWidth(header()->mapToSection(i)) > 0) {
      titles << columnText(header()->mapToSection(i));
    }
  }
  return titles;
}

BCUnitList BCDetailedListView::visibleUnits() {
  // We could just return the full collection unit list if the filter is 0
  // but printing depends on the sorted order
  BCUnitList list;
  for(QListViewItemIterator it(this); it.current(); ++it) {
    if(it.current()->isVisible()) {
      list.append(static_cast<BCUnitItem*>(it.current())->unit());
    }
  }
  return list;
}
