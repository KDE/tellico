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

#include "listview.h"
#include "../controller.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"

#include <kapplication.h>

#include <qpainter.h>
#include <qpixmap.h>
#include <qheader.h>

using Tellico::GUI::ListView;
using Tellico::GUI::ListViewItem;

ListView::ListView(QWidget* parent_, const char* name_) : KListView(parent_, name_/*=0*/),
    m_sortStyle(SortByText), m_isClear(true) {
  setSelectionMode(QListView::Extended);
  connect(this, SIGNAL(selectionChanged()),
          SLOT(slotSelectionChanged()));
  connect(this, SIGNAL(doubleClicked(QListViewItem*)),
          SLOT(slotDoubleClicked(QListViewItem*)));
#if !KDE_IS_VERSION(3,3,90)
  m_shadeSortColumn = false;
  // call it once to initialize
  slotUpdateColors();
#endif
  connect(kapp, SIGNAL(kdisplayPaletteChanged()), SLOT(slotUpdateColors()));
}

void ListView::clearSelection() {
  blockSignals(true);
  if(!m_selectedItems.isEmpty()) {
    selectAll(false);
    m_isClear = true;
  }
  blockSignals(false);
}

void ListView::updateSelected(ListViewItem* item_, bool s_) {
  if(s_) {
    m_selectedItems.append(item_);
  } else {
    m_selectedItems.removeRef(item_);
  }
}

bool ListView::isSelectable(ListViewItem* item_) const {
  // don't allow hidden items to be selected
  if(!item_->isVisible()) {
    return false;
  }

  // selecting multiple items is ok
  // only when parent is open. Be careful to check for existence of parent
  if(item_->parent() && !item_->parent()->isOpen()) {
    return false;
  }

  // just selecting a single item is always ok
  if(m_selectedItems.isEmpty()) {
    return true;
  }

  // not allowed is something other than an entry is selected and current is entry
  if(m_selectedItems.getFirst()->isEntryItem() != item_->isEntryItem()) {
    return false;
  }

  return true;
}

int ListView::firstVisibleColumn() const {
  int col = 0;
  while(col < columns() && columnWidth(header()->mapToSection(col)) == 0) {
    ++col;
  }
  if(col == columns()) {
    return -1;
  }
  return header()->mapToSection(col);
}

int ListView::lastVisibleColumn() const {
  int col = columns()-1;
  while(col < columns() && columnWidth(header()->mapToSection(col)) == 0) {
    --col;
  }
  if(col == columns()) {
    return -1;
  }
  return header()->mapToSection(col);
}

#if !KDE_IS_VERSION(3,3,90)
void ListView::setShadeSortColumn(bool shade_) {
  if(m_shadeSortColumn != shade_) {
    m_shadeSortColumn = shade_;
    repaint();
  }
}
#endif

void ListView::slotUpdateColors() {
#if !KDE_IS_VERSION(3,3,90)
  m_backColor2 = viewport()->colorGroup().base();
  if(m_backColor2 == Qt::black) {
    m_backColor2 = QColor(50, 50, 50);  // dark gray
  } else {
    int h,s,v;
    m_backColor2.hsv(&h, &s, &v);
    if(v > 175) {
      m_backColor2 = m_backColor2.dark(105);
    } else {
      m_backColor2 = m_backColor2.light(120);
    }
  }

  m_altColor2 = alternateBackground();
  if(m_altColor2 == Qt::black) {
    m_altColor2 = QColor(50, 50, 50);  // dark gray
  } else {
    int h,s,v;
    m_altColor2.hsv(&h, &s, &v);
    if(v > 175) {
      m_altColor2 = m_altColor2.dark(105);
    } else {
      m_altColor2 = m_altColor2.light(120);
    }
  }
#endif
  Tellico::updateContrastColor(viewport()->colorGroup());
  repaint();
}

void ListView::slotSelectionChanged() {
  if(m_selectedItems.isEmpty()) {
    if(m_isClear) {
      return; // nothing to do
    }
    m_isClear = true;
    Controller::self()->slotClearSelection();
    return;
  }
  m_isClear = false;

  Data::EntryVec entries;
  // now just find all the children or grandchildren that are entry items
  for(GUI::ListViewItemListIt it(m_selectedItems); it.current(); ++it) {
    Data::EntryVec more = it.current()->entries();
    for(Data::EntryVecIt entry = more.begin(); entry != more.end(); ++entry) {
      if(!entries.contains(entry)) {
        entries.append(entry);
      }
    }
  }
//  Controller::self()->slotUpdateCurrent(entries); // just update current, don't change selection
  Controller::self()->slotUpdateSelection(this, entries);
}

