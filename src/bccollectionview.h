/* *************************************************************************
                             bccollectionview.h
                             -------------------
    begin                : Sat Oct 13 2001
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

#ifndef BCCOLLECTIONVIEW_H
#define BCCOLLECTIONVIEW_H

class BCUnit;
class BCAttribute;
class QListViewItem;

#include "bcunititem.h"
#include "bccollection.h"

#include <klistview.h>
#include <kpopupmenu.h>

#include <qdict.h>
#include <qlist.h>
#include <qpoint.h>
#include <qpixmap.h>

/**
 * The BCCollectionView is the main listview for the class, showing only the titles.
 *
 * There is one root item for each collection in the document. The units are grouped
 * by the attribute defined by each collection. A @ref QDict is used to keep track of the
 * group items.
 *
 * @see BCCollection
 *
 * @author Robby Stephenson
 * @version $Id: bccollectionview.h,v 1.19 2002/10/20 16:36:52 robby Exp $
 */
class BCCollectionView : public KListView {
Q_OBJECT

public:
  /**
   * The constructor sets up the single column, and initializes the popup menu.
   *
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  BCCollectionView(QWidget* parent, const char* name=0);
  /**
   */
  ~BCCollectionView();

public slots:
  /**
   * Resets the list view, clearing and deleting all items.
   */
  void slotReset();
  /**
   * Adds a new root item for a collection.
   *
   * @param coll A pointer to the collection
   */
  void slotAddItem(BCCollection* coll);
  /**
   * Adds a new list item for a unit. If a new group is needed, it will be created.
   *
   * @param unit A pointer to the unit
   */
  void slotAddItem(BCUnit* unit);
  /**
   * Modifies any items which refer to a unit. This slot is called when the unit is
   * modified, regardless of whether or not the title actually changed. If the group
   * attribute value has changed, all relevant groups are created, modified, or deleted.
   *
   * @param unit A pointer to the unit
   */
  void slotModifyItem(BCUnit* unit);
  /**
   * Removes a root collection item, and all of its children.
   *
   * @param coll A pointer to the collection
   */
  void slotRemoveItem(BCCollection* coll);
  /**
   * Removes all items which refer to a certain unit. Any empty groups will also
   * be deleted.
   *
   * @param unit A pointer to the unit
   */
  void slotRemoveItem(BCUnit* unit);
  /**
   * Selects the first item which refers to a certain unit.
   *
   * @param unit A pointer to the unit
   */
  void slotSetSelected(BCUnit* unit);
  /**
   * Expands all items at the same depth as the current selected item.
   */
  void slotExpandAll();
  /**
   * Overloads the previous method, to allow custom depth expansion. If depth is
   * equal to either 0 or 1, then all items at that depth are expanded.
   *
   * @param depth The depth value
   */
  void slotExpandAll(int depth);
  /**
   * Collapses all items at the same depth as the current selected item.
   */
  void slotCollapseAll();
  /**
   * Overloads the previous method, to allow custom depth collapsing. If depth is
   * equal to either 0 or 1, then all items at that depth are collapsed.
   *
   * @param depth The depth value
   */
  void slotCollapseAll(int depth);

protected:
  /**
   * A helper method to insert an item using the value of the group attribute, setting
   * the parent collection item and referring to the unit. The method takes care if the
   * group attribute can have multiple values separated by a semi-colon. The inserted
   * item is made visible, and its parent is opened. If a new group item is needed, it's
   * created.
   *
   * @param group A pointer to the group attribute
   * @param root A pointer to the root collection parent item
   * @param unit A pointer to the unit
   */
  void insertItem(BCAttribute* group, ParentItem* root, BCUnit* unit);
  /**
   * A helper method to locate all items which refer to a certain unit. Since multiple
   * items could exist, a QList is returned.
   *
   * @param unit A pointer to the unit
   * @return A list of BCUnitItems which refer to the unit
   */
  QList<BCUnitItem> locateItem(BCUnit* unit);
  /**
   * Returns a pointer to the root item for the collection. If none exists, then one
   * is created.
   *
   * @param coll A pointer to the collection
   * @return A pointer to the parent item
   */
  ParentItem* locateItem(BCCollection* coll);

protected slots:
  /**
   * Handles everything when an item is selected. The proper signal is emitted, depending
   * on whether the item refers to a collection, a group, or a unit.
   *
   * @param item A pointer to the selected item
   */
  void slotSelected(QListViewItem* item);
  /**
   * Toggles the open/closed status of the item if it refers to a collection or a group.
   *
   * @param item A pointer to the toggled item
   */
  void slotToggleItem(QListViewItem* item);
  /**
   * Handles the appearance of the popup menu, determining which of the three (collection,
   * group, or unit) menus to display.
   *
   * @param item A pointer to the item underneath the mouse
   * @param point The location point
   * @param col The column number, not currently used
   */
  void slotRMB(QListViewItem* item, const QPoint& point, int col);
  /**
   * Handles clicking the rename menu item. The collection is not modified, but
   * the @ref signalDoCollectionRename signal is emitted.
   */
  void slotHandleRename();
  /**
   * Handles changing the icon when an item is expanded, depended on whether it refers
   * to a collection, a group, or a unit.
   *
   * @param item A pointer to the expanded list item
   */
  void slotExpanded(QListViewItem* item);
  /**
   * Handles changing the icon when an item is collapsed, depended on whether it refers
   * to a collection, a group, or a unit.
   *
   * @param item A pointer to the collapse list item
   */
  void slotCollapsed(QListViewItem* item);
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
   * Signals a unit has been selected. Only emitted when selection changed, and the
   * selection item refers to a unit.
   *
   * @param unit A pointer to the unit to which the selected item refers
   */
  void signalUnitSelected(BCUnit* unit);
  /**
   * Signals a collection has been selected. Only emitted when selection changed, and
   * the selection item refers to a collection.
   *
   * @param unit The id of the collection to which the selected item refers
   */
  void signalCollectionSelected(int id);
  /**
   * Signals a desire to rename the collection having a certain id.
   *
   * @param id The collection id
   * @param newName The new name of the collection
   */
  void signalRenameCollection(int id, const QString& newName);

private:
  QDict<ParentItem> m_groupDict;

  KPopupMenu m_collMenu;
  KPopupMenu m_groupMenu;
  KPopupMenu m_unitMenu;
  QPixmap m_groupOpenPixmap;
  QPixmap m_groupClosedPixmap;
};

#endif
