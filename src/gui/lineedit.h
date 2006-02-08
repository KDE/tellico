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

#ifndef TELLICO_GUILINEEDIT_H
#define TELLICO_GUILINEEDIT_H

#include <klineedit.h>

#include <qlabel.h>

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class LineEdit : public KLineEdit {
Q_OBJECT

public:
  LineEdit(QWidget* parent = 0, const char* name = 0);

  virtual void setText(const QString& text);
  void setHint(const QString& hint);

public slots:
  void clear();

protected:
  virtual void focusInEvent(QFocusEvent* event);
  virtual void focusOutEvent(QFocusEvent* event);
  virtual void drawContents(QPainter* painter);

private:
  QString m_hint;
  bool m_drawHint;
};

  } // end namespace
} // end namespace
#endif
