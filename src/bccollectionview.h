/* *************************************************************************
                          bccollectionview.h  -  description
                             -------------------
    begin                : Sat Oct 13 2001
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

#ifndef BCCOLLECTIONVIEW_H
#define BCCOLLECTIONVIEW_H

class BCUnit;
class BCAttribute;
class KPopupMenu;
class QListViewItem;

#include "bcunititem.h"
#include "bccollection.h"
#include <klistview.h>
#include <qdict.h>
#include <qlist.h>
#include <qpoint.h>
#include <qpixmap.h>

/**
  * @author Robby Stephenson
  * @version $Id: bccollectionview.h,v 1.8 2001/11/05 05:56:43 robby Exp $
  */
class BCCollectionView : public KListView {
Q_OBJECT

public: 
  BCCollectionView(QWidget* parent, const char* name=0);
  /**
   */
  ~BCCollectionView();

public slots:
  void slotReset();
  void slotAddItem(BCCollection* coll);
  void slotAddItem(BCUnit* unit);
  void slotModifyItem(BCUnit* unit);
  void slotRemoveItem(BCCollection* coll);
  void slotRemoveItem(BCUnit* unit);
  void slotSetSelected(BCUnit* unit);

protected:
  void insertItem(BCAttribute* group, ParentItem* root, BCUnit* unit);
  QList<BCUnitItem> locateItem(BCUnit* unit);
  ParentItem* locateItem(BCCollection* coll);

protected slots:
  void slotSelected(QListViewItem* item);
  void slotToggleItem(QListViewItem* item);
  void slotExpandAll();
  void slotCollapseAll();
  void slotRMB(QListViewItem* item, const QPoint& point, int col);
  void slotHandleRename();
  void slotExpanded(QListViewItem* item);
  void slotCollapsed(QListViewItem* item);

signals:
  void signalClear();
  void signalUnitSelected(BCUnit* unit);
  void signalDoCollectionRename(int id, const QString& newName);

private:
  QDict<ParentItem>* m_groupDict;
  KPopupMenu* m_menu;
  QPixmap m_groupOpen;
  QPixmap m_groupClosed;
};

#endif
