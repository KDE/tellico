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

#ifndef BCGROUPVIEW_H_H
#define BCGROUPVIEW_H_H

class KPopupMenu;

#include "entry.h" // needed for EntryList
#include "entryitem.h" // needed for Parent Item

#include <klistview.h>

#include <qdict.h>
#include <qptrlist.h>
#include <qpoint.h>
#include <qpixmap.h>

namespace Bookcase {
  namespace Data {
    class Collection;
  }
  class ParentItem;

/**
 * The GroupView is the main listview for the class, showing only the titles.
 *
 * There is one root item for each collection in the document. The units are grouped
 * by the field defined by each collection. A @ref QDict is used to keep track of the
 * group items.
 *
 * @see Bookcase::Data::Collection
 *
 * @author Robby Stephenson
 * @version $Id: groupview.h 386 2004-01-24 05:12:28Z robby $
 */
class GroupView : public KListView {
Q_OBJECT

public:
  /**
   * The constructor sets up the single column, and initializes the popup menu.
   *
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  GroupView(QWidget* parent, const char* name=0);

  /**
   * Returns the name of the field by which the units are grouped
   *
   * @return The field name
   */
  const QString& groupBy() const;
  /**
   * Sets the name of the field by which the units are grouped
   *
   * @param coll A pointer to the collection being grouped
   * @param groupFieldName The field name
   */
  void setGroupField(Data::Collection* coll, const QString& groupFieldName);
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
   * the groupFieldribute. This method gets called as well when the groupFieldribute
   * is changed, since it essentially repopulates the listview.
   *
   * @param coll A pointer to the collection being added
   */
  void addCollection(Data::Collection* coll);
  /**
   * Removes a root collection item, and all of its children.
   *
   * @param coll A pointer to the collection
   */
  void removeCollection(Data::Collection* coll);
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
  void setEntrySelected(Data::Entry* unit);
  /**
   * Refresh all the items for a collection.
   *
   * @param coll The collection
   * @return The item for the collection
   */
  ParentItem* populateCollection(Data::Collection* coll);
  
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
  void slotModifyGroup(Bookcase::Data::Collection* coll, const Bookcase::Data::EntryGroup* group);
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
  ParentItem* locateItem(Data::Collection* coll);
  /**
   * A helper method to locate any pointer to a listviewitem which references
   * a given EntryGroup
   *
   * @param collItem A pointer to the collection listviewitem
   * @param group The group to be added
   * @return A pointer to the group listviewitem
   */
  ParentItem* locateItem(ParentItem* collItem, const Data::EntryGroup* group);
  /**
   * Inserts a listviewitem for a given group
   *
   * @param collItem The parent listview item, for the collection itself
   * @param group The group to be added
   * @return A pointer to the created @ ref ParentItem
   */
  ParentItem* insertItem(ParentItem* collItem, const Data::EntryGroup* group);
  /**
   * A helper method to make sure that the key used for the groups
   * is consistant. For now, the key is the id of the unit's collection
   * concatenated with the text of the group name.
   */
  QString groupKey(const ParentItem* par, QListViewItem* item) const;
  /**
   * Identical to the previous function.
   */
  QString groupKey(const ParentItem* par, const Data::EntryGroup* group_) const;
  /**
   * Identical to the previous function.
   */
  QString groupKey(const Data::Collection* coll, QListViewItem* item) const;
  /**
   * Identical to the previous function.
   */
  QString groupKey(const Data::Collection* coll, const Data::EntryGroup* group) const;
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
   * group, or entry) menus to display.
   *
   * @param item A pointer to the item underneath the mouse
   * @param point The location point
   * @param col The column number, not currently used
   */
  void slotRMB(QListViewItem* item, const QPoint& point, int col);
  /**
   * Handles changing the icon when an item is expanded, depended on whether it refers
   * to a collection, a group, or an entry.
   *
   * @param item A pointer to the expanded list item
   */
  void slotExpanded(QListViewItem* item);
  /**
   * Handles changing the icon when an item is collapsed, depended on whether it refers
   * to a collection, a group, or an entry.
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
  void signalEntrySelected(QWidget* widget, const Bookcase::Data::EntryList& list);
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
  void signalDeleteEntry(Bookcase::Data::Entry* unit);

private:
  QDict<ParentItem> m_groupDict;
  QString m_groupBy;
  Data::EntryList m_selectedEntries;

  KPopupMenu* m_collMenu;
  KPopupMenu* m_groupMenu;
  KPopupMenu* m_entryMenu;
  
  QPixmap m_collOpenPixmap;
  QPixmap m_collClosedPixmap;
  QPixmap m_groupOpenPixmap;
  QPixmap m_groupClosedPixmap;

  bool m_showCount;
};

} // end namespace
#endif
