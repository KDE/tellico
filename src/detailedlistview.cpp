/***************************************************************************
    copyright            : (C) 2001-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "detailedlistview.h"
#include "entryitem.h"
#include "collection.h"
#include "filter.h"
#include "mainwindow.h"
#include "utils.h"

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

using Bookcase::DetailedListView;

DetailedListView::DetailedListView(QWidget* parent_, const char* name_/*=0*/)
    : KListView(parent_, name_), m_filter(0),
    m_prevSortColumn(-1), m_prev2SortColumn(-1), m_firstSection(-1) {
//  kdDebug() << "DetailedListView()" << endl;
  setAllColumnsShowFocus(true);
  setShowSortIndicator(true);
  setSelectionMode(QListView::Extended);

  connect(this, SIGNAL(selectionChanged()), SLOT(slotSelectionChanged()));
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

  MainWindow* mainWindow = static_cast<MainWindow*>(QObjectAncestor(parent_, "Bookcase::MainWindow"));
  m_itemMenu = new KPopupMenu(this);
  mainWindow->action("edit_edit_entry")->plug(m_itemMenu);
  mainWindow->action("edit_copy_entry")->plug(m_itemMenu);
  mainWindow->action("edit_delete_entry")->plug(m_itemMenu);

  m_checkPix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("ok"), KIcon::Small);
}

DetailedListView::~DetailedListView() {
  delete m_filter;
  m_filter = 0;
}

