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
#include "bcdetailedlistview.h"

#include <qpainter.h>
#include <qregexp.h>

int BCUnitItem::compare(QListViewItem* item_, int col_, bool asc_) const {
  BCDetailedListView* lv = dynamic_cast<BCDetailedListView*>(listView());

// compare keys. If not equal or not BCDetailedListView parent view, just return result
// otherwise, check prev sorted column
  int result = KListViewItem::compare(item_, col_, asc_);
  if(result != 0 || !lv) {
    return result;
  } else {
    return KListViewItem::compare(item_, lv->prevSortedColumn(), asc_);
  }
}

// if there's a non-null pixmap and no text, return a tab character to put this one first
QString BCUnitItem::key(int col_, bool) const {
  if(pixmap(col_) && !pixmap(col_)->isNull() && text(col_).isEmpty()) {
    return QString::fromLatin1("\t");
  } else {
    return text(col_);
  }
}

// prepend a tab character to always sort the empty group name first
// also check for surname prefixes
QString ParentItem::key(int col_, bool) const {
  if(col_ > 0) {
    return text(col_);
  }

  if(text(col_) == BCCollection::emptyGroupName()) {
    return QString::fromLatin1("\t");
  }
  
  if(m_text.isEmpty() || m_text != text(col_)) {
    m_text = text(col_);
    if(BCAttribute::autoFormat()) {
      QString prefixes = BCAttribute::surnamePrefixList().join(QString::fromLatin1("|"));
      QRegExp rx(QString::fromLatin1("^(") + prefixes + QString::fromLatin1(")\\s"));
      rx.setCaseSensitive(false);
      if(rx.search(m_text) > -1) {
        m_key = m_text.mid(rx.matchedLength());
      } else {
        m_key = m_text;
      }
    } else {
      m_key = m_text;
    }
  }

  return m_key;
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

  BCGroupView* groupView = dynamic_cast<BCGroupView*>(listView());
  if(!groupView) {
    return;
  }

  // show count is only for first column and depth of 1
  if(column_ == 0 && depth() == 1) {
    if(groupView->showCount()) {
      int marg = groupView->itemMargin();

      QString numText = QString::fromLatin1(" (%1)").arg(m_count);

      if(isSelected()) {
        p_->setPen(cg_.highlightedText());
      } else {
        p_->setPen(cg_.highlight());
      }

      // don't call ParentItem::width() because that includes the count already
      int w = KListViewItem::width(p_->fontMetrics(), groupView, column_);
      
      p_->drawText(w-marg, 0, width_-marg-w, height(), align_ | Qt::AlignVCenter, numText);
    }
  }
}

int ParentItem::width(const QFontMetrics& fm_, const QListView* lv_, int column_) const {
  int w = KListViewItem::width(fm_, lv_, column_);

  // show count is only for first column and depth of 1
  if(lv_ && lv_->isA("BCGroupView") && column_ == 0 && depth() == 1) {
    const BCGroupView* groupView = static_cast<const BCGroupView*>(lv_); 

    if(groupView->showCount()) {
      QString numText = QString::fromLatin1(" (");
      numText += QString::number(m_count);
      numText += QString::fromLatin1(")");

      w += fm_.width(numText);
    }
  }
  return w;
}


