/***************************************************************************
    Copyright (C) 2001-2020 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_GROUPVIEW_H
#define TELLICO_GROUPVIEW_H

#include "gui/treeview.h"
#include "observer.h"

namespace Tellico {
  namespace Data {
    class EntryGroup;
  }
  class Filter;
  class GroupIterator;
  class EntryGroupModel;

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
class GroupView : public GUI::TreeView, public Observer {
Q_OBJECT

public:
  /**
   * The constructor sets up the single column, and initializes the popup menu.
   *
   * @param parent A pointer to the parent widget
   */
  GroupView(QWidget* parent);

  EntryGroupModel* sourceModel() const;

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
  QString entrySortField() const;
  void setEntrySortField(const QString& groupSortName);
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
   * Refresh all the items for the collection.
   */
  void populateCollection();
  /**
   * Selects the first item which refers to a certain entry.
   *
   * @param entry A pointer to the entry
   */
  void setEntrySelected(Data::EntryPtr entry);

  virtual void modifyField(Data::CollPtr coll, Data::FieldPtr oldField, Data::FieldPtr newField) override;

public Q_SLOTS:
  /**
   * Resets the list view, clearing and deleting all items.
   */
  void slotReset();
  /**
   * Adds or removes listview items when groups are modified.
   *
   * @param coll A pointer to the collection of the group
   * @param groups A vector of pointers to the modified groups
   */
  void slotModifyGroups(Tellico::Data::CollPtr coll, QList<Tellico::Data::EntryGroup*> groups);

private:
  void contextMenuEvent(QContextMenuEvent* event) override;
  /**
   * Inserts a listviewitem for a given group
   *
   * @param group The group to be added
   */
  void addGroup(Data::EntryGroup* group);

  QString groupTitle();
  void updateHeader(Data::FieldPtr field=Data::FieldPtr());

private Q_SLOTS:
  /**
   * Handles changing the icon when an item is expanded, depended on whether it refers
   * to a collection, a group, or an entry.
   */
  void slotExpanded(const QModelIndex& index);
  /**
   * Handles changing the icon when an item is collapsed, depended on whether it refers
   * to a collection, a group, or an entry.
   */
  void slotCollapsed(const QModelIndex& index);
  /**
   * Filter by group
   */
  void slotFilterGroup();
  void slotDoubleClicked(const QModelIndex& index);
  void slotSortingChanged(int column, Qt::SortOrder order);
  void slotSortMenuActivated(QAction* action);

Q_SIGNALS:
  /**
   * Signals a desire to filter the view.
   *
   * @param filter A pointer to the filter
   */
  void signalUpdateFilter(Tellico::FilterPtr filter);

private:
  friend class GroupIterator;

  bool m_notSortedYet;
  Data::CollPtr m_coll;
  QString m_groupBy;
  QString m_entrySortField;

  QString m_groupOpenIconName;
  QString m_groupClosedIconName;
};

} // end namespace
#endif