void DetailedListView::addCollection(Bookcase::Data::Collection* coll_) {
  if(!coll_) {
    return;
  }

//  kdDebug() << "DetailedListView::addCollection()" << endl;

  KConfig* config = kapp->config();
  config->setGroup(QString::fromLatin1("Options - %1").arg(coll_->entryName()));

  QStringList colNames = config->readListEntry("ColumnNames");
  if(colNames.isEmpty()) {
    colNames = QString::fromLatin1("title");
  }

  // this block compensates for the chance that the user added a field and it wasn't
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
    if(coll_->fieldByName(colNames[i]) != 0) {
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
  for(Data::FieldListIterator fIt(coll_->fieldList()); fIt.current(); ++fIt) {
    if(colNames.findIndex(fIt.current()->name()) > -1 && colWidths.count() > 0) {
      addField(fIt.current(), colWidths.front());
      if(none && colWidths.front() != 0) {
        none = false;
      }
      colWidths.pop_front();
    } else {
      addField(fIt.current(), 0);
    }
  }
  if(none && columns() > 0) {
    showColumn(coll_->fieldNames().findIndex(colNames[0]));
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

  unsigned count = coll_->entryCount();
  Data::EntryListIterator entryIt(coll_->entryList());
  for(unsigned j = 0; entryIt.current(); ++entryIt, ++j) {
    addEntry(entryIt.current());

    // magic number here
    if(j%20 == 0) {
      emit signalFractionDone(static_cast<float>(j)/static_cast<float>(count));
    }
  }

  setUpdatesEnabled(true);
  triggerUpdate();
}

void DetailedListView::slotReset() {
//  kdDebug() << "DetailedListView::slotReset()" << endl;
  //clear() does not remove columns
  clear();
  m_selectedEntries.clear();
//  while(columns() > 0) {
//    removeColumn(0);
//  }
  delete m_filter;
  m_filter = 0;
}

void DetailedListView::addEntry(Data::Entry* entry_) {
  if(!entry_) {
    kdWarning() << "DetailedListView::addEntry() - null entry pointer!" << endl;
    return;
  }

  if(m_entryPix.isNull()) {
    m_entryPix = KGlobal::iconLoader()->loadIcon(entry_->collection()->entryName(), KIcon::User);
  }

//  kdDebug() << "DetailedListView::addEntry() - " << entry_->title() << endl;

  EntryItem* item = new EntryItem(this, entry_);

  populateItem(item);
  bool match = true;
  if(m_filter) {
    match = m_filter->matches(entry_);
    item->setVisible(match);
  }

  if(isUpdatesEnabled() && match) {
    sort();
    ensureItemVisible(item);
    setCurrentItem(item);
    if(m_selectedEntries.count() > 0) {
      blockSignals(true);
      clearSelection();
      setSelected(item, true);
      blockSignals(false);
    }
  }
}

void DetailedListView::modifyEntry(Data::Entry* entry_) {
  if(!entry_) {
    kdWarning() << "DetailedListView::modifyEntry() - null entry pointer!" << endl;
    return;
  }

//  kdDebug() << "DetailedListView::modifyEntry() - " << entry_->title() << endl;

  EntryItem* item = locateItem(entry_);
  if(item) {
    populateItem(item);
    bool match = true;
    if(m_filter) {
      match = m_filter->matches(entry_);
      item->setVisible(match);
    }

    if(isUpdatesEnabled() && match) {
      sort();
      setCurrentItem(item);
      ensureItemVisible(item);
    }

    if(!item->isSelected() && m_selectedEntries.count() > 0) {
      blockSignals(true);
      clearSelection();
      setSelected(item, true);
      blockSignals(false);
    }
  } else {
    kdWarning() << "DetailedListView::modifyEntry() - no item found for " << entry_->title() << endl;
    return;
  }
}

void DetailedListView::removeEntry(Data::Entry* entry_) {
  if(!entry_) {
    kdWarning() << "DetailedListView::removeEntry() - null entry pointer!" << endl;
    return;
  }

  kdDebug() << "DetailedListView::removeEntry() - " << entry_->title() << endl;

  clearSelection();
  delete locateItem(entry_);
}

// TODO: more efficient way to do this?
void DetailedListView::removeCollection(Bookcase::Data::Collection* coll_) {
  if(!coll_) {
    kdWarning() << "DetailedListView::removeCollection() - null coll pointer!" << endl;
    return;
  }

//  kdDebug() << "DetailedListView::removeCollection() - " << coll_->title() << endl;

  saveConfig(coll_);

  clear();
  while(columns() > 0) {
    removeColumn(0);
  }

  m_headerMenu->clear();
  m_headerMenu->insertTitle(i18n("View Columns"));

  m_columnWidths.clear();
  m_isNumber.clear();
  m_entryPix = QPixmap();

  m_selectedEntries.clear();

  // clear the filter, too
  delete m_filter;
  m_filter = 0;
}

void DetailedListView::populateColumn(int col_) {
  if(childCount() == 0) {
    return;
  }
//  kdDebug() << "DetailedListView::populateColumn() - " << columnText(col_) << endl;
  EntryItem* item = static_cast<EntryItem*>(firstChild());
  Data::Field* field = item->entry()->collection()->fieldByTitle(columnText(col_));
  for( ; item; item = static_cast<EntryItem*>(item->nextSibling())) {
    setPixmapAndText(item, col_, field);
  }
  m_isDirty[col_] = false;
}

void DetailedListView::populateItem(EntryItem* item_) {
  const Data::Entry* entry = item_->entry();
  if(!entry) {
    return;
  }

  for(int colNum = 0; colNum < columns(); ++colNum) {
    if(columnWidth(colNum) > 0) {
      Data::Field* field = entry->collection()->fieldByTitle(columnText(colNum));
      if(!field) {
        kdWarning() << "DetailedListView::populateItem() - no field found for " << columnText(colNum) << endl;
        return;
      }
      setPixmapAndText(item_, colNum, field);
    } else {
      m_isDirty[colNum] = true;
    }
  }
}

void DetailedListView::slotRMB(QListViewItem* item_, const QPoint& point_, int) {
  if(item_ && m_itemMenu->count() > 0) {
//    setSelected(item_, true);
    m_itemMenu->popup(point_);
  }
}

void DetailedListView::slotSelectionChanged() {
//  kdDebug() << "DetailedListView::slotSelectionChanged()" << endl;
  // all items in the listview are entryItems
  EntryItem* item;
  for(QListViewItemIterator it(this); it.current(); ++it) {
    item = static_cast<EntryItem*>(it.current());
    if(item->isSelected()) {
      if(item->entry() && !m_selectedEntries.containsRef(item->entry())) {
        // the reason I do it this way is because I want the first item in the list
        // to be the first item that was selected, so the edit widget can fill its
        // contents with the first entry
//        kdDebug() << "DetailedListView::slotSelectionChanged() - adding " << item->entry()->title() << endl;
        m_selectedEntries.append(item->entry());;
      }
    } else { // it's ok to call this even when entry not in list
      m_selectedEntries.removeRef(item->entry());
    }
  }
  
  emit signalEntrySelected(this, m_selectedEntries);
}

// don't shadow QListView::setSelected
void DetailedListView::setEntrySelected(Data::Entry* entry_) {
//  kdDebug() << "DetailedListView::setEntrySelected()" << endl;
  // if entry_ is null pointer, just return
  if(!entry_) {
    return;
  }

  EntryItem* item = locateItem(entry_);

  blockSignals(true);
  setSelected(item, true);
  setCurrentItem(item);
  blockSignals(false);
  ensureItemVisible(item);
}

Bookcase::EntryItem* const DetailedListView::locateItem(const Data::Entry* entry_) {
  for(QListViewItemIterator it(this); it.current(); ++it) {
    EntryItem* item = static_cast<EntryItem*>(it.current());
    if(item->entry() == entry_) {
      return item;
    }
  }

  return 0;
}

void DetailedListView::clearSelection() {
//  kdDebug() << "DetailedListView::clearSelection()" << endl;
  blockSignals(true);
  if(!m_selectedEntries.isEmpty()) {
    selectAll(false);
  }
  setCurrentItem(0);
  blockSignals(false);
  m_selectedEntries.clear();
}

bool DetailedListView::eventFilter(QObject* obj_, QEvent* ev_) {
  if(ev_->type() == QEvent::MouseButtonPress
      && static_cast<QMouseEvent*>(ev_)->button() == Qt::RightButton
      && obj_->isA("QHeader")) {
    m_headerMenu->popup(static_cast<QMouseEvent*>(ev_)->globalPos());
    return true;
  }
  return KListView::eventFilter(obj_, ev_);
}

void DetailedListView::slotHeaderMenuActivated(int id_) {
//  kdDebug() << "DetailedListView::slotHeaderMenuActivated() - " << m_headerMenu->text(id_) << endl;
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

void DetailedListView::slotRefresh() {
  if(childCount() == 0) {
    return;
  }

  // the algorithm here is to iterate over each column, then over every list item
  Data::Collection* coll = static_cast<EntryItem*>(firstChild())->entry()->collection();
  Data::Field* field;
  EntryItem* item;

  for(int colNum = 0; colNum < columns(); ++colNum) {
    field = coll->fieldByTitle(columnText(colNum));

    // iterate over all items
    
    for(QListViewItemIterator it(this); it.current(); ++it) {
      item = static_cast<EntryItem*>(it.current());
      
      setPixmapAndText(item, colNum, field);
      
      // if we're doing this for the first time, go ahead and pass through filter
      if(colNum == 0) {
        if(m_filter && !m_filter->matches(item->entry())) {
          item->setVisible(false);
        } else {
          item->setVisible(true);
        }
      }
    }
  }
}

void DetailedListView::setPixmapAndText(EntryItem* item_, int col_, Data::Field* field_) {
  if(!item_) {
    return;
  }

  // if the bool is not empty, show the checkmark pixmap
  if(field_->type() == Data::Field::Bool) {
    QString value = item_->entry()->field(field_->name());
    item_->setPixmap(col_, value.isEmpty() ? QPixmap() : m_checkPix);
    item_->setText(col_, QString::null);
  } else { // for everything else, there's no pixmap, unless it's the first column
    if(col_ == m_firstSection) {
      item_->setPixmap(col_, m_entryPix);
    } else {
      item_->setPixmap(col_, QPixmap());
    }
    QString value = item_->entry()->formattedField(field_->name());
    item_->setText(col_, value);
  }
}

void DetailedListView::showColumn(int col_) {
//  kdDebug() << "DetailedListView::showColumn() - " << columnText(col_) << endl;
  int w = m_columnWidths[col_]; // this should be safe - all were initialized to 0
  if(w == 0) {
    setColumnWidthMode(col_, QListView::Maximum);
    for(QListViewItemIterator it(this); it.current(); ++it) {
      w = QMAX(it.current()->width(fontMetrics(), this, col_), w);
    }
    w = QMAX(w, MIN_COL_WIDTH);
  } else {
    setColumnWidthMode(col_, QListView::Manual);
  }

  if(m_isDirty[col_]) {
    populateColumn(col_);
  }
  setColumnWidth(col_, w);
  header()->setResizeEnabled(true, col_);
  triggerUpdate();
}

void DetailedListView::hideColumn(int col_) {
//  kdDebug() << "DetailedListView::hideColumn() - " << columnText(col_) << endl;
  setColumnWidthMode(col_, QListView::Manual);
  setColumnWidth(col_, 0);
  header()->setResizeEnabled(false, col_);
  triggerUpdate();
}

void DetailedListView::slotCacheColumnWidth(int section_, int oldSize_, int newSize_) {
  // if the old size was 0, update the menu
  if(oldSize_ == 0 && newSize_ > 0) {
    m_headerMenu->setItemChecked(m_headerMenu->idAt(section_+1), true); // add 1 for title item
  }

  m_columnWidths[section_] = newSize_;
  setColumnWidthMode(section_, QListView::Manual);
}

void DetailedListView::setFilter(const Filter* filter_) {
  delete m_filter;
  m_filter = filter_;

//  clearSelection();

  // iterate over all items
  EntryItem* item;
  for(QListViewItemIterator it(this); it.current(); ++it) {
    item = static_cast<EntryItem*>(it.current());
    if(m_filter && !m_filter->matches(item->entry())) {
      item->setVisible(false);
    } else {
      item->setVisible(true);
    }
  }
}

const Bookcase::Filter* DetailedListView::filter() const {
  return m_filter;
}

void DetailedListView::addField(Data::Field* field_, int width_) {
//  kdDebug() << "DetailedListView::slotAddColumn() - " << field_->title() << endl;
  int col = addColumn(field_->title());

  // bools have a checkmark pixmap, so center it. Numbers too
  if(field_->type() == Data::Field::Bool || field_->type() == Data::Field::Number) {
    setColumnAlignment(col, Qt::AlignHCenter);
  }

  // width might be -1, which means set the width to maximum
  // but m_columnWidths is the cached width, so just set it to 0
  m_columnWidths.push_back(QMAX(width_, 0));
  
  m_isNumber.push_back(field_->type() == Data::Field::Number);
  m_isDirty.push_back(true);
  
  int id = m_headerMenu->insertItem(field_->title());
  if(width_ == 0) {
    m_headerMenu->setItemChecked(id, false);
    hideColumn(col);
  } else {
    m_headerMenu->setItemChecked(id, true);
    showColumn(col);
  }
}

void DetailedListView::modifyField(Data::Field* newField_, Data::Field* oldField_) {
  int sec; // I need it for after the loop
  for(sec = 0; sec < columns(); ++sec) {
    if(header()->label(sec) == oldField_->title()) {
      break;
    }
  }

  // I thought this would have to be mapped to index, but not the case
  setColumnText(sec, newField_->title());
  m_isNumber[sec] = (newField_->type() == Data::Field::Number);
  if(newField_->type() == Data::Field::Bool || newField_->type() == Data::Field::Number) {
    setColumnAlignment(sec, Qt::AlignHCenter);
  } else {
    setColumnAlignment(sec, Qt::AlignLeft);
  }
  m_headerMenu->changeItem(m_headerMenu->idAt(sec+1), newField_->title()); // add 1 since menu has title
}

void DetailedListView::removeField(Data::Field* field_) {
//  kdDebug() << "DetailedListView::removeField() - " << field_->name() << endl;

  int sec; // I need it for after the loop
  for(sec = 0; sec < columns(); ++sec) {
    if(header()->label(sec) == field_->title()) {
//      kdDebug() << "Removing section " << sec << endl;
      break;
    }
  }

  if(sec == columns()) {
    kdWarning() << "DetailedListView::slotRemoveColumn() - no column named " << field_->title() << endl;
    return;
  }

  m_headerMenu->removeItem(m_headerMenu->idAt(sec+1)); // add 1 since menu has title

  m_columnWidths.erase(&m_columnWidths[sec]);
  m_isNumber.erase(&m_isNumber[sec]);
  m_isDirty.erase(&m_isDirty[sec]);

  // I thought this would have to be mapped to index, but not the case
  removeColumn(sec);

  // sometimes resizeEnabled gets messed up
  for(int i = sec; i < columns(); ++i) {
    header()->setResizeEnabled(columnWidth(i) > 0, header()->mapToSection(i));
  }
  slotUpdatePixmap();
  triggerUpdate();
}

void DetailedListView::reorderFields(const Data::FieldList& list_) {
//  kdDebug() << "DetailedListView::reorderFields()" << endl;
  // find the first out of place field
  int sec = 0;
  Data::FieldListIterator it(list_);
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
    if(it.current()->type() == Data::Field::Bool || it.current()->type() == Data::Field::Number) {
      setColumnAlignment(header()->mapToIndex(sec), Qt::AlignHCenter);
    } else {
      setColumnAlignment(header()->mapToIndex(sec), Qt::AlignLeft);
    }
    m_isNumber[sec] = (it.current()->type() == Data::Field::Number);

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

int DetailedListView::prevSortedColumn() const {
  return m_prevSortColumn;
}

int DetailedListView::prev2SortedColumn() const {
  return m_prev2SortColumn;
}

void DetailedListView::setPrevSortedColumn(int prev1_, int prev2_) {
  m_prevSortColumn = prev1_;
  m_prev2SortColumn = prev2_;
}

void DetailedListView::setSorting(int column_, bool ascending_/*=true*/) {
  if(column_ != columnSorted()) {
    m_prev2SortColumn = m_prevSortColumn;
    m_prevSortColumn = columnSorted();
  }
  KListView::setSorting(column_, ascending_);
}

// it's possible to have a zero-length vector and have this called, so check bounds
bool DetailedListView::isNumber(int column_) const {
  return column_ < static_cast<int>(m_isNumber.size()) && m_isNumber[column_];
}

void DetailedListView::updateFirstSection() {
  for(int numCol = 0; numCol < columns(); ++numCol) {
    if(columnWidth(numCol) > 0) {
      m_firstSection = header()->mapToSection(numCol);
      return;
    }
  }
}

void DetailedListView::slotUpdatePixmap() {
  int oldSection = m_firstSection;
  updateFirstSection();
  if(oldSection == m_firstSection) {
    return;
  }

  for(QListViewItemIterator it(this); it.current(); ++it) {
    it.current()->setPixmap(oldSection, QPixmap());
    it.current()->setPixmap(m_firstSection, m_entryPix);
  }
}

void DetailedListView::saveConfig(Bookcase::Data::Collection* coll_) {
  KConfig* config = kapp->config();
  config->setGroup(QString::fromLatin1("Options - %1").arg(coll_->entryName()));

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
    colNames += coll_->fieldNameByTitle(columnText(col));
  }
  config->writeEntry("ColumnNames", colNames);
}

QString DetailedListView::sortColumnTitle1() const {
  return columnSorted() == -1 ? QString::null : header()->label(columnSorted());
}

QString DetailedListView::sortColumnTitle2() const {
  return prevSortedColumn() == -1 ? QString::null : header()->label(prevSortedColumn());
}

QString DetailedListView::sortColumnTitle3() const {
  return prev2SortedColumn() == -1 ? QString::null : header()->label(prev2SortedColumn());
}

QStringList DetailedListView::visibleColumns() const {
  QStringList titles;
  for(int i = 0; i < columns(); ++i) {
    if(columnWidth(header()->mapToSection(i)) > 0) {
      titles << columnText(header()->mapToSection(i));
    }
  }
  return titles;
}

Bookcase::Data::EntryList DetailedListView::visibleEntries() {
  // We could just return the full collection entry list if the filter is 0
  // but printing depends on the sorted order
  Data::EntryList list;
#if QT_VERSION >= 0x030200
  QListViewItemIterator it(this, QListViewItemIterator::Visible);
#else
  QListViewItemIterator it(this);
#endif
  for( ; it.current(); ++it) {
#if QT_VERSION < 0x030200
    if(it.current()->isVisible()) {
#endif
      list.append(static_cast<EntryItem*>(it.current())->entry());
#if QT_VERSION < 0x030200
    }
#endif
  }
  return list;
}

void DetailedListView::selectAllVisible() {
  blockSignals(true);
#if QT_VERSION >= 0x030200
  QListViewItemIterator it(this, QListViewItemIterator::Visible);
#else
  QListViewItemIterator it(this);
#endif
  for( ; it.current(); ++it) {
#if QT_VERSION < 0x030200
    if(it.current()->isVisible()) {
#endif
      setSelected(it.current(), true);
#if QT_VERSION < 0x030200
    }
#endif
  }
  blockSignals(false);
  slotSelectionChanged();
}

