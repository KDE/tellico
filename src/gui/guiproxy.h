/***************************************************************************
    copyright            : (C) 2009 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_GUI_PROXY_H
#define TELLICO_GUI_PROXY_H

class QWidget;
class QString;

namespace Tellico {
  class MainWindow;
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class Proxy {
public:
  static QWidget* widget();
  static void sorry(const QString& text, QWidget* widget=0);

private:
  friend class Tellico::MainWindow;
  static void setMainWidget(QWidget* widget);
  static QWidget* s_widget;
};

  } // end namespace
} // end namespace
#endif
