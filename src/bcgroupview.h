/***************************************************************************
                                bcgroupview.h
                             -------------------
    begin                : Sat Oct 13 2001
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

#ifndef BCGROUPVIEW_H_H
#define BCGROUPVIEW_H_H

class BCAttribute;
class BCCollection;
class ParentItem;

class KPopupMenu;

#include "bcunit.h" // needed for BCUnitList
#include "bcunititem.h" // needed for Parent Item

#include <klistview.h>

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
 * @version $Id: bcgroupview.h 217 2003-10-24 01:32:12Z robby $
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
  const QString& groupBy() const;
  /**
   * Sets the name of the attribute by which the units are grouped
   *
   * @param coll A pointer to the collection being grouped
   * @param groupAttName The attribute name
   */
  void setGroupAttribute(BCCollection* coll, const QString& groupAttName);
  /**
   * Returns true if the view should show the number of items in each group.
   */
  bool showCount() const;
  /**
   * Changes the view to show or hide the number of items in the group.
   *
   * @param showCount A boolean indicating whether or not the count should be shown
   */
  void showCount(bool showCount);
  /**
   * Adds a collection, along with all all the groups for the collection in
   * the groupAttribute. This method gets called as well when the groupAttribute
   * is changed, since it essentially repopulates the listview.
   *
   * @param coll A pointer to the collection being added
   */
  void addCollection(BCCollection* coll);
  /**
   * Removes a root collection item, and all of its children.
   *
   * @param coll A pointer to the collection
   */
  void removeCollection(BCCollection* coll);
  /**
   * Renames the top-level collection item.
   *
   * @param name The new collection name
   */
  void renameCollection(const QString& name);
  /**
   * Clears the selection.
   */
  void clearSelection();
  /**
   * Selects the first item which refers to a certain unit.
   *
   * @param unit A pointer to the unit
   */
  void setUnitSelected(BCUnit* unit);
  
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
  void slotModifyGroup(BCCollection* coll, const BCUnitGroup* group);
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
   * @param collItem A pointer to the collection listviewitem
   * @param group The group to be added
   * @return A pointer to the group listviewitem
   */
  ParentItem* locateItem(ParentItem* collItem, const BCUnitGroup* group);
  /**
   * Inserts a listviewitem for a given group
   *
   * @param collItem The parent listview item, for the collection itself
   * @param group The group to be added
   * @return A pointer to the created @ ref ParentItem
   */
  ParentItem* insertItem(ParentItem* collItem, const BCUnitGroup* group);
  /**
   * A helper method to make sure that the key used for the groups
   * is consistant. For now, the key is the id of the unit's collection
   * concatenated with the text of the group name.
   */
  QString groupKey(const ParentItem* par, QListViewItem* item) const;
  /**
   * Identical to the previous function.
   */
  QString groupKey(const ParentItem* par, const BCUnitGroup* group_) const;
  /**
   * Identical to the previous function.
   */
  QString groupKey(const BCCollection* coll, QListViewItem* item) const;
  /**
   * Identical to the previous function.
   */
  QString groupKey(const BCCollection* coll, const BCUnitGroup* group) const;
  ParentItem* populateCollection(BCCollection* coll);
  /**
   * Traverse all siblings at a certain depth, setting them open or closed. If depth is -1,
   * then the depth of the @ref currentItem() is used.
   *
   * @param depth Desired depth
   * @param open Whether the item should be open or not
   */
  void setSiblingsOpen(int depth, bool open);
  
protected slots:
  /**
   * Handles everything when an item is selected. The proper signal is emitted, depending
   * on whether the item refers to a collection, a group, or a unit.
   */
  void slotSelectionChanged();
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

signals:
  /**
   * Signals that the selection has changed.
   *
   * @param widget A pointer to the widget where the seleection changed, this widget
   * @param list A list of the selected items, may be empty.
   */
  void signalUnitSelected(QWidget* widget, const BCUnitList& list);  
  /**
   * Signals a collection has been selected. Only emitted when selection changed, and
   * the selection item refers to a collection.
   *
   * @param id The id of the collection to which the selected item refers
   */
  void signalCollectionSelected(int id);
  /**
   * Signals a desire to delete a unit.
   *
   * @param unit A pointer to the unit
   */
  void signalDeleteUnit(BCUnit* unit);

private:
  QDict<ParentItem> m_groupDict;
  QString m_groupBy;
  BCUnitList m_selectedUnits;

  KPopupMenu* m_collMenu;
  KPopupMenu* m_groupMenu;
  KPopupMenu* m_unitMenu;
  
  QPixmap m_collOpenPixmap;
  QPixmap m_collClosedPixmap;
  QPixmap m_groupOpenPixmap;
  QPixmap m_groupClosedPixmap;

  bool m_showCount;
};

#endif
