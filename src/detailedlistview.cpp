/***************************************************************************
    copyright            : (C) 2001-2006 by Robby Stephenson
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
#include "detailedentryitem.h"
#include "collection.h"
#include "imagefactory.h"
#include "controller.h"
#include "field.h"
#include "entry.h"
#include "gui/ratingwidget.h"
#include "tellico_debug.h"
#include "tellico_kernel.h"
#include "core/tellico_config.h"

#include <klocale.h>
#include <kconfig.h>
#include <kapplication.h>
#include <kaction.h>
#include <kiconloader.h>
#include <kpopupmenu.h>

#include <qptrlist.h>
#include <qstringlist.h>
#include <qvaluelist.h>
#include <qheader.h>

namespace {
  static const int MIN_COL_WIDTH = 50;
}

using Tellico::DetailedListView;

DetailedListView::DetailedListView(QWidget* parent_, const char* name_/*=0*/)
    : GUI::ListView(parent_, name_), m_filter(0),
    m_prevSortColumn(-1), m_prev2SortColumn(-1), m_firstSection(-1),
    m_pixWidth(50), m_pixHeight(50) {
//  myDebug() << "DetailedListView()" << endl;
  setAllColumnsShowFocus(true);
  setShowSortIndicator(true);
  setShadeSortColumn(true);

//  connect(this, SIGNAL(selectionChanged()), SLOT(slotSelectionChanged()));
  connect(this, SIGNAL(contextMenuRequested(QListViewItem*, const QPoint&, int)),
          SLOT(contextMenuRequested(QListViewItem*, const QPoint&, int)));
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

  m_checkPix = UserIcon(QString::fromLatin1("checkmark"));
}

DetailedListView::~DetailedListView() {
}

void DetailedListView::addCollection(Data::CollPtr coll_) {
  if(!coll_) {
    return;
  }

  m_imageColumns.clear();
//  myDebug() << "DetailedListView::addCollection()" << endl;

  KConfigGroup config(kapp->config(), QString::fromLatin1("Options - %1").arg(coll_->typeName()));

  QString configN;
  if(coll_->type() == Data::Collection::Base) {
    KURL url = Kernel::self()->URL();
    for(uint i = 0; i < Config::maxCustomURLSettings(); ++i) {
      KURL u = config.readEntry(QString::fromLatin1("URL_%1").arg(i));
      if(u == url) {
        configN = QString::fromLatin1("_%1").arg(i);
        break;
      }
    }
  }

  QStringList colNames = config.readListEntry("ColumnNames" + configN);
  if(colNames.isEmpty()) {
    colNames = QString::fromLatin1("title");
  }

  // this block compensates for the chance that the user added a field and it wasn't
  // written to the widths. Also compensates for 0.5.x to 0.6.x column layout changes
  QValueList<int> colWidths = config.readIntListEntry("ColumnWidths" + configN);
  if(colWidths.empty()) {
    colWidths.insert(colWidths.begin(), colNames.count(), -1); // automatic width
  }

  QValueList<int> colOrder = config.readIntListEntry("ColumnOrder" + configN);

  // need to remove values for fields which don't exist in the current collection
  QStringList newCols;
  QValueList<int> newWidths, removeCols;
  for(uint i = 0; i < colNames.count(); ++i) {
    if(!colNames[i].isEmpty() && coll_->hasField(colNames[i])) {
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
      for(uint j = 0; j < removeCols.count() && removeCols[j] < i; ++j) {
        --i;
      }
      newOrder += i;
    }
  }
  colOrder = newOrder;

  bool none = true;
  Data::FieldVec fields = coll_->fields();
  for(Data::FieldVec::Iterator fIt = fields.begin(); fIt != fields.end(); ++fIt) {
    if(colNames.findIndex(fIt->name()) > -1 && colWidths.count() > 0) {
      addField(fIt, colWidths.front());
      if(none && colWidths.front() != 0) {
        none = false;
      }
      colWidths.pop_front();
    } else {
      addField(fIt, 0);
    }
  }
  if(none && columns() > 0 && !colNames.isEmpty()) {
    showColumn(coll_->fieldNames().findIndex(colNames[0]));
  }

  QValueList<int>::ConstIterator it = colOrder.begin();
  for(int i = 0; it != colOrder.end(); ++it) {
    header()->moveSection(i++, *it);
  }
  slotUpdatePixmap();

  int sortCol = config.readNumEntry("SortColumn" + configN, 0);
  bool sortAsc = config.readBoolEntry("SortAscending" + configN, true);
  setSorting(sortCol, sortAsc);
  int prevSortCol = config.readNumEntry("PrevSortColumn" + configN, -1);
  int prev2SortCol = config.readNumEntry("Prev2SortColumn" + configN, -1);
  setPrevSortedColumn(prevSortCol, prev2SortCol);

  triggerUpdate();
  kapp->processEvents();
  setUpdatesEnabled(false);

  m_loadingCollection = true;
  Data::EntryVec entries = coll_->entries();
  for(Data::EntryVecIt entry = entries.begin(); entry != entries.end(); ++entry) {
    addEntryInternal(entry);
  }
  m_loadingCollection = false;

  setUpdatesEnabled(true);
  triggerUpdate();
}

