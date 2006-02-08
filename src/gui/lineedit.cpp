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

#include "lineedit.h"

#include <kdebug.h>

#include <qapplication.h>
#include <qpainter.h>

using Tellico::GUI::LineEdit;

LineEdit::LineEdit(QWidget* parent_, const char* name_) : KLineEdit(parent_, name_), m_drawHint(false) {
}

void LineEdit::clear() {
  KLineEdit::clear();
  m_drawHint = true;
  repaint();
}

void LineEdit::setText(const QString& text_) {
  m_drawHint = text_.isEmpty();
  repaint();
  KLineEdit::setText(text_);
}

void LineEdit::setHint(const QString& hint_) {
  m_hint = hint_;
  m_drawHint = text().isEmpty();
  repaint();
}

void LineEdit::focusInEvent(QFocusEvent* event_) {
  if(m_drawHint) {
    m_drawHint = false;
    repaint();
  }
  KLineEdit::focusInEvent(event_);
}

void LineEdit::focusOutEvent(QFocusEvent* event_) {
  if(text().isEmpty()) {
    m_drawHint = true;
    repaint();
  }
  KLineEdit::focusOutEvent(event_);
}

void LineEdit::drawContents(QPainter* painter_) {
  // draw the regular line edit first
  KLineEdit::drawContents(painter_);

  // no need to draw anything else if in focus or no hint
  if(hasFocus() || !m_drawHint || m_hint.isEmpty() || !text().isEmpty()) {
    return;
  }

  // save current pen
  QPen oldPen = painter_->pen();

  // follow lead of kdepim and amarok, use disabled text color
  painter_->setPen(palette().color(QPalette::Disabled, QColorGroup::Text));

  QRect rect = contentsRect();
  // again, follow kdepim and amarok lead, and pad by 2 pixels
  rect.rLeft() += 2;
  painter_->drawText(rect, AlignAuto | AlignVCenter, m_hint);

  // reset pen
  painter_->setPen(oldPen);
}

#include "lineedit.moc"
