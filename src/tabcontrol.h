/***************************************************************************
    copyright            : (C) 2002-2004 by Robby Stephenson
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

namespace Bookcase {

/**
 * @author Robby Stephenson
 * @version $Id: tabcontrol.h 527 2004-03-11 02:38:36Z robby $
 */
class TabControl : public QTabWidget {
Q_OBJECT

public:
  /**
   * Constructor
   */
  TabControl(QWidget* parent, const char* name=0);

  QTabBar* tabBar();
  /**
   * Resets the focus to the first focusable widget on each page.
   */
//  void resetFocus();
  void setResetFocus(bool reset);

public slots:
  /**
   * Sets the focus to the first focusable widget on the current page.
   */
  void slotUpdateFocus(QWidget* page);
  void clear();

private:
  /**
   * Sets the focus to the first focusable widget on the page.
   *
   * @param page The page to set the focus
   */
  void setFocusToFirstChild(QWidget* page);

  bool m_resetFocus;
};

} // end namespace
#endif
