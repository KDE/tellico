/***************************************************************************
                             bctabcontrol.h
                             -------------------
    begin                : Sun Jan 6 2002
    copyright            : (C) 2002, 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BCTABCONTROL_H
#define BCTABCONTROL_H

#include <qtabwidget.h>

/**
 * @author Robby Stephenson
 * @version $Id: bctabcontrol.h 267 2003-11-08 09:18:46Z robby $
 */
class BCTabControl : public QTabWidget {
Q_OBJECT

public:
  /**
   * Constructor
   */
  BCTabControl(QWidget* parent, const char* name=0);

  QTabBar* tabBar();

public slots:
  /**
   * Sets the focus to the first focusable widget on a certain tab page.
   *
   * @param tabNum The tab number
   */
  void setFocusToChild(int tabNum);
  void clear();
};

#endif
