/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "entryitem.h"
#include "groupview.h"
#include "collection.h"
#include "detailedlistview.h"

#include <kdebug.h>

#include <qpainter.h>
#include <qregexp.h>

using Tellico::EntryItem;
using Tellico::ParentItem;

// should only get called for DetailedListView parents
int EntryItem::compareColumn(QListViewItem* item_, int col_) const {
  DetailedListView* lv = dynamic_cast<DetailedListView*>(listView());
  if(lv && lv->isNumber(col_)) {
    // by default, an empty string would get sorted before "1" because toFloat() turns it into "0"
    // I want the empty strings to be at the end
    bool ok1, ok2;
    // use section in case of multiple values
    float num1 = text(col_).section(';', 0, 0).toFloat(&ok1);
    float num2 = item_->text(col_).section(';', 0, 0).toFloat(&ok2);
    if(ok1 && ok2) {
      return static_cast<int>(num1 - num2);
    } else if(ok1 && !ok2) {
      return -1;
    } else if(!ok1 && ok2) {
      return 1;
    } else {
      return 0;
    }
  } else {
    return KListViewItem::compare(item_, col_, true);
  }
}

int EntryItem::compare(QListViewItem* item_, int col_, bool asc_) const {
  // if not custom sort, do default compare
  if(!m_customSort) {
    // but, check if depth() == 2, then assume the parent is the GroupView
    // if sorting by column 1, always sort aphabetically ascending by column 0
    // that way, even if reverse sorting by count, the entries are alphabetized
    if(col_ == 1 && depth() == 2 ) {
      return asc_ ? key(0, asc_).compare(item_->key(0, asc_)) : item_->key(0, asc_).compare(key(0, asc_));
    } else {
      return KListViewItem::compare(item_, col_, asc_);
    }
  }

// if keys are equal, check previous column
// if those keys are equal, check column before that
  int result = compareColumn(item_, col_);
  if(result != 0) {
    return result;
  }

  Tellico::DetailedListView* lv = dynamic_cast<Tellico::DetailedListView*>(listView());
  result = compareColumn(item_, lv->prevSortedColumn());
  if(result != 0) {
    return result;
  }
  return compareColumn(item_, lv->prev2SortedColumn());
}

// if there's a non-null pixmap and no text, return a tab character to put this one first
QString EntryItem::key(int col_, bool) const {
  if(pixmap(col_) && !pixmap(col_)->isNull() && text(col_).isEmpty()) {
    // a little weird, sort for width, too, in case of rating widget
    // but sort reverse by width
    return QChar('\t') + QString::number(1000-pixmap(col_)->width());
  } else {
    // empty string go last
    return text(col_);
  }
}

// prepend a tab character to always sort the empty group name first
// also check for surname prefixes
QString ParentItem::key(int col_, bool) const {
  if(col_ > 0) {
    return text(col_);
  }

  if(text(col_) == Data::Collection::s_emptyGroupTitle) {
    return QString::fromLatin1("\t");
  }

  if(m_text.isEmpty() || m_text != text(col_)) {
    m_text = text(col_);
    if(Data::Field::autoFormat()) {
      // build a regexp to match surname prefixes
      // complicated by fact that prefix could have an apostrophe
      QString prefixes;
      for(QStringList::ConstIterator it = Data::Field::surnamePrefixList().begin();
                                     it != Data::Field::surnamePrefixList().end();
                                     ++it) {
        prefixes += (*it);
        if(!(*it).endsWith(QChar('\''))) {
          prefixes += QString::fromLatin1("\\s");
        }
        // if it's not the last item, add a pipe
        if(!(*it) != Data::Field::surnamePrefixList().last()) {
          prefixes += QChar('|');
        }
      }
      QRegExp rx(QString::fromLatin1("^(") + prefixes + QChar(')'), false);
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

// if the parent listview is a GroupView, column=0, and showCount is true, then
// include and color the number of books.
// Otherwise, just pass the call up the line
void ParentItem::paintCell(QPainter* p_, const QColorGroup& cg_,
                           int column_, int width_, int align_) {
  if(!p_) {
    return;
  }

  // always paint the cell
  KListViewItem::paintCell(p_, cg_, column_, width_, align_);

  GroupView* groupView = dynamic_cast<Tellico::GroupView*>(listView());
  if(!groupView) {
    return;
  }

  // show count is only for first column and depth of 1
  if(column_ == 0 && depth() == 1 && groupView->showCount()) {
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

int ParentItem::width(const QFontMetrics& fm_, const QListView* lv_, int column_) const {
  int w = KListViewItem::width(fm_, lv_, column_);

  // show count is only for first column and depth of 1
  if(lv_ && lv_->isA("Tellico::GroupView") && column_ == 0 && depth() == 1) {
    const GroupView* groupView = static_cast<const GroupView*>(lv_);

    if(groupView->showCount()) {
      QString numText = QString::fromLatin1(" (%1)").arg(m_count);
      w += fm_.width(numText);
    }
  }
  return w;
}

void ParentItem::setCount(int count_) {
  m_count = count_;
  //s.sprintf("%06d", count_); // surely no one will ever have over a million entries!
  int digits = 1;
  while(count_ /= 10) {
    ++digits;
  }
  QString s;
  s.fill('0', 6 - digits);
  s += QString::number(m_count);
  setText(1, s);
}
