/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FILTERVIEW_H
#define TELLICO_FILTERVIEW_H

#include "gui/treeview.h"
#include "observer.h"

namespace Tellico {
  class FilterModel;
  class EntrySortModel;

/**
 * @author Robby Stephenson
 */
class FilterView : public GUI::TreeView, public Observer {
Q_OBJECT

public:
  FilterView(QWidget* parent);

//  virtual bool isSelectable(GUI::ListViewItem*) const;
  FilterModel* sourceModel() const;

  void addCollection(Data::CollPtr coll);

  virtual void    addEntries(Data::EntryList entries);
  virtual void modifyEntries(Data::EntryList entries);
  virtual void removeEntries(Data::EntryList entries);

  virtual void    addFilter(FilterPtr filter);
  virtual void modifyFilter(FilterPtr) {}
  virtual void removeFilter(FilterPtr filter);

public slots:
  /**
   * Resets the list view, clearing and deleting all items.
   */
  void slotReset();

private slots:
  /**
   * Modify a saved filter
   */
  void slotModifyFilter();
  /**
   * Delete a saved filter
   */
  void slotDeleteFilter();
  void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
  void slotDoubleClicked(const QModelIndex& index);
  void slotSortingChanged(int column, Qt::SortOrder order);

private:
  void contextMenuEvent(QContextMenuEvent* event);
  void updateHeader();
  void invalidate(Data::EntryList entries);

  bool m_notSortedYet;
  Data::CollPtr m_coll;
};

} // end namespace

#endif
