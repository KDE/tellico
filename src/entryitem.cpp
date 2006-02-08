/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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
#include "detailedlistview.h"

#include <kdebug.h>
#include <kiconloader.h>

#include <qregexp.h>

using Tellico::EntryItem;

EntryItem::EntryItem(DetailedListView* parent, Data::EntryPtr entry)
    : GUI::ListViewItem(parent), m_entry(entry), m_customSort(true) {
}

EntryItem::EntryItem(GUI::CountedItem* parent_, Data::EntryPtr entry_)
    : GUI::ListViewItem(parent_), m_entry(entry_), m_customSort(false) {
  setText(0, m_entry->title());
  setPixmap(0, UserIcon(entry_->collection()->typeName()));
}

Tellico::Data::EntryPtr const EntryItem::entry() const {
  return m_entry;
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
    return ListViewItem::compare(item_, col_, asc_);
  }

// if keys are equal, check previous column
// if those keys are equal, check column before that
  int result = compareColumn(item_, col_);
  if(result != 0) {
    return result;
  }

  DetailedListView* lv = static_cast<DetailedListView*>(listView());
  result = compareColumn(item_, lv->prevSortedColumn());
  if(result != 0) {
    return result;
  }
  return compareColumn(item_, lv->prev2SortedColumn());
}

// if there's a non-null pixmap and no text, return a tab character to put this one first
QString EntryItem::key(int col_, bool) const {
  if(text(col_).isEmpty() && pixmap(col_) && !pixmap(col_)->isNull()) {
    // a little weird, sort for width, too, in case of rating widget
    // but sort reverse by width
    return QChar('\t') + QString::number(1000-pixmap(col_)->width());
  }
  // there's some sort of painting bug if the key is identical for multiple entries
  // so for non-custom sorting, append the entry id
  return m_customSort ? text(col_) : text(col_) + QString::number(m_entry->id());
}
