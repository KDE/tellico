/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
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
#include "entry.h"
#include "gui/counteditem.h"
#include "collection.h"
#include "filter.h"

#include <kdebug.h>
#include <kiconloader.h>

#include <qregexp.h>

using Tellico::EntryItem;

EntryItem::EntryItem(GUI::CountedItem* parent_, Data::Entry* entry_)
    : GUI::ListViewItem(parent_), m_entry(entry_), m_customSort(false) {
  setText(0, m_entry->title());
  setPixmap(0, UserIcon(entry_->collection()->entryName()));
}

// should only get called for DetailedListView parents
int EntryItem::compareColumn(QListViewItem* item_, int col_) const {
  DetailedListView* lv = static_cast<DetailedListView*>(listView());
  if(lv->isNumber(col_)) {
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
    return ListViewItem::compare(item_, col_, true);
  }
}

int EntryItem::compare(QListViewItem* item_, int col_, bool asc_) const {
  // if not custom sort, do default compare
  if(!m_customSort) {
    // but, check if depth() > 0, then assume the parent is the GroupView
    // if sorting by column 1, always sort aphabetically ascending by column 0
    // that way, even if reverse sorting by count, the entries are alphabetized
    if(col_ == 1 && depth() > 0 ) {
      return asc_ ? key(0, asc_).compare(item_->key(0, asc_)) : item_->key(0, asc_).compare(key(0, asc_));
    } else {
      return ListViewItem::compare(item_, col_, asc_);
    }
  }

// if keys are equal, check previous column
// if those keys are equal, check column before that
  int result = compareColumn(item_, col_);
  if(result != 0) {
    return result;
  }

  Tellico::DetailedListView* lv = static_cast<Tellico::DetailedListView*>(listView());
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
