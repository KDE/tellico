/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

// much of this code is adapted from libkdepim
// which is GPL licensed, Copyright (c) 2004 David Faure

#ifndef TELLICO_GUI_OVERLAYWIDGET_H
#define TELLICO_GUI_OVERLAYWIDGET_H

#include <qframe.h>

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class OverlayWidget : public QFrame {
Q_OBJECT

public:
  OverlayWidget(QWidget* parent, QWidget* anchor);

  void setCorner(Corner corner);
  Corner corner() const { return m_corner; }

  void addWidget(QWidget* widget);

protected:
  void resizeEvent(QResizeEvent* event);
  bool eventFilter(QObject* object, QEvent* event);
  bool event(QEvent* event);

private:
  void reposition();

  QWidget* m_anchor;
  Corner m_corner;
};

  } // end namespace
} // end namespace

#endif
