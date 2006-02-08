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

#include "groupiterator.h"
#include "entrygroupitem.h"

using Tellico::GroupIterator;

GroupIterator::GroupIterator(const QListView* view_) {
  // groups are the first children in the group view
  m_item = static_cast<GUI::ListViewItem*>(view_->firstChild());
}

GroupIterator& GroupIterator::operator++() {
  m_item = static_cast<GUI::ListViewItem*>(m_item->nextSibling());
  return *this;
}

Tellico::Data::EntryGroup* GroupIterator::group() {
  if(!m_item || !m_item->isEntryGroupItem()) {
    return 0;
  }
  return static_cast<EntryGroupItem*>(m_item)->group();
}
