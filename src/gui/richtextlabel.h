/***************************************************************************
    copyright            : (C) 2001-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_GUI_RICHTEXTLABEL_H
#define TELLICO_GUI_RICHTEXTLABEL_H

#include <qtextedit.h>

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class RichTextLabel : public QTextEdit {
Q_OBJECT

public:
  RichTextLabel(QWidget* parent);
  RichTextLabel(const QString& text, QWidget* parent);

  virtual QSize sizeHint() const;

private:
  void init();
  // redefine these to disable selection
  void contentsMousePressEvent(QMouseEvent*) {}
  void contentsMouseMoveEvent(QMouseEvent*) {}
  void contentsMouseReleaseEvent(QMouseEvent*) {}
  void contentsMouseDoubleClickEvent(QMouseEvent*) {}
};

  } // end namespace
} // end namespace

#endif
