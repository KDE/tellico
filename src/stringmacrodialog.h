/***************************************************************************
                             stringmacrodialog.h
                             -------------------
    begin                : Fri Oct 24 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef STRINGMACRODIALOG_H
#define STRINGMACRODIALOG_H

class BCCollection;
class BibtexCollection;

class QListView;

#include <kdialogbase.h>

/**
 * @author Robby Stephenson
 * @version $Id: stringmacrodialog.h 227 2003-10-25 17:28:09Z robby $
 */
class StringMacroDialog : public KDialogBase {
Q_OBJECT

public: 
  StringMacroDialog(BCCollection* coll, QWidget* parent, const char* name=0);

private slots:
  void slotOk();
  void slotAdd();
  void slotDelete();

private:
  BibtexCollection* m_coll;
  QListView* m_listView;
};

#endif
