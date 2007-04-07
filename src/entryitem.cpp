/***************************************************************************
    copyright            : (C) 2003-2007 by Robby Stephenson
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
#include "controller.h"

#include <kiconloader.h>

using Tellico::EntryItem;

EntryItem::EntryItem(GUI::ListView* parent, Data::EntryPtr entry)
    : GUI::ListViewItem(parent), m_entry(entry), m_isDetailedList(true) {
}

EntryItem::EntryItem(GUI::CountedItem* parent_, Data::EntryPtr entry_)
    : GUI::ListViewItem(parent_), m_entry(entry_), m_isDetailedList(false) {
  setText(0, m_entry->title());
  setPixmap(0, UserIcon(entry_->collection()->typeName()));
}

Tellico::Data::EntryPtr const EntryItem::entry() const {
  return m_entry;
}

QString EntryItem::key(int col_, bool) const {
  // first column is always title unless it's a detailed list
  // detailed list takes care of things on its own
  bool checkArticles = (!m_isDetailedList && col_ == 0);
  // there's some sort of painting bug if the key is identical for multiple entries
  // probably a null string in the group view. TODO
  // don't add the entry id if it's a detailed view cause that messes up secondary sorting
  QString key = (checkArticles ? Data::Field::sortKeyTitle(text(col_)) : text(col_));
  return (m_isDetailedList ? key : key + QString::number(m_entry->id()));
}

void EntryItem::doubleClicked() {
  Controller::self()->editEntry(m_entry);
}

Tellico::Data::EntryVec EntryItem::entries() const {
  return Data::EntryVec(entry());
}
