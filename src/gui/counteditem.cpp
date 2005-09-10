/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "counteditem.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"

#include <kglobalsettings.h>

#include <qpainter.h>
#include <qpixmap.h>

using Tellico::GUI::CountedItem;

int CountedItem::compare(QListViewItem* item_, int col_, bool asc_) const {
  GUI::ListView* lv = static_cast<GUI::ListView*>(listView());
  if(lv->sortStyle() == GUI::ListView::SortByCount) {
    if(childCount() < item_->childCount()) {
      return -1;
    } else if(childCount() > item_->childCount()) {
      return 1;
    } else {
      return GUI::ListViewItem::compare(item_, col_, asc_);
    }
  }
  // for now, only other style is by text
  return GUI::ListViewItem::compare(item_, col_, asc_);
}

void CountedItem::paintCell(QPainter* p_, const QColorGroup& cg_,
                            int column_, int width_, int align_) {
  if(!p_) {
    return;
  }

  // always paint the cell

  // show count is only for first column
  if(column_ != 0) {
    ListViewItem::paintCell(p_, cg_, column_, width_, align_);
  } else {
    // set an empty text so that we can have our own implementation (see further down)
    // but still benefit from KListView::paintCell
    QString oldText = text(column_);
    if(oldText.isEmpty()) {
      return; // avoid endless loop!
    }

    setText(column_, QString::null);
    ListViewItem::paintCell(p_, cg_, column_, width_, align_);
    setText(column_, oldText);

    const QPixmap* icon = pixmap(column_);

    int marg = listView()->itemMargin();
    int r = marg;
    if(icon) {
      r += icon->width() + marg;
    }

/*
    QFont oldFont = p_->font();
    QFont newFont = oldFont;
    newFont.setBold(true);
    p_->setFont(newFont);
*/

    QFontMetrics fm = p_->fontMetrics();
    QString numText = QString::fromLatin1(" (%1)").arg(childCount());
    // don't call CountedListViewItem::width() because that includes the count already
    int w = ListViewItem::width(fm, listView(), column_);
    int countWidth = fm.width(numText);
    if(w+marg+r+countWidth > width_) {
      oldText = Tellico::rPixelSqueeze(oldText, fm, width_-marg-r-countWidth);
    }
    if(isSelected()) {
      p_->setPen(cg_.highlightedText());
    } else {
      p_->setPen(cg_.text());
    }
    QRect br;
    p_->drawText(r, 0, width_-marg-r, height(), align_ | AlignVCenter, oldText, -1, &br);

    if(isSelected()) {
      p_->setPen(cg_.highlightedText());
    } else {
      if(!Tellico::contrastColor.isValid()) {
        updateContrastColor(cg_);
      }
      p_->setPen(Tellico::contrastColor);
    }
    p_->drawText(br.right(), 0, width_-marg-br.right(), height(), align_ | Qt::AlignVCenter, numText);

//    p_->setFont(oldFont);
  }
}

int CountedItem::width(const QFontMetrics& fm_, const QListView* lv_, int column_) const {
  int w = ListViewItem::width(fm_, lv_, column_);

  // show count is only for first column
  if(column_ == 0) {
    QString numText = QString::fromLatin1(" (%1)").arg(childCount());
    w += fm_.width(numText) + 2; // add a little pad
  }
  return w;
}