void DetailedListView::slotReset() {
//  myDebug() << "DetailedListView::slotReset()" << endl;
  //clear() does not remove columns
  clear();
//  while(columns() > 0) {
//    removeColumn(0);
//  }
  m_filter = 0;
}

void DetailedListView::addEntries(Data::EntryVec entries_) {
  if(entries_.isEmpty()) {
    return;
  }

//  myDebug() << "DetailedListView::addEntry() - " << entry_->title() << endl;

  DetailedEntryItem* item = 0;
  for(Data::EntryVecIt entry = entries_.begin(); entry != entries_.end(); ++entry) {
    item = addEntryInternal(entry);
    item->setState(DetailedEntryItem::New);
    item->setVisible(!m_filter || m_filter->matches(entry.data()));
  }

  if(isUpdatesEnabled() && item && item->isVisible()) {
    sort();
    ensureItemVisible(item);
    setCurrentItem(item);
    if(!selectedItems().isEmpty()) {
      blockSignals(true);
      clearSelection();
      setSelected(item, true);
      blockSignals(false);
    }
  } else {
    triggerUpdate();
  }
}

Tellico::DetailedEntryItem* DetailedListView::addEntryInternal(Data::EntryPtr entry_) {
  if(m_entryPix.isNull()) {
    m_entryPix = UserIcon(entry_->collection()->typeName());
    if(m_entryPix.isNull()) {
      kdWarning() << "DetailedListView::addEntryInternal() - can't find entry pix" << endl;
    }
  }

  DetailedEntryItem* item = new DetailedEntryItem(this, entry_);
  populateItem(item);
  return item;
}

void DetailedListView::modifyEntries(Data::EntryVec entries_) {
  if(entries_.isEmpty()) {
    return;
  }

  DetailedEntryItem* item = 0;
  for(Data::EntryVecIt entry = entries_.begin(); entry != entries_.end(); ++entry) {
    item = locateItem(entry.data());
    if(!item) {
      kdWarning() << "DetailedListView::modifyEntries() - no item found for " << entry->title() << endl;
      continue;
    }

    populateItem(item);
    item->setState(DetailedEntryItem::Modified);
    item->setVisible(!m_filter || m_filter->matches(entry.data()));
  }

  if(isUpdatesEnabled() && item && item->isVisible()) {
    sort();
  }

  if(item && !item->isSelected() && !selectedItems().isEmpty()) {
    blockSignals(true);
    clearSelection();
    setSelected(item, true);
    blockSignals(false);
  }
}

void DetailedListView::removeEntries(Data::EntryVec entries_) {
  if(entries_.isEmpty()) {
    return;
  }

//  myDebug() << "DetailedListView::removeEntries() - " << entry_->title() << endl;

  for(Data::EntryVecIt entry = entries_.begin(); entry != entries_.end(); ++entry) {
    delete locateItem(entry);
  }
  // update is required
  triggerUpdate();
}

