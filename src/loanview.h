/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
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

#include "gui/listview.h"
#include "observer.h"
#include "borroweritem.h"

#include <qdict.h>

namespace Tellico {
  namespace Data {
    class Borrower;
  }
  class BorrowerItem;

/**
 * @author Robby Stephenson
 */
class LoanView : public GUI::ListView, public Observer {
Q_OBJECT

public:
  LoanView(QWidget* parent, const char* name=0);

  virtual bool isSelectable(GUI::ListViewItem*) const;

  void addCollection(Data::CollPtr coll);

  virtual void    addBorrower(Data::BorrowerPtr);
  virtual void modifyBorrower(Data::BorrowerPtr);

private slots:
  /**
   * Handles the appearance of the popup menu.
   *
   * @param item A pointer to the item underneath the mouse
   * @param point The location point
   * @param col The column number, not currently used
   */
  void contextMenuRequested(QListViewItem* item, const QPoint& point, int col);
  void slotExpanded(QListViewItem* item);
  void slotCollapsed(QListViewItem* item);
  void slotCheckIn();
  void slotModifyLoan();

private:
  virtual void setSorting(int column, bool ascending = true);

  bool m_notSortedYet;
  QDict<BorrowerItem> m_itemDict;
};

} // end namespace

#endif
