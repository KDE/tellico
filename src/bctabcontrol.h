/* *************************************************************************
                             bctabcontrol.h
                             -------------------
    begin                : Sun Jan 6 2002
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

#ifndef BCTABCONTROL_H
#define BCTABCONTROL_H

#include <ktabctl.h>

/**
 * BCTabControl subclasses KTabCtl and makes a few changes.
 *
 * The @ref KTabCtl::showTab slot is made public. A method is added to move the focus.
 *
 * @author Robby Stephenson
 * @version $Id: bctabcontrol.h,v 1.7 2002/09/22 03:31:43 robby Exp $
 */
class BCTabControl : public KTabCtl {
Q_OBJECT

public:
  /**
   */
  BCTabControl(QWidget* parent, const char* name=0);
  /**
   */
  ~BCTabControl();

  void setFocusToLineEdit(int tabNum);
  int count();
	
public slots:
  /**
   * Calls the parent method.
   *
   * @param i The id of the tab to show
   */
  void showTab(int i);
};

#endif
