/* *************************************************************************
                          bcuniteditwidget.h  -  description
                             -------------------
    begin                : Wed Sep 26 2001
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

#ifndef BCUNITEDITWIDGET_H
#define BCUNITEDITWIDGET_H

#include "bccollection.h"
#include <klineedit.h>
#include <kcombobox.h>
#include <qmultilineedit.h>
#include <qcheckbox.h>
#include <qlist.h>
#include <qdict.h>
#include <qgrid.h>

class BCUnit;
class KTabCtl;
class QListViewItem;
class QPushButton;
class QWidgetStack;

/**
  * @author Robby Stephenson
  * @version $Id: bcuniteditwidget.h,v 1.10 2001/11/05 05:56:43 robby Exp $
  */
class BCUnitEditWidget : public QWidget  {
Q_OBJECT

public: 
  BCUnitEditWidget(QWidget* parent, const char* name=0);
  ~BCUnitEditWidget();

public slots:
  void slotAddPage(BCCollection* coll);
  void slotRemovePage(BCCollection* coll);
  void slotReset();
  void slotHandleClear();
  void slotSetContents(BCUnit* unit);

protected:
  void clearWidgets();

protected slots:
  void slotHandleReturn(const QString& text);
  void slotHandleNew();
  void slotHandleCopy();
  void slotHandleSave();
  void slotHandleDelete();

private:
  BCCollection* m_currColl;
  BCUnit* m_currUnit;
  QWidgetStack* m_pages;
  //QDict<QGrid> m_tabDict;
  QDict<KLineEdit>* m_editDict;
  QDict<QMultiLineEdit>* m_multiDict;
  QDict<KComboBox>* m_comboDict;
  QDict<QCheckBox>* m_checkDict;
  QPushButton* m_new;
  QPushButton* m_copy;
  QPushButton* m_save;
  QPushButton* m_delete;
  QPushButton* m_clear;

signals:
  void signalDoUnitSave(BCUnit* unit);
  void signalDoUnitDelete(BCUnit* unit);
};

#endif
