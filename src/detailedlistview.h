/***************************************************************************
    Copyright (C) 2001-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_DETAILEDLISTVIEW_H
#define TELLICO_DETAILEDLISTVIEW_H

#include "gui/treeview.h"
#include "observer.h"
#include "filter.h"

#include <QStringList>
#include <QEvent>
#include <QVector>

class QMenu;

namespace Tellico {
  class DetailedEntryItem;
  class EntryModel;

/**
 * The DetailedListView class shows detailed information about entries in the
 * collection.
 *
 * @author Robby Stephenson
 */
class DetailedListView : public GUI::TreeView, public Observer {
Q_OBJECT

public:
  /**
   * The constructor initializes the popup menu, but no columns are inserted.
   *
   * @param parent A pointer to the parent widget
   */
  DetailedListView(QWidget* parent);
  virtual ~DetailedListView();

  EntryModel* sourceModel() const;

  /**
   * Event filter used to popup the menu
   */
  bool eventFilter(QObject* obj, QEvent* ev) override;
  /**
   * Selects the item which refers to a certain entry.
   *
   * @param entry A pointer to the entry
   */
  void setEntriesSelected(Data::EntryList entries);
  void setFilter(FilterPtr filter);
  FilterPtr filter() const;

  QString sortColumnTitle1() const;
  QString sortColumnTitle2() const;
  QString sortColumnTitle3() const;
  QStringList visibleColumns() const;
  Data::EntryList visibleEntries();

  /**
   * @param coll A pointer to the collection
   */
  void addCollection(Data::CollPtr coll);
  /**
   * Removes all items which refers to a entry within a collection.
   *
   * @param coll A pointer to the collection
   */
  void removeCollection(Data::CollPtr coll);

  /**
   * Adds a new list item showing the details for a entry.
   *
   * @param entry A pointer to the entry
   */
  virtual void addEntries(Data::EntryList entries) override;
  /**
   * Modifies any item which refers to a entry, resetting the column contents.
   *
   * @param entry A pointer to the entry
   */
  virtual void modifyEntries(Data::EntryList entries) override;
  /**
   * Removes any item which refers to a certain entry.
   *
   * @param entry A pointer to the entry
   */
  virtual void removeEntries(Data::EntryList entries) override;

  virtual void addField(Data::CollPtr, Data::FieldPtr field) override;
  void addField(Data::FieldPtr field, int width);
  virtual void modifyField(Data::CollPtr, Data::FieldPtr oldField, Data::FieldPtr newField) override;
  virtual void removeField(Data::CollPtr, Data::FieldPtr field) override;

  void reorderFields(const Data::FieldList& fields);
  /**
   * saveConfig is only needed for custom collections */
  void saveConfig(Data::CollPtr coll, int saveConfig);
  /**
   * Select all visible items.
   */
  void selectAllVisible();
  int visibleItems() const;
  void resetEntryStatus();

public Q_SLOTS:
  /**
   * Resets the list view, clearing and deleting all items.
   */
  void slotReset();
  /**
   * Refreshes the view, repopulating all items.
   */
  void slotRefresh();
  void slotRefreshImages();

private Q_SLOTS:
  void slotDoubleClicked(const QModelIndex& index);
  void slotColumnMenuActivated(QAction* action);
  void updateHeaderMenu();
  void showAllColumns();
  void hideAllColumns();
  void hideCurrentColumn();
  void resizeColumnsToContents();
  void hideNewColumn(const QModelIndex& index, int start, int end);
//  void slotCacheColumnWidth(int section, int oldSize, int newSize);
  void updateColumnDelegates();

private:
  void contextMenuEvent(QContextMenuEvent* event) override;
  void setState(Tellico::Data::EntryList entries_, int state);
  void adjustColumnWidths();
  void checkHeader();
  QString columnFieldName(int ncol) const;

  struct ConfigInfo {
    QStringList cols;
    QList<int> widths;
    QList<int> order;
    int prevSort;
    int prev2Sort;
    int sortOrder;
  };

  QMenu* m_headerMenu;
  QMenu* m_columnMenu;
  bool m_loadingCollection;
  int m_currentContextColumn;
};

} // end namespace;
#endif
