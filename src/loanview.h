/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
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

class KPopupMenu;

#include "gui/listview.h"
#include "observer.h"

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

  void addCollection(Data::Collection* coll);

  virtual void    addBorrower(Data::Borrower*);
  virtual void modifyBorrower(Data::Borrower*);

private slots:
  /**
   * Handles the appearance of the popup menu.
   *
   * @param item A pointer to the item underneath the mouse
   * @param point The location point
   * @param col The column number, not currently used
   */
  void contextMenuRequested(QListViewItem* item, const QPoint& point, int col);
  void slotCheckIn();
  void slotModifyLoan();

private:
  virtual void setSorting(int column, bool ascending = true);

  bool m_notSortedYet;
  KPopupMenu* m_loanMenu;
  QDict<BorrowerItem> m_itemDict;
};

} // end namespace

#endif
