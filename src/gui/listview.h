/***************************************************************************
    copyright            : (C) 2001-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_LISTVIEW_H
#define TELLICO_LISTVIEW_H

#include <klistview.h>
#include <kdeversion.h>

namespace Tellico {
  namespace GUI {

class ListViewItem;
typedef QPtrList<ListViewItem> ListViewItemList;
typedef QPtrListIterator<ListViewItem> ListViewItemListIt;

/**
 * A ListView keeps track of the selected items and allows subclasses to determine
 * whether child items may be selected or not. In addition, it provides alternating
 * background colors and shaded sort columns.
 *
 * @see GroupView
 * @see DetailedListView
 *
 * @author Robby Stephenson
 */
class ListView : public KListView {
Q_OBJECT

friend class ListViewItem; // needed so the ListViewItem d'tor can update selection list

public:
  enum SortStyle {
    SortByText = 0, // don't change these values, they're saved as ints in the config
    SortByCount = 1
  };

  ListView(QWidget* parent, const char* name = 0);
  virtual ~ListView() {}

  void clearSelection();
  /**
   * Returns a list of the currently selected items;
   *
   * @return The list of selected items
   */
  const ListViewItemList& selectedItems() const { return m_selectedItems; }
  /**
   * Used to determine whether an item may be selected without having to setSelectable on
   * every child item.
   *
   * @return Whether the item may be selected
   */
  virtual bool isSelectable(ListViewItem*) const;
  SortStyle sortStyle() const { return m_sortStyle; }
  void setSortStyle(SortStyle style) { m_sortStyle = style; }

#if !KDE_IS_VERSION(3,3,90)
  // taken from KDE bug 59791
  void setShadeSortColumn(bool shade_);
  bool shadeSortColumn() const { return m_shadeSortColumn; }
  const QColor& background2() const { return m_backColor2; }
  const QColor& alternateBackground2() const { return m_altColor2; }
#endif

protected slots:
  /**
   * Handles everything when an item is selected. The proper signal is emitted, depending
   * on whether the item refers to a collection, a group, or a entry.
   */
  virtual void slotSelectionChanged();

protected:
  virtual void drawContentsOffset(QPainter* p, int ox, int oy, int cx, int cy, int cw, int ch);

private slots:
  void slotUpdateColors();
  void slotDoubleClicked(QListViewItem* item);

private:
  /**
   * Updates the pointer list.
   *
   * @param item The item being selected or deselected
   * @param s Selected or not
   */
  void updateSelected(ListViewItem* item, bool s);

  SortStyle m_sortStyle;
  bool m_isClear;
  ListViewItemList m_selectedItems;
#if !KDE_IS_VERSION(3,3,90)
  bool m_shadeSortColumn;
  QColor m_backColor2;
  QColor m_altColor2;
#endif
};

/**
 * The ListViewItem keeps track of what kind of specialized listview item it is, as
 * well as taking care of the selection tracking.
 *
 * @author Robby Stephenson
 */
class ListViewItem : public KListViewItem {
public:
  ListViewItem(ListView* parent) : KListViewItem(parent), m_sortWeight(-1) {}
  ListViewItem(ListViewItem* parent) : KListViewItem(parent), m_sortWeight(-1) {}
  ListViewItem(ListView* parent, const QString& text) : KListViewItem(parent, text), m_sortWeight(-1) {}
  ListViewItem(ListViewItem* parent, const QString& text) : KListViewItem(parent, text), m_sortWeight(-1) {}
  virtual ~ListViewItem();

  virtual void clear();

  virtual bool isEntryGroupItem() const { return false; }
  virtual bool isEntryItem() const { return false; }
  virtual bool isFilterItem() const { return false; }
  virtual bool isBorrowerItem() const { return false; }
  virtual bool isLoanItem() const { return false; }

  int sortWeight() const { return m_sortWeight; }
  void setSortWeight(int w) { m_sortWeight = w; }
  virtual int compare(QListViewItem* item, int col, bool ascending) const;

  virtual void setSelected(bool selected);
  /**
   * Returns the background color for the column, which depends on whether the item
   * is an alternate row and whether the column is selected.
   *
   * @param column The column number
   * @param alternate The alternate row color can be forced
   */
  const QColor& backgroundColor(int column, bool alternate=false);
  virtual void paintCell(QPainter* painter, const QColorGroup& colorGroup,
                         int column, int width, int alignment);

private:
  int m_sortWeight;
};

  } // end namespace
} // end namespace

#endif
