/* *************************************************************************
                                bcdetailedlistview.h
                             -------------------
    begin                : Tue Sep 4 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@periapsis.org
 * *************************************************************************/

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 * *************************************************************************/

#ifndef BCDETAILEDLISTVIEW_H
#define BCDETAILEDLISTVIEW_H

class BCUnit;
class BCUnitItem;
class BCCollection;

class QListViewItem;

#include <klistview.h>
#include <kpopupmenu.h>

#include <qpoint.h>

/**
 * The BCDetailedListView class shows detailed information about units in the
 * collection.
 *
 * @author Robby Stephenson
 * @version $Id: bcdetailedlistview.h,v 1.24 2002/11/10 06:23:51 robby Exp $
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

public slots:
  /**
   * Resets the list view, clearing and deleting all items.
   */
  void slotReset();
  /**
   * Adds a new listview to the stack, and populates it with listitems corresponding to
   * all the units in the collection.
   *
   * @param coll A pointer to the collection added
   */
  void slotSetContents(BCCollection* coll);
  /**
   * Adds a new list item showing the details for a unit.
   *
   * @param unit A pointer to the unit
   */
  void slotAddItem(BCUnit* unit);
  /**
   * Modifies any item which refers to a unit, resetting the column contents.
   *
   * @param unit A pointer to the unit
   */
  void slotModifyItem(BCUnit* unit);
  /**
   * Removes any item which refers to a certain unit.
   *
   * @param unit A pointer to the unit
   */
  void slotRemoveItem(BCUnit* unit);
  /**
   * Selects the item which refers to a certain unit.
   *
   * @param unit A pointer to the unit
   */
  void slotSetSelected(BCUnit* unit);
  /**
   * Handles emitting the signal indicating a desire to delete the unit attached
   * to the current list item.
   */
  void slotHandleDelete();

protected:
  /**
   * A helper method to populate an item. The column text is initialized by pulling
   * the contents from the unit pointer of the item, so it should properly be set
   * before this method is called.
   *
   * @param item A pointer to the item
   */
  void populateItem(BCUnitItem* item);
  /**
   * A helper method to locate any item which refers to a certain unit. If none
   * is found, a NULL pointer is returned.
   *
   * @param unit A pointer to the unit
   * @return A pointer to the item
   */
  BCUnitItem* locateItem(BCUnit* unit);

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
   *
   * @param item The selected item
   */
  void slotSelected(QListViewItem* item);
  /**
   * Clears the selection.
   */
  void slotClearSelection();

signals:
  /**
   * Signals that the selection has been cleared.
   */
  void signalClear();
  /**
   * Signals a unit has been selected. Only emitted when selection changed.
   *
   * @param unit A pointer to the unit to which the selected item refers
   */
  void signalUnitSelected(BCUnit* unit);
  /**
   * Signals a desire to delete a unit.
   *
   * @param unit A pointer to the unit
   */
  void signalDeleteUnit(BCUnit* unit);

private:
  BCCollection* m_currColl;
  KPopupMenu m_menu;
};

#endif