void ListView::slotDoubleClicked(QListViewItem* item_) {
  if(!item_) {
    return;
  }

  // if it has children, just open it
  // but some items delay children creation
  if(static_cast<ListViewItem*>(item_)->realChildCount() > 0) {
    item_->setOpen(!item_->isOpen());
  }

  GUI::ListViewItem* item = static_cast<GUI::ListViewItem*>(item_);
  item->doubleClicked();
}

void ListView::drawContentsOffset(QPainter* p, int ox, int oy, int cx, int cy, int cw, int ch) {
  bool oldUpdatesEnabled = isUpdatesEnabled();
  setUpdatesEnabled(false);
  KListView::drawContentsOffset(p, ox, oy, cx, cy, cw, ch);
  setUpdatesEnabled(oldUpdatesEnabled);
}

/* ****************** ListViewItem ********************* */

ListViewItem::~ListViewItem() {
  // I think there's a bug in qt where the children of this item are deleted after the item itself
  // as a result, there is no listView() pointer for the children, that obvious causes
  // a problem with updating the selection. So we MUST call clear() here ourselves!
  clear();
  // be sure to remove from selected list when it's deleted
  ListView* lv = listView();
  if(lv) {
    lv->updateSelected(this, false);
  }
}

void ListViewItem::clear() {
  QListViewItem* item = firstChild();
  while(item) {
    delete item;
    item = firstChild();
  }
}

int ListViewItem::compare(QListViewItem* item_, int col_, bool asc_) const {
  int res = compareWeight(item_, col_, asc_);
  return res == 0 ? KListViewItem::compare(item_, col_, asc_) : res;
}

int ListViewItem::compareWeight(QListViewItem* item_, int col_, bool asc_) const {
  Q_UNUSED(col_);
  // I want the sorting to be independent of sort order
  GUI::ListViewItem* i = static_cast<GUI::ListViewItem*>(item_);
  int res = 0;
  if(m_sortWeight < i->sortWeight()) {
    res = -1;
  } else if(m_sortWeight > i->sortWeight()) {
    res = 1;
  }
  if(asc_) {
    res *= -1; // reverse, heavier weights will come first always
  }
  return res;
}

void ListViewItem::setSelected(bool s_) {
  ListView* lv = listView();
  if(!lv) {
    return;
  }
  if(s_ && !lv->isSelectable(this)) {
    return;
  }
  if(s_ != isSelected()) {
    lv->updateSelected(this, s_);
    KListViewItem::setSelected(s_);
  }
}

QColor ListViewItem::backgroundColor(int column_) {
#if KDE_IS_VERSION(3,3,90)
  return KListViewItem::backgroundColor(column_);
#else
  ListView* view = listView();
  if(view->columns() > 1 && view->shadeSortColumn() && column_ == view->sortColumn()) {
    return isAlternate() ? view->alternateBackground2() : view->background2();
  }
  return isAlternate() ? view->alternateBackground() : view->viewport()->colorGroup().base();
#endif
}

void ListViewItem::paintCell(QPainter* p_, const QColorGroup& cg_,
                             int column_, int width_, int align_) {
  // taken from klistview.cpp
  // I can't call KListViewItem::paintCell since KListViewItem::backgroundCOlor(int) is
  // not virtual. I need to be sure to call ListViewItem::backgroundColor(int);
  QColorGroup cg = cg_;
  const QPixmap* pm = listView()->viewport()->backgroundPixmap();
  if(pm && !pm->isNull()) {
    cg.setBrush(QColorGroup::Base, QBrush(backgroundColor(column_), *pm));
    QPoint o = p_->brushOrigin();
    p_->setBrushOrigin(o.x()-listView()->contentsX(), o.y()-listView()->contentsY());
  } else {
    cg.setColor(listView()->viewport()->backgroundMode() == Qt::FixedColor ?
                QColorGroup::Background : QColorGroup::Base,
                backgroundColor(column_));
  }

  // don't call KListViewItem::paintCell() since that also does alternate painting, etc...
  QListViewItem::paintCell(p_, cg, column_, width_, align_);

  // borrowed from amarok, draw line to left of cell
  if(!isSelected()) {
    p_->setPen(QPen(listView()->alternateBackground(), 0, Qt::SolidLine));
    p_->drawLine(width_-1, 0, width_-1, height()-1);
  }
}

Tellico::Data::EntryVec ListViewItem::entries() const {
  Data::EntryVec entries;
  for(QListViewItem* child = firstChild(); child; child = child->nextSibling()) {
    Data::EntryVec more = static_cast<GUI::ListViewItem*>(child)->entries();
    for(Data::EntryVecIt entry = more.begin(); entry != more.end(); ++entry) {
      if(!entries.contains(entry)) {
        entries.append(entry);
      }
    }
  }
  return entries;
}

#include "listview.moc"
