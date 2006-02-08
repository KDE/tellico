/***************************************************************************
    copyright            : (C) 2001-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef ENTRYITEM_H
#define ENTRYITEM_H

#include "gui/listview.h"
#include "datavectors.h"

namespace Tellico {
  class DetailedListView;
  namespace GUI {
    class CountedItem;
  }

/**
 * The EntryItem is a subclass of ListViewItem containing a pointer to an Entry.
 *
 * The entry pointer allows easy access to listview items which refer to a certain entry.
 *
 * @see Entry
 *
 * @author Robby Stephenson
 */
class EntryItem : public GUI::ListViewItem {
public:
  /**
   * This constructor is for items which are direct children of a ListView object,
   * which is just the @ref DetailedListView. Custom sorting is used, so t
   *
   * @param parent A pointer to the parent
   * @param entry A pointer to the entry to which the item refers
   */
  EntryItem(DetailedListView* parent, Data::EntryPtr entry);
  /**
   * This constructor is for items which have other KListViewItems as parents. It
   * initializes the text in the first column, as well.
   *
   * @param parent A pointer to the parent
   * @param text The text in the first column
   * @param entry A pointer to the entry to which the item refers
   */
  EntryItem(GUI::CountedItem* parent, Data::EntryPtr entry);

  virtual bool isEntryItem() const { return true; }

  /**
   * Compares one column to another. If the parent is a @ref DetailedListView,
   * it calls @ref compareColumn, otherwise just does default comparison.
   *
   * @param item Pointer to comparison item
   * @param col Column to compare
   * @param ascending Whether ascending or descing comparison, ignored
   * @return Comparison result, -1,0, or 1
   */
  virtual int compare(QListViewItem* item, int col, bool ascending) const;
  /**
   * Returns the key for the list item. The key is just the text, unless there is none,
   * in which case a tab character is returned if there is a non-null pixmap.
   *
   * @param col Column to compare
   * @return The key string
   */
  virtual QString key(int col, bool) const;
  /**
   * Returns a const pointer to the entry to which the item refers
   *
   * @return The entry pointer
   */
  Data::EntryPtr const entry() const;

private:
  /**
   * Compares one column to another, calling @ref DetailedListView::isNumber() and
   * using that to determine whether to do numerical or alphabetical comparison.
   *
   * @param item Pointer to comparison item
   * @param col Column to compare
   * @return Comparison result, -1,0, or 1
   */
  int compareColumn(QListViewItem* item, int col) const;

  Data::EntryPtr m_entry;
  // if the parent is a DetailedListView
  // this way, I don't have to call listView()->isA("Tellico::DetailedListView") every time
  // when I want to do funky comparisons
  bool m_customSort;
};

} // end namespace
#endif