void DetailedListView::removeCollection(Data::CollPtr coll_) {
  if(!coll_) {
    kdWarning() << "DetailedListView::removeCollection() - null coll pointer!" << endl;
    return;
  }

//  myDebug() << "DetailedListView::removeCollection() - " << coll_->title() << endl;

  clear();
  while(columns() > 0) {
    removeColumn(0);
  }

  m_headerMenu->clear();
  m_headerMenu->insertTitle(i18n("View Columns"));

  m_columnWidths.clear();
  clearComparisons();
  m_entryPix = QPixmap();

  // clear the filter, too
  m_filter = 0;
}

void DetailedListView::populateColumn(int col_) {
  if(childCount() == 0) {
    return;
  }
//  myDebug() << "DetailedListView::populateColumn() - " << columnText(col_) << endl;
  DetailedEntryItem* item = static_cast<DetailedEntryItem*>(firstChild());
  Data::FieldPtr field = item->entry()->collection()->fieldByTitle(columnText(col_));
  for( ; item; item = static_cast<DetailedEntryItem*>(item->nextSibling())) {
    setPixmapAndText(item, col_, field);
  }
  m_isDirty[col_] = false;
}

void DetailedListView::populateItem(DetailedEntryItem* item_) {
  Data::EntryPtr entry = item_->entry();
  if(!entry) {
    return;
  }

  for(int colNum = 0; colNum < columns(); ++colNum) {
    if(columnWidth(colNum) > 0) {
      Data::FieldPtr field = entry->collection()->fieldByTitle(columnText(colNum));
      if(!field) {
        kdWarning() << "DetailedListView::populateItem() - no field found for " << columnText(colNum) << endl;
        continue;
      }
      setPixmapAndText(item_, colNum, field);
    } else {
      m_isDirty[colNum] = true;
    }
  }
}

void DetailedListView::contextMenuRequested(QListViewItem* item_, const QPoint& point_, int) {
  if(!item_) {
    return;
  }
  KPopupMenu menu(this);
  Controller::self()->plugEntryActions(&menu);
  menu.exec(point_);
}

// don't shadow QListView::setSelected
void DetailedListView::setEntrySelected(Data::EntryPtr entry_) {
//  myDebug() << "DetailedListView::setEntrySelected()" << endl;
  // if entry_ is null pointer, just return
  if(!entry_) {
    return;
  }

  DetailedEntryItem* item = locateItem(entry_);

  blockSignals(true);
  clearSelection();
  setSelected(item, true);
  setCurrentItem(item);
  blockSignals(false);
  ensureItemVisible(item);
}

Tellico::DetailedEntryItem* DetailedListView::locateItem(Data::EntryPtr entry_) {
  for(QListViewItemIterator it(this); it.current(); ++it) {
    DetailedEntryItem* item = static_cast<DetailedEntryItem*>(it.current());
    if(item->entry() == entry_) {
      return item;
    }
  }

  return 0;
}

bool DetailedListView::eventFilter(QObject* obj_, QEvent* ev_) {
  if(ev_->type() == QEvent::MouseButtonPress
      && static_cast<QMouseEvent*>(ev_)->button() == Qt::RightButton
      && obj_ == header()) {
    m_headerMenu->popup(static_cast<QMouseEvent*>(ev_)->globalPos());
    return true;
  }
  return GUI::ListView::eventFilter(obj_, ev_);
}

