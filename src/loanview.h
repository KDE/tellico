/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
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

  virtual void    addBorrower(Data::BorrowerPtr);
  virtual void modifyBorrower(Data::BorrowerPtr);

public slots:
  /**
   * Resets the list view, clearing and deleting all items.
   */
  void slotReset();

private slots:
  void slotCheckIn();
  void slotModifyLoan();
  void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
  void slotDoubleClicked(const QModelIndex& index);
  void slotSortingChanged(int column, Qt::SortOrder order);

private:
  void contextMenuEvent(QContextMenuEvent* event);
  void updateHeader();

  bool m_notSortedYet;
  Data::CollPtr m_coll;
};

} // end namespace

#endif
