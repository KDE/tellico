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

#include "multiselectionlistview.h"

#include <kapplication.h>
#include <kdebug.h>

#include <qpainter.h>
#include <qpixmap.h>

using Tellico::MultiSelectionListView;
using Tellico::MultiSelectionListViewItem;

MultiSelectionListView::MultiSelectionListView(QWidget* parent_, const char* name_)
    : KListView(parent_, name_/*=0*/), m_shadeSortColumn(false) {
  setSelectionMode(QListView::Extended);
  connect(kapp, SIGNAL(kdisplayPaletteChanged()), SLOT(slotUpdateColors()));
  // call it once to initialize
  slotUpdateColors();
}

void MultiSelectionListView::updateSelected(MultiSelectionListViewItem* item_, bool s_) const {
  if(s_) {
    m_selectedItems.append(item_);
  } else {
    m_selectedItems.removeRef(item_);
  }
}

void MultiSelectionListView::setShadeSortColumn(bool shade_) {
  if(m_shadeSortColumn != shade_) {
    m_shadeSortColumn = shade_;
    repaint();
  }
}

void MultiSelectionListView::slotUpdateColors() {
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
  repaint();
}

MultiSelectionListViewItem::~MultiSelectionListViewItem() {
  // be sure to remove from selected list when it's deleted
  MultiSelectionListView* lv = static_cast<MultiSelectionListView*>(listView());
  if(lv) {
    lv->updateSelected(this, false);
  }
}

void MultiSelectionListViewItem::setSelected(bool s_) {
  MultiSelectionListView* lv = static_cast<MultiSelectionListView*>(listView());
  if(s_ && !lv->isSelectable(this)) {
    return;
  }
  lv->updateSelected(this, s_);
  KListViewItem::setSelected(s_);
}

const QColor& MultiSelectionListViewItem::backgroundColor(int column_) {
  MultiSelectionListView* view = static_cast<MultiSelectionListView*>(listView());
  if(view->columns() > 1 && view->shadeSortColumn() && column_ == view->sortColumn()) {
    return isAlternate() ? view->alternateBackground2() : view->background2();
  }
  return isAlternate() ? view->alternateBackground() : view->viewport()->colorGroup().base();
}

void MultiSelectionListViewItem::paintCell(QPainter* p_, const QColorGroup& cg_,
                                           int column_, int width_, int alignment_) {
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
  QListViewItem::paintCell(p_, cg, column_, width_, alignment_);
}

#include "multiselectionlistview.moc"
