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

#ifndef ENTRYITEM_H
#define ENTRYITEM_H

#include "multiselectionlistview.h"

// for QColorGroup
#include <qpalette.h>
//#include <qguardedptr.h>

namespace Tellico {
  namespace Data {
    class Entry;
    class EntryGroup;
  }

/**
 * The EntryItem is a subclass of KListViewItem containing a pointer to an Entry.
 *
 * The entry pointer allows easy access to listview items which refer to a certain entry.
 *
 * @see Entry
 *
 * @author Robby Stephenson
 * @version $Id: entryitem.h 862 2004-09-15 01:49:51Z robby $
 */
class EntryItem : public MultiSelectionListViewItem {
public:
  /**
   * This constructor is for items which are direct children of a KListView object.
   *
   * @param parent A pointer to the parent
   * @param entry A pointer to the entry to which the item refers
   */
  EntryItem(MultiSelectionListView* parent, Data::Entry* entry)
      : MultiSelectionListViewItem(parent), m_entry(entry), m_customSort(parent->isA("Tellico::DetailedListView")) {}
  /**
   * This constructor is for items which have other KListViewItems as parents. It
   * initializes the text in the first column, as well.
   *
   * @param parent A pointer to the parent
   * @param text The text in the first column
   * @param entry A pointer to the entry to which the item refers
   */
  EntryItem(MultiSelectionListViewItem* parent, const QString& text, Data::Entry* entry)
      : MultiSelectionListViewItem(parent, text), m_entry(entry), m_customSort(false) {}

  /**
   * Compares one column to another, calling @ref DetailedListView::isNumber() and
   * using that to determine whether to do numerical or alphabetical comparison.
   *
   * @param item Pointer to comparison item
   * @param col Column to compare
   * @return Comparison result, -1,0, or 1
   */
  int compareColumn(QListViewItem* item, int col) const;
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
  Data::Entry* const entry() const { return m_entry; }

private:
  // if I make this a QGuardedPtr, the app crashes, why?
  Data::Entry* m_entry;
  // if the parent is a DetailedListView
  // this way, I don't have to call listView()->isA("Tellico::DetailedListView") every time
  // when I want to do funky comparisons
  bool m_customSort;
};

/**
 * The ParentItem is a subclass of KListViewItem which includes an id reference number.
 *
 * The id allows for matching on collections or whatever. The id is not a unique identifier
 * of the item itself.
 *
 *
 * @author Robby Stephenson
 * @version $Id: entryitem.h 862 2004-09-15 01:49:51Z robby $
 */
class ParentItem : public MultiSelectionListViewItem {
public:
  /**
   * This constructor is for items which are direct children of a KListView object.
   *
   * @param parent A pointer to the parent
   * @param text The text in the first column
   * @param id The id number
   */
  ParentItem(MultiSelectionListView* parent, const QString& text, int id)
      : MultiSelectionListViewItem(parent, text), m_id(id), m_group(0) {}
  /**
   * This constructor is for items which are children of another ParentItem and do not
   * have an id reference number. It is primarily used for grouping of the EntryItems.
   * The id is set to -1.
   *
   * @see EntryItem
   *
   * @param parent A pointer to the parent
   * @param text The text in the first column
   */
  ParentItem(ParentItem* parent, const QString& text, const Data::EntryGroup* group)
      : MultiSelectionListViewItem(parent, text), m_id(-1), m_group(group) {}

  /**
   * Sets the count for the number of items.
   *
   * @param c The count
   */
  void setCount(int c);
  /**
   * Returns the id reference number of the ParentItem.
   *
   * @return The id number
   */
  int id() const { return m_id; }
  /**
   * Returns the id reference number of the ParentItem.
   *
   * @return The id number
   */
  const Data::EntryGroup* group() const { return m_group; }
  /**
   * Returns the key for sorting the listitems. The text used for an empty
   * value should be sorted first, so the returned key is "_". Since the text may
   * have the number of entries or something added to the name, only check if the
   * text begins with the empty name. Maybe there should be something better.
   *
   * @param col The column number
   * @return The key
   */
  virtual QString key(int col, bool) const;

  /**
   * Paints the cell, adding the number count.
   */
  virtual void paintCell(QPainter* p, const QColorGroup& cg,
                         int column, int width, int align);
  virtual int width(const QFontMetrics& fm, const QListView* lv, int c) const;

private:
  int m_id;
  int m_count;
  const Data::EntryGroup* m_group;

// since I do an expensive RegExp match for the surname prefixes, I want to
// cache the text and the resulting key
  mutable QString m_text;
  mutable QString m_key;
};

} // end namespace
#endif
