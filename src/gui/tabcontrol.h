/***************************************************************************
    copyright            : (C) 2002-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TABCONTROL_H
#define TABCONTROL_H

#include <qtabwidget.h>

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class TabControl : public QTabWidget {
Q_OBJECT

public:
  /**
   * Constructor
   */
  TabControl(QWidget* parent, const char* name=0);

  QTabBar* tabBar() const;
  void setTabBarHidden(bool hide);

  /**
   * Sets the focus to the first focusable widget on the current page.
   */
  void setFocusToFirstChild();

public slots:
  void clear();
};

  } // end namespace
} // end namespace
#endif
