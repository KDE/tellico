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

#include <kdebug.h>

using Bookcase::MultiSelectionListView;
using Bookcase::MultiSelectionListViewItem;

MultiSelectionListView::MultiSelectionListView(QWidget* parent_, const char* name_)
    : KListView(parent_, name_/*=0*/) {
  setSelectionMode(QListView::Extended);
}

void MultiSelectionListView::updateSelected(MultiSelectionListViewItem* item_, bool s_) const {
  if(s_) {
    m_selectedItems.append(item_);
  } else {
    m_selectedItems.removeRef(item_);
  }
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

#include "multiselectionlistview.moc"
