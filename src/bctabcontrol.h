/* *************************************************************************
                             bctabcontrol.h
                             -------------------
    begin                : Sun Jan 6 2002
    copyright            : (C) 2002 by Robby Stephenson
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

#ifndef BCTABCONTROL_H
#define BCTABCONTROL_H

#include <ktabctl.h>

/**
 * BCTabControl subclasses KTabCtl merely to make the @ref KTabCtl::showTab slot public.
 *
 * The class is empty and thus identical to @ref KTabCtl except that showTab()
 * is redeclared to be public.
 *
 * @author Robby Stephenson
 * @version $Id: bctabcontrol.h,v 1.4 2002/01/17 04:19:42 robby Exp $
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

public slots:
  /**
   * Calls the parent method.
   *
   * @param i The id of the tab to show
   */
  void showTab(int i);
};

#endif
