/***************************************************************************
                               bcunititem.cpp
                             -------------------
    begin                : Fri Mar 14 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bcunititem.h"
#include "bcgroupview.h"
#include "bccollection.h"

#include <qpainter.h>

QString ParentItem::key(int col_, bool) const {
  bool empty = (text(col_) == BCCollection::emptyGroupName());
  return (col_ == 0 && empty) ? QString::fromLatin1("\t") : text(col_);
}

// if the parent listview is a BCGroupView, column=0, and showCount is true, then
// include and color the number of books.
// Otherwise, just pass the call up the line
void ParentItem::paintCell(QPainter* p_, const QColorGroup& cg_,
                           int column_, int width_, int align_) {
  if(!p_) {
    return;
  }
  
  // always paint the cell
  KListViewItem::paintCell(p_, cg_, column_, width_, align_);

  QListView* lv = listView();
  if(!lv) {
    return;
  }

  // show count is only for first column and depth of 1
  if(lv->isA("BCGroupView") && column_ == 0 && depth() == 1) {
    BCGroupView* groupView = static_cast<BCGroupView*>(lv);

    if(groupView->showCount()) {
      int marg = lv->itemMargin();

      QString numText = QString::fromLatin1(" (");
      numText += QString::number(m_count);
      numText += QString::fromLatin1(")");

      if(isSelected()) {
        p_->setPen(cg_.highlightedText());
      } else {
        //TODO: make configurable
        p_->setPen(QColor("blue"));
      }

      // don't call ParentItem::width() because that includes the count already
      int w = KListViewItem::width(p_->fontMetrics(), lv, column_);
      
      p_->drawText(w-marg, 0, width_-marg-w, height(), align_ | Qt::AlignVCenter, numText);
    }
  }
}

int ParentItem::width(const QFontMetrics& fm_, const QListView* lv_, int column_) const {
  int w = KListViewItem::width(fm_, lv_, column_);

  QListView* lv = listView();
  if(!lv) {
    return -1;
  }

  // show count is only for firct column and depth of 1
  if(lv->isA("BCGroupView") && column_ == 0 && depth() == 1) {
    BCGroupView* groupView = static_cast<BCGroupView*>(lv); 

    if(groupView->showCount()) {
      QString numText = QString::fromLatin1(" (");
      numText += QString::number(m_count);
      numText += QString::fromLatin1(")");

      w += fm_.width(numText);
    }
  }
  return w;
}


