/***************************************************************************
                             bcdetailedlistview.h
                             -------------------
    begin                : Tue Sep 4 2001
    copyright            : (C) 2001, 2002, 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BCDETAILEDLISTVIEW_H
#define BCDETAILEDLISTVIEW_H

class BCUnitItem;
class BCCollection;
class BCAttribute;
class BCFilter;

// needed for BCUnitList definition
#include "bcunit.h"

#include <klistview.h>
#include <kpopupmenu.h>

#include <qpoint.h>
#include <qstringlist.h>
#include <qpixmap.h>
#include <qvaluevector.h>
#include <qguardedptr.h>

/**
 * The BCDetailedListView class shows detailed information about units in the
 * collection.
 *
 * @author Robby Stephenson
 * @version $Id: bcdetailedlistview.h 307 2003-11-26 01:45:44Z robby $
 */
class BCDetailedListView : public KListView {
Q_OBJECT

public:
  /**
   * The constructor initializes the popup menu, but no columns are inserted.
   *
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  BCDetailedListView(QWidget* parent, const char* name=0);
  ~BCDetailedListView();
  
  bool eventFilter(QObject* obj, QEvent* ev);
  /**
   * Clears the selection.
   */
  void clearSelection();
  /**
   * Selects the item which refers to a certain unit.
   *
   * @param unit A pointer to the unit
   */
  void setUnitSelected(BCUnit* unit);
  void setFilter(const BCFilter* filter);
  const BCFilter* filter() const;
  
  int prevSortedColumn() const;
  int prev2SortedColumn() const;
  virtual void setSorting(int column, bool ascending = true);
  void setPrevSortedColumn(int prev1, int prev2);
  QString sortColumnTitle1() const;
  QString sortColumnTitle2() const;
  QString sortColumnTitle3() const;
  QStringList visibleColumns() const;
  BCUnitList visibleUnits();
  /**
   * Returns whether the given column is formatted as a number or not.
   *
   * @param column Column number
   * @return Whether column is formatted as a number or not
   */
  bool isNumber(int column) const;
  /**
   * @param coll A pointer to the collection
   */
  void addCollection(BCCollection* coll);
  /**
   * Removes all items which refers to a unit within a collection.
   *
   * @param coll A pointer to the collection
   */
  void removeCollection(BCCollection* coll);
  /**
   * Adds a new list item showing the details for a unit.
   *
   * @param unit A pointer to the unit
   */
  void addItem(BCUnit* unit);
  /**
   * Modifies any item which refers to a unit, resetting the column contents.
   *
   * @param unit A pointer to the unit
   */
  void modifyItem(BCUnit* unit);
  /**
   * Removes any item which refers to a certain unit.
   *
   * @param unit A pointer to the unit
   */
  void removeItem(BCUnit* unit);
  void addAttribute(BCAttribute* att, int width=-1);
  void modifyAttribute(BCAttribute* newAtt, BCAttribute* oldAtt);
  void removeAttribute(BCAttribute* att);
  void reorderAttributes(const BCAttributeList& list);
  void saveConfig(BCCollection* coll);

public slots:
  /**
   * Resets the list view, clearing and deleting all items.
   */
  void slotReset();
  /**
   * Refreshes the view, repopulating all items.
   */
  void slotRefresh();

protected:
  /**
   * A helper method to populate an item. The column text is initialized by pulling
   * the contents from the unit pointer of the item, so it should properly be set
   * before this method is called.
   *
   * @param item A pointer to the item
   */
  void populateItem(BCUnitItem* item);
  void setPixmapAndText(BCUnitItem* item, int col, BCAttribute* att);
  
  /**
   * A helper method to locate any item which refers to a certain unit. If none
   * is found, a NULL pointer is returned.
   *
   * @param unit A pointer to the unit
   * @return A pointer to the item
   */
  BCUnitItem* const locateItem(const BCUnit* unit);
  void showColumn(int col);
  void hideColumn(int col);
  void updateFirstSection();

protected slots:
  /**
   * Handles the appearance of the popup menu.
   *
   * @param item A pointer to the list item underneath the mouse
   * @param point The location point
   * @param col The column number, not currently used
   */
  void slotRMB(QListViewItem* item, const QPoint& point, int col);
  /**
   * Handles everything when an item is selected. The proper signal is emitted.
   */
  void slotSelectionChanged();
  void slotHeaderMenuActivated(int id);
  void slotCacheColumnWidth(int section, int oldSize, int newSize);
  /**
   * Slot to update the position of the pixmap
   */
  void slotUpdatePixmap();

signals:
  /**
   * Signals that the selected units have changed. Zero, one or more may be selected.
   *
   * @param widget A pointer to the widget where the selection changed, this widget
   * @param list A list of the selected items, may be empty.
   */
  void signalUnitSelected(QWidget* widget, const BCUnitList& list);
  /**
   * Signals a desire to delete a unit.
   *
   * @param unit A pointer to the unit
   */
  void signalDeleteUnit(BCUnit* unit);
  /**
   * Signals that a fraction of an operation has been completed.
   *
   * @param f The fraction, 0 =< f >= 1
   */
  void signalFractionDone(float f);

private:
  KPopupMenu* m_itemMenu;
  KPopupMenu* m_headerMenu;
  QValueVector<int> m_columnWidths;
  QValueVector<bool> m_isNumber;
  QPixmap m_unitPix;
  QPixmap m_checkPix;
  BCUnitList m_selectedUnits;
  const BCFilter* m_filter;
  int m_prevSortColumn;
  int m_prev2SortColumn;
  int m_firstSection;
};

#endif
