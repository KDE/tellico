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

#ifndef GROUPVIEW_H_H
#define GROUPVIEW_H_H

namespace Bookcase {
  namespace Data {
    class Collection;
  }
  class ParentItem;
  class Filter;
}
class KPopupMenu;

#include "multiselectionlistview.h"
#include "entryitem.h" // needed for Parent Item

#include <qdict.h>
#include <qpoint.h>
#include <qpixmap.h>

namespace Bookcase {

/**
 * The GroupView is the main listview for the class, showing only the titles.
 *
 * There is one root item for each collection in the document. The entries are grouped
 * by the field defined by each collection. A @ref QDict is used to keep track of the
 * group items.
 *
 * @see Bookcase::Data::Collection
 *
 * @author Robby Stephenson
 * @version $Id: groupview.h 744 2004-08-07 22:00:00Z robby $
 */
class GroupView : public MultiSelectionListView {
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
   * Returns the name of the field by which the entries are grouped
   *
   * @return The field name
   */
  const QString& groupBy() const { return m_groupBy; }
  /**
   * Sets the name of the field by which the entries are grouped
   *
   * @param coll A pointer to the collection being grouped
   * @param groupFieldName The field name
   */
  void setGroupField(Data::Collection* coll, const QString& groupFieldName);
  /**
   * Returns true if the view should show the number of items in each group.
   */
  bool showCount() const { return m_showCount; }
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
   * Selects the first item which refers to a certain entry.
   *
   * @param entry A pointer to the entry
   */
  void setEntrySelected(Data::Entry* entry);
  /**
   * Refresh all the items for a collection.
   *
   * @param coll The collection
   * @return The item for the collection
   */
  ParentItem* populateCollection(Data::Collection* coll);
  bool isSelectable(MultiSelectionListViewItem* item) const;

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
   * on whether the item refers to a collection, a group, or a entry.
   */
  void slotSelectionChanged();
  /**
   * Handles the appearance of the popup menu, determining which of the three (collection,
   * group, or entry) menus to display.
   *
   * @param item A pointer to the item underneath the mouse
   * @param point The location point
   * @param col The column number, not currently used
   */
  void contextMenuRequested(QListViewItem* item, const QPoint& point, int col);
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
  /**
   * Sort groups by group name, ascending.
   */
  void slotSortByGroupAscending();
  /**
   * Sort groups by group name, descending.
   */
  void slotSortByGroupDescending();
  /**
   * Sort groups by the number of entries in each group, ascending.
   */
  void slotSortByCountAscending();
  /**
   * Sort groups by the number of entries in each group, descending.
   */
  void slotSortByCountDescending();
  /**
   * Filter by group
   */
  void slotFilterGroup();
  void slotDoubleClicked(QListViewItem* item);

signals:
  /**
   * Signals a desire to delete a entry.
   *
   * @param entry A pointer to the entry
   */
  void signalDeleteEntry(Bookcase::Data::Entry* entry);
  /**
   * Signals a desire to filter the view.
   *
   * @param filter A pointer to the filter
   */
  void signalUpdateFilter(Bookcase::Filter* filter);

private:
  QDict<ParentItem> m_groupDict;
  QString m_groupBy;

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
