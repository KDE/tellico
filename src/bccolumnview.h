/* *************************************************************************
                                bccolumnview.h
                             -------------------
    begin                : Tue Sep 4 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@radiojodi.com
 * *************************************************************************/

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 * *************************************************************************/

#ifndef BCCOLUMNVIEW_H
#define BCCOLUMNVIEW_H

class BCUnit;
class BCUnitItem;
class BCCollection;
class KListView;
class QListViewItem;
class QWidgetStack;

//#include <klistview.h>
#include <kpopupmenu.h>
#include <qpoint.h>

/**
 * The BCColumnView class shows detailed information about units in a specific collection.
 *
 * A QWidgetStack flips between list views for each collection. Obviously, only units
 * in a common collection are shown at a time.
 *
 * @author Robby Stephenson
 * @version $Id: bccolumnview.h,v 1.16 2002/02/09 07:11:25 robby Exp $
 */
class BCColumnView : public QWidget {
Q_OBJECT

public:
  /**
   * The constructor initializes the popup menu, but no columns are inserted.
   *
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  BCColumnView(QWidget* parent, const char* name=0);
  /**
   */
  ~BCColumnView();

  /**
   * Returns the visible KListView object or NULL if none exists.
   * Basically just a wrapper around a cast and a call to @ref QWidgetStack::visibleWidget.
   *
   * @return A pointer to the KListView object
   */
  KListView* visibleListView();
  /**
   * Returns a listview by id number or NULL if none exists.
   * Basically just a wrapper around a cast and a call to @ref QWidgetStack::widget.
   *
   * @param id The id number
   * @return A pointer to the KListView object
   */
  KListView* listView(int id);

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
  void slotAddPage(BCCollection* coll);
  /**
   * Removes the page corresponding to a certain collection.
   *
   * @param coll A pointer to the collection being removed
   */
   void slotRemovePage(BCCollection* coll);
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
   * Ensures that the unit is visible, bringing container list view to front
   *
   * @param unit A point to the unit
   */
  void slotShowUnit(BCUnit* unit);
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
   * @param view A pointer to the listview to which the item belongs, if known
   * @return A pointer to the item
   */
  BCUnitItem* locateItem(BCUnit* unit, KListView* view=0);

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
  void signalDoUnitDelete(BCUnit* unit);

private:
  BCCollection* m_currColl;
  QWidgetStack* m_stack;
  KPopupMenu m_menu;
};

#endif
