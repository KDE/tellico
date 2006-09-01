/***************************************************************************
    copyright            : (C) 2001-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef GROUPVIEW_H
#define GROUPVIEW_H

#include "gui/listview.h"
#include "observer.h"

#include <qdict.h>
#include <qpixmap.h>

namespace Tellico {
  namespace Data {
    class EntryGroup;
  }
  class Filter;
  class EntryGroupItem;
  class GroupIterator;

/**
 * The GroupView shows the entries grouped, as well as the saved filters.
 *
 * There is one root item for each collection in the document. The entries are grouped
 * by the field defined by each collection. A @ref QDict is used to keep track of the
 * group items.
 *
 * @see Tellico::Data::Collection
 *
 * @author Robby Stephenson
 */
class GroupView : public GUI::ListView, public Observer {
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
   * @param groupFieldName The field name
   */
  void setGroupField(const QString& groupFieldName);
  /**
   * Adds a collection, along with all all the groups for the collection in
   * the groupFieldribute. This method gets called as well when the groupFieldribute
   * is changed, since it essentially repopulates the listview.
   *
   * @param coll A pointer to the collection being added
   */
  void addCollection(Data::CollPtr coll);
  /**
   * Removes a root collection item, and all of its children.
   *
   * @param coll A pointer to the collection
   */
  void removeCollection(Data::CollPtr coll);
  /**
   * Selects the first item which refers to a certain entry.
   *
   * @param entry A pointer to the entry
   */
  void setEntrySelected(Data::EntryPtr entry);
  /**
   * Refresh all the items for the collection.
   *
   * @return The item for the collection
   */
  void populateCollection();

  void modifyField(Data::CollPtr coll, Data::FieldPtr oldField, Data::FieldPtr newField);

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
  void slotModifyGroup(Tellico::Data::CollPtr coll, Tellico::Data::EntryGroup* group);
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

private:
  /**
   * Inserts a listviewitem for a given group
   *
   * @param group The group to be added
   * @return A pointer to the created @ ref ParentItem
   */
  EntryGroupItem* addGroup(Data::EntryGroup* group);
  /**
   * Traverse all siblings at a certain depth, setting them open or closed. If depth is -1,
   * then the depth of the @ref currentItem() is used.
   *
   * @param depth Desired depth
   * @param open Whether the item should be open or not
   */
  void setSiblingsOpen(int depth, bool open);

private slots:
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
   * Filter by group
   */
  void slotFilterGroup();

signals:
  /**
   * Signals a desire to filter the view.
   *
   * @param filter A pointer to the filter
   */
  void signalUpdateFilter(Tellico::FilterPtr filter);

private:
  friend class GroupIterator;

  virtual void setSorting(int column, bool ascending = true);
  QString groupTitle();
  void updateHeader(Data::FieldPtr field=0);

  bool m_notSortedYet;
  Data::CollPtr m_coll;
  QDict<EntryGroupItem> m_groupDict;
  QString m_groupBy;

  QPixmap m_groupOpenPixmap;
  QPixmap m_groupClosedPixmap;
};

} // end namespace
#endif
