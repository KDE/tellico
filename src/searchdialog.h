/* *************************************************************************
                             searchdialog.h
                             -------------------
    begin                : Wed Feb 27 2002
    copyright            : (C) 2002 by Robby Stephenson
    email                : robby@periapsis.org
 * *************************************************************************/

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 * *************************************************************************/

#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <kdialogbase.h>

#include <qwidget.h>

/**
 * The search dialog allows the user to search for a string in the document.
 *
 * @author Robby Stephenson
 * @version $Id: searchdialog.h,v 1.4 2002/11/10 00:38:29 robby Exp $
 */
class SearchDialog : public KDialogBase  {
Q_OBJECT

public: 
  /**
   * The constructor sets up the dialog.
   *
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  SearchDialog(QWidget* parent=0, const char* name=0);

protected slots:
  /**
   * Called when the Find button is clicked.
   */
  void slotUser1();
};

#endif
