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
 * @version $Id: bctabcontrol.h,v 1.8 2002/11/10 00:38:29 robby Exp $
 */
class BCTabControl : public KTabCtl {
Q_OBJECT

public:
  /**
   * Constructor
   */
  BCTabControl(QWidget* parent, const char* name=0);
  /**
   * Sets the focus to the first focusable widget on a certain tab page.
   *
   * @param tabNum The tab number
   */
  void setFocusToLineEdit(int tabNum);
  /**
   * Returns the numbers of tabs in the widget.
   *
   * @return The number of tabs
   */
  int count();

public slots:
  /**
   * Calls the parent method. Essentialy just making @ref KTabCtl#showTab public.
   *
   * @param i The id of the tab to show
   */
  void showTab(int i);
};

#endif