void DetailedListView::slotHeaderMenuActivated(int id_) {
//  myDebug() << "DetailedListView::slotHeaderMenuActivated() - " << m_headerMenu->text(id_) << endl;
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
  Data::CollPtr coll = static_cast<DetailedEntryItem*>(firstChild())->entry()->collection();
  Data::FieldPtr field;
  DetailedEntryItem* item;

  for(int colNum = 0; colNum < columns(); ++colNum) {
    field = coll->fieldByTitle(columnText(colNum));

    // iterate over all items

    for(QListViewItemIterator it(this); it.current(); ++it) {
      item = static_cast<DetailedEntryItem*>(it.current());

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

void DetailedListView::slotRefreshImages() {
  for(QValueVector<int>::const_iterator it = m_imageColumns.begin(); it != m_imageColumns.end(); ++it) {
    if(columnWidth(*it) > 0) {
      populateColumn(*it);
    }
  }
}

void DetailedListView::setPixmapAndText(DetailedEntryItem* item_, int col_, Data::FieldPtr field_) {
  if(!item_ || !field_) {
    return;
  }

  // if the bool is not empty, show the checkmark pixmap
  if(field_->type() == Data::Field::Bool) {
    const QString value = item_->entry()->field(field_);
    item_->setPixmap(col_, value.isEmpty() ? QPixmap() : m_checkPix);
    item_->setText(col_, QString::null);
  } else if(field_->type() == Data::Field::Image) {
    // if we're currently loading a collection
    // don't load the image just yet, it'll get refreshed later
    if(m_loadingCollection || columnWidth(col_) == 0) {
      item_->setPixmap(col_, QPixmap());
      item_->setText(col_, QString::null);
    } else {
      item_->setPixmap(col_, ImageFactory::pixmap(item_->entry()->field(field_), m_pixWidth, m_pixHeight));
      item_->setText(col_, QString::null);
    }
  } else if(field_->type() == Data::Field::Rating) {
    item_->setPixmap(col_, GUI::RatingWidget::pixmap(item_->entry()->field(field_)));
    item_->setText(col_, QString::null);
  } else { // for everything else, there's no pixmap, unless it's the first column
    item_->setPixmap(col_, col_ == m_firstSection ? m_entryPix : QPixmap());
    item_->setText(col_, item_->entry()->formattedField(field_));
  }
  item_->widthChanged(col_);
}

void DetailedListView::showColumn(int col_) {
//  myDebug() << "DetailedListView::showColumn() - " << columnText(col_) << endl;
  int w = m_columnWidths[col_]; // this should be safe - all were initialized to 0
  if(w == 0) {
    setColumnWidthMode(col_, QListView::Maximum);
    for(QListViewItemIterator it(this); it.current(); ++it) {
      w = KMAX(it.current()->width(fontMetrics(), this, col_), w);
    }
    w = KMAX(w, MIN_COL_WIDTH);
  } else {
    setColumnWidthMode(col_, QListView::Manual);
  }

  setColumnWidth(col_, w);
  if(m_isDirty[col_]) {
    populateColumn(col_);
  }
  header()->setResizeEnabled(true, col_);
  triggerUpdate();
}

void DetailedListView::hideColumn(int col_) {
//  myDebug() << "DetailedListView::hideColumn() - " << columnText(col_) << endl;
  setColumnWidthMode(col_, QListView::Manual);
  setColumnWidth(col_, 0);
  header()->setResizeEnabled(false, col_);

  // special case for images, I don't want all the items to be tall, so remove pixmaps
  if(childCount() > 0) {
    Data::EntryPtr entry = static_cast<DetailedEntryItem*>(firstChild())->entry();
    if(entry) {
      Data::FieldPtr field = entry->collection()->fieldByTitle(columnText(col_));
      if(field && field->type() == Data::Field::Image) {
        m_isDirty[col_] = true;
        for(QListViewItemIterator it(this); it.current(); ++it) {
          it.current()->setPixmap(col_, QPixmap());
        }
      }
    }
  }

  triggerUpdate();
}

void DetailedListView::slotCacheColumnWidth(int section_, int oldSize_, int newSize_) {
  // if the old size was 0, update the menu
  if(oldSize_ == 0 && newSize_ > 0) {
    m_headerMenu->setItemChecked(m_headerMenu->idAt(section_+1), true); // add 1 for title item
  }

  if(newSize_ > 0) {
    m_columnWidths[section_] = newSize_;
  }
  setColumnWidthMode(section_, QListView::Manual);
}

void DetailedListView::setFilter(FilterPtr filter_) {
  if(m_filter.data() != filter_) { // might just be updating
    m_filter = filter_;
  }
//  clearSelection();

  int count = 0;
  // iterate over all items
  DetailedEntryItem* item;
  for(QListViewItemIterator it(this); it.current(); ++it) {
    item = static_cast<DetailedEntryItem*>(it.current());
    if(m_filter && !m_filter->matches(item->entry())) {
      item->setVisible(false);
      setSelected(item, false);
    } else {
      item->setVisible(true);
      ++count;
    }
  }
  m_visibleItems = count;
}

void DetailedListView::addField(Data::CollPtr, Data::FieldPtr field) {
  addField(field, 0); /* field is hidden by default */
}

void DetailedListView::addField(Data::FieldPtr field_, int width_) {
//  myDebug() << "DetailedListView::addField() - " << field_->title() << endl;
  int col = addColumn(field_->title());

  // Bools, images, and numbers should be centered
  if(field_->type() == Data::Field::Bool
     || field_->type() == Data::Field::Number
     || field_->type() == Data::Field::Image) {
    setColumnAlignment(col, Qt::AlignHCenter | Qt::AlignVCenter);
    if(field_->type() == Data::Field::Image) {
      m_imageColumns.push_back(col);
    }
  } else {
    setColumnAlignment(col, Qt::AlignLeft | Qt::AlignVCenter);
  }

  // width might be -1, which means set the width to maximum
  // but m_columnWidths is the cached width, so just set it to 0
  m_columnWidths.push_back(KMAX(width_, 0));

  m_isDirty.push_back(true);

  int id = m_headerMenu->insertItem(field_->title());
  if(width_ == 0) {
    m_headerMenu->setItemChecked(id, false);
    hideColumn(col);
  } else {
    m_headerMenu->setItemChecked(id, true);
    showColumn(col);
  }
  setComparison(col, ListViewComparison::create(field_));
  resetComparisons();
}

void DetailedListView::modifyField(Tellico::Data::CollPtr, Data::FieldPtr oldField_, Data::FieldPtr newField_) {
  int sec; // I need it for after the loop
  for(sec = 0; sec < columns(); ++sec) {
    if(header()->label(sec) == oldField_->title()) {
      break;
    }
  }

  // I thought this would have to be mapped to index, but not the case
  setColumnText(sec, newField_->title());
  if(newField_->type() == Data::Field::Bool
     || newField_->type() == Data::Field::Number
     || newField_->type() == Data::Field::Image) {
    setColumnAlignment(sec, Qt::AlignHCenter | Qt::AlignVCenter);
    if(oldField_->type() == Data::Field::Image) {
      QValueVector<int>::iterator it = qFind(m_imageColumns.begin(), m_imageColumns.end(), sec);
      if(it != m_imageColumns.end()) {
        m_imageColumns.erase(it);
      }
    }
    if(newField_->type() == Data::Field::Image) {
      m_imageColumns.push_back(sec);
    }
  } else {
    setColumnAlignment(sec, Qt::AlignLeft | Qt::AlignVCenter);
  }
  m_headerMenu->changeItem(m_headerMenu->idAt(sec+1), newField_->title()); // add 1 since menu has title
  setComparison(sec, ListViewComparison::create(newField_));
  resetComparisons();
}

void DetailedListView::removeField(Tellico::Data::CollPtr, Data::FieldPtr field_) {
//  myDebug() << "DetailedListView::removeField() - " << field_->name() << endl;

  int sec; // I need it for after the loop
  for(sec = 0; sec < columns(); ++sec) {
    if(header()->label(sec) == field_->title()) {
//      myDebug() << "Removing section " << sec << endl;
      break;
    }
  }

  if(sec == columns()) {
    kdWarning() << "DetailedListView::removeField() - no column named " << field_->title() << endl;
    return;
  }

  m_headerMenu->removeItem(m_headerMenu->idAt(sec+1)); // add 1 since menu has title

  m_columnWidths.erase(&m_columnWidths[sec]);
  m_isDirty.erase(&m_isDirty[sec]);

  // I thought this would have to be mapped to index, but not the case
  removeComparison(sec); // must be before removeColumn();
  removeColumn(sec);

  // sometimes resizeEnabled gets messed up
  for(int i = sec; i < columns(); ++i) {
    header()->setResizeEnabled(columnWidth(i) > 0, header()->mapToSection(i));
  }
  resetComparisons();
  slotUpdatePixmap();
  triggerUpdate();
}

void DetailedListView::reorderFields(const Data::FieldVec& fields_) {
//  myDebug() << "DetailedListView::reorderFields()" << endl;
  // find the first out of place field
  int sec = 0;
  Data::FieldVec::ConstIterator it = fields_.begin();
  for(sec = 0; it != fields_.end() && sec < columns(); ++sec, ++it) {
    if(header()->label(sec) != it->title()) {
      break;
    }
  }

  QStringList visible = visibleColumns();
  for( ; it != fields_.end() && sec < columns();  ++sec, ++it) {
    header()->setLabel(sec, it->title());
    bool isVisible = (visible.findIndex(it->title()) > -1);
    m_headerMenu->changeItem(m_headerMenu->idAt(sec+1), it->title());
    m_headerMenu->setItemChecked(m_headerMenu->idAt(sec+1), isVisible);
    m_columnWidths[sec] = 0;
    if(it->type() == Data::Field::Bool
       || it->type() == Data::Field::Number
       || it->type() == Data::Field::Image) {
      setColumnAlignment(sec, Qt::AlignHCenter | Qt::AlignVCenter);
    } else {
      setColumnAlignment(sec, Qt::AlignLeft | Qt::AlignVCenter);
    }

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
//  DEBUG_BLOCK;
  if(column_ != columnSorted()) {
    m_prev2SortColumn = m_prevSortColumn;
    m_prevSortColumn = columnSorted();
  }
  GUI::ListView::setSorting(column_, ascending_);
}

void DetailedListView::updateFirstSection() {
  for(int numCol = 0; numCol < columns(); ++numCol) {
    if(columnWidth(header()->mapToSection(numCol)) > 0) {
      m_firstSection = header()->mapToSection(numCol);
      break;
    }
  }
}

void DetailedListView::slotUpdatePixmap() {
  int oldSection = m_firstSection;
  updateFirstSection();
  if(childCount() == 0 || oldSection == m_firstSection) {
    return;
  }

  Data::EntryPtr entry = static_cast<DetailedEntryItem*>(firstChild())->entry();
  if(!entry) {
    return;
  }

  Data::FieldPtr field1 = entry->collection()->fieldByTitle(columnText(oldSection));
  Data::FieldPtr field2 = entry->collection()->fieldByTitle(columnText(m_firstSection));
  if(!field1 || !field2) {
    kdWarning() << "DetailedListView::slotUpdatePixmap() - no field found." << endl;
    return;
  }

  for(QListViewItemIterator it(this); it.current(); ++it) {
    setPixmapAndText(static_cast<DetailedEntryItem*>(it.current()), oldSection, field1);
    setPixmapAndText(static_cast<DetailedEntryItem*>(it.current()), m_firstSection, field2);
  }
}

void DetailedListView::saveConfig(Tellico::Data::CollPtr coll_, int configIndex_) {
  KConfigGroup config(kapp->config(), QString::fromLatin1("Options - %1").arg(coll_->typeName()));

  // all of this is to have custom settings on a per file basis
  QString configN;
  if(coll_->type() == Data::Collection::Base) {
    QValueList<ConfigInfo> info;
    for(uint i = 0; i < Config::maxCustomURLSettings(); ++i) {
      KURL u = config.readEntry(QString::fromLatin1("URL_%1").arg(i));
      if(!u.isEmpty() && static_cast<int>(i) != configIndex_) {
        configN = QString::fromLatin1("_%1").arg(i);
        ConfigInfo ci;
        ci.cols      = config.readListEntry("ColumnNames" + configN);
        ci.widths    = config.readIntListEntry("ColumnWidths" + configN);
        ci.order     = config.readIntListEntry("ColumnOrder" + configN);
        ci.colSorted = config.readNumEntry("SortColumn" + configN);
        ci.ascSort   = config.readBoolEntry("SortAscending" + configN);
        ci.prevSort  = config.readNumEntry("PrevSortColumn" + configN);
        ci.prev2Sort = config.readNumEntry("Prev2SortColumn" + configN);
        info.append(ci);
      }
    }
    // subtract one since we're writing the current settings, too
    uint limit = QMIN(info.count(), Config::maxCustomURLSettings()-1);
    for(uint i = 0; i < limit; ++i) {
      // starts at one since the current config will be written below
      configN = QString::fromLatin1("_%1").arg(i+1);
      config.writeEntry("ColumnNames"     + configN, info[i].cols);
      config.writeEntry("ColumnWidths"    + configN, info[i].widths);
      config.writeEntry("ColumnOrder"     + configN, info[i].order);
      config.writeEntry("SortColumn"      + configN, info[i].colSorted);
      config.writeEntry("SortAscending"   + configN, info[i].ascSort);
      config.writeEntry("PrevSortColumn"  + configN, info[i].prevSort);
      config.writeEntry("Prev2SortColumn" + configN, info[i].prev2Sort);
    }
    configN = QString::fromLatin1("_0");
  }

  QValueList<int> widths, order;
  for(int i = 0; i < columns(); ++i) {
    if(columnWidthMode(i) == QListView::Manual) {
      widths += columnWidth(i);
    } else {
      widths += -1; // Maximum width mode
    }
    order += header()->mapToIndex(i);
  }

  config.writeEntry("ColumnWidths"    + configN, widths);
  config.writeEntry("ColumnOrder"     + configN, order);
  config.writeEntry("SortColumn"      + configN, columnSorted());
  config.writeEntry("SortAscending"   + configN, ascendingSort());
  config.writeEntry("PrevSortColumn"  + configN, prevSortedColumn());
  config.writeEntry("Prev2SortColumn" + configN, prev2SortedColumn());

  QStringList colNames;
  for(int col = 0; col < columns(); ++col) {
    colNames += coll_->fieldNameByTitle(columnText(col));
  }
  config.writeEntry("ColumnNames" + configN, colNames);
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

// can't be const
Tellico::Data::EntryVec DetailedListView::visibleEntries() {
  // We could just return the full collection entry list if the filter is 0
  // but printing depends on the sorted order
  Data::EntryVec entries;
  for(QListViewItemIterator it(this); it.current(); ++it) {
    if(it.current()->isVisible()) {
      entries.append(static_cast<DetailedEntryItem*>(it.current())->entry());
    }
  }
  return entries;
}

void DetailedListView::selectAllVisible() {
  blockSignals(true);
  for(QListViewItemIterator it(this); it.current(); ++it) {
    if(it.current()->isVisible()) {
      setSelected(it.current(), true);
    }
  }
  blockSignals(false);
  // FIXME: not right with MultiSelectionListView
  slotSelectionChanged();
}

void DetailedListView::resetEntryStatus() {
  for(QListViewItemIterator it(this); it.current(); ++it) {
    static_cast<DetailedEntryItem*>(it.current())->setState(DetailedEntryItem::Normal);
  }
  triggerUpdate();
}

int DetailedListView::compare(int col_, const GUI::ListViewItem* item1_, GUI::ListViewItem* item2_, bool asc_) {
  DetailedEntryItem* item2 = static_cast<DetailedEntryItem*>(item2_);
  int res = 0;
  return (res = compareColumn(col_,              item1_, item2, asc_)) != 0 ? res :
         (res = compareColumn(m_prevSortColumn,  item1_, item2, asc_)) != 0 ? res :
         (res = compareColumn(m_prev2SortColumn, item1_, item2, asc_)) != 0 ? res : 0;
}

int DetailedListView::compareColumn(int col, const GUI::ListViewItem* item1, GUI::ListViewItem* item2, bool asc) {
  return GUI::ListView::compare(col, item1, item2, asc);
}

void DetailedListView::resetComparisons() {
  // this is only allowed when the view is not empty, so we can grab a collection ptr
  if(childCount() == 0) {
    return;
  }
  Data::CollPtr coll = static_cast<DetailedEntryItem*>(firstChild())->entry()->collection();
  if(!coll) {
    return;
  }
  for(int i = 0; i < columns(); ++i) {
    Data::FieldPtr f = coll->fieldByTitle(header()->label(i));
    if(f) {
      setComparison(i, ListViewComparison::create(f));
    }
  }
}

#include "detailedlistview.moc"
