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

#ifndef TELLICO_LOANVIEW_H
#define TELLICO_LOANVIEW_H

#include "gui/treeview.h"
#include "observer.h"

#include <QTreeView>

namespace Tellico {
  class BorrowerModel;
  class EntrySortModel;

/**
 * @author Robby Stephenson
 */
class LoanView : public GUI::TreeView, public Observer {
Q_OBJECT

public:
  LoanView(QWidget* parent);

//  virtual bool isSelectable(GUI::ListViewItem*) const;
  BorrowerModel* sourceModel() const;

  virtual void addCollection(Data::CollPtr coll);

  virtual void    addBorrower(Data::BorrowerPtr) override;
  virtual void modifyBorrower(Data::BorrowerPtr) override;
  virtual void removeBorrower(Data::BorrowerPtr) override;

public Q_SLOTS:
  /**
   * Resets the list view, clearing and deleting all items.
   */
  void slotReset();

private Q_SLOTS:
  void slotCheckIn();
  void slotModifyLoan();
  void slotDoubleClicked(const QModelIndex& index);
  void slotSortingChanged(int column, Qt::SortOrder order);

private:
  void contextMenuEvent(QContextMenuEvent* event) override;
  void updateHeader();

  bool m_notSortedYet;
  Data::CollPtr m_coll;
};

} // end namespace

#endif
