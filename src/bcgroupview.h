/* *************************************************************************
                             bcgroupview.h
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

#ifndef BCGROUPVIEW_H_H
#define BCGROUPVIEW_H_H

class BCUnit;
class BCAttribute;
class BCUnitGroup;

#include "bcunititem.h"
#include "bccollection.h"

#include <klistview.h>
#include <kpopupmenu.h>

#include <qdict.h>
#include <qptrlist.h>
#include <qpoint.h>
#include <qpixmap.h>

/**
 * The BCGroupView is the main listview for the class, showing only the titles.
 *
 * There is one root item for each collection in the document. The units are grouped
 * by the attribute defined by each collection. A @ref QDict is used to keep track of the
 * group items.
 *
 * @see BCCollection
 *
 * @author Robby Stephenson
 * @version $Id: bcgroupview.h,v 1.6 2002/11/25 00:56:22 robby Exp $
 */
class BCGroupView : public KListView {
Q_OBJECT

public:
  /**
   * The constructor sets up the single column, and initializes the popup menu.
   *
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  BCGroupView(QWidget* parent, const char* name=0);
  /**
   * Returns the name of the attribute by which the units are grouped
   *
   * @return The attribute name
   */
  const QString& groupAttribute() const;
  /**
   * Sets the name of the attribute by which the units are grouped
   *
   * @param coll A pointer to the collection being grouped
   * @param groupAttName The attribute name
   */
  void setGroupAttribute(BCCollection* coll, const QString& groupAttName);

public slots:
  /**
   * Resets the list view, clearing and deleting all items.
   */
  void slotReset();
  /**
   * Adds or removes listview items when a group is modified.
   *
   * @param coll A pointer to the collection of the gorup
   * @param group A pointer to the modified group
   */
  void slotModifyGroup(BCCollection* coll, BCUnitGroup* group);
  /**
   * Removes a root collection item, and all of its children.
   *
   * @param coll A pointer to the collection
   */
  void slotRemoveItem(BCCollection* coll);
  /**
   * Selects the first item which refers to a certain unit.
   *
   * @param unit A pointer to the unit
   */
  void slotSetSelected(BCUnit* unit);
  /**
   * Expands all items at a certain depth. If depth is -1, the current selected item
   * is expanded. If depth is equal to either 0 or 1, then all items at that depth
   * are expanded.
   *
   * @param depth The depth value
   */
  void slotExpandAll(int depth=-1);
  /**
   * Collapses all items at a certain depth. If depth is -1, the current selected item
   * is collapsed. If depth is equal to either 0 or 1, then all items at that depth
   * are collapsed.
   *
   * @param depth The depth value
   */
  void slotCollapseAll(int depth=-1);
  /**
   * Adds a collection, along with all all the groups for the collection in
   * the groupAttribute. This method gets called as well when the groupAttribute
   * is changed, since it essentially repopulates the listview.
   *
   * @param coll A pointer to the collection being added
   */
  void slotAddCollection(BCCollection* coll);
  /**
   * Changes the view to show or hide the number if items in the group.
   *
   * @param showCount A boolean indicating whether or not the count should be shown
   */
  void slotShowCount(bool showCount);

protected:
  /**
   * Returns a pointer to the root item for the collection. If none exists, then one
   * is created.
   *
   * @param coll A pointer to the collection
   * @return A pointer to the collection listviewitem
   */
  ParentItem* locateItem(BCCollection* coll);
  /**
   * A helper method to locate any pointer to a listviewitem which references
   * a given BCUnitGroup
   *
   * @param unit A pointer to the collection listviewitem
   * @return A pointer to the group listviewitem
   */
  ParentItem* locateItem(ParentItem* collItem, BCUnitGroup* group);
  /**
   * Inserts a listviewitem for a given group
   *
   * @param The parent listview item, for the collection itself
   * @param group The group to be added
   * @return A pointer to the created @ ref ParentItem
   */
  ParentItem* insertItem(ParentItem* collItem, BCUnitGroup* group);
  
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
  QString m_groupAttribute;

  KPopupMenu m_collMenu;
  KPopupMenu m_groupMenu;
  KPopupMenu m_unitMenu;
  QPixmap m_groupOpenPixmap;
  QPixmap m_groupClosedPixmap;

  bool m_showCount;
};

#endif
