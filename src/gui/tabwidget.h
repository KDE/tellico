/***************************************************************************
    copyright            : (C) 2002-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_GUI_TABWIDGET_H
#define TELLICO_GUI_TABWIDGET_H

#include <KTabWidget>

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class TabWidget : public KTabWidget {
Q_OBJECT

public:
  /**
   * Constructor
   */
  TabWidget(QWidget* parent);

  /**
   * Sets the focus to the first focusable widget on the current page.
   */
  void setFocusToFirstChild();
};

  } // end namespace
} // end namespace
#endif
