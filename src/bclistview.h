/* *************************************************************************
                          bclistview.h  -  description
                             -------------------
    begin                : Tue Sep 4 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@radiojodi.com
 * *************************************************************************/

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 * *************************************************************************/

#ifndef BCLISTVIEW_H
#define BCLISTVIEW_H

class BCUnit;
class BCUnitItem;
class BCCollection;
class KPopupMenu;
//class QWidgetStack;

#include <klistview.h>
#include <qpoint.h>

/**
  * @author Robby Stephenson
  * @version $Id: bclistview.h,v 1.10 2001/11/05 05:56:43 robby Exp $
  */
class BCListView : public KListView {
Q_OBJECT

public:
  BCListView(QWidget* parent, const char* name=0);
  ~BCListView();

public slots:
  void slotReset();
  void slotAddPage(BCCollection* coll);
  void slotRemovePage(BCCollection* coll);
  void slotAddItem(BCUnit* unit);
  void slotModifyItem(BCUnit* unit);
  void slotRemoveItem(BCUnit* unit);
  void slotSetSelected(BCUnit* unit);
  void slotHandleDelete();

protected:
  void populateItem(BCUnitItem* item);
  BCUnitItem* locateItem(BCUnit* unit);

protected slots:
  void slotRMB(QListViewItem* item, const QPoint& point, int i);
  void slotSelected(QListViewItem* item);

private:
//  QWidgetStack* m_pages;
  KPopupMenu* m_menu;

signals:
  void signalClear();
  void signalUnitSelected(BCUnit* unit);
  void signalDoUnitDelete(BCUnit* unit);
};

#endif
