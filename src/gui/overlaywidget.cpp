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

#include "overlaywidget.h"

#include <qlayout.h>

using Tellico::GUI::OverlayWidget;

OverlayWidget::OverlayWidget(QWidget* parent, QWidget* anchor) : QFrame(parent)
    , m_anchor(anchor)
    , m_corner(TopRight) {
  m_anchor->installEventFilter(this);
  reposition();
  hide();
}

void OverlayWidget::setCorner(Corner corner_) {
  if(corner_ == m_corner) {
    return;
  }
  m_corner = corner_;
  reposition();
}

void OverlayWidget::addWidget(QWidget* widget_) {
  layout()->add(widget_);
  adjustSize();
}

void OverlayWidget::reposition() {
  if(!m_anchor) {
    return;
  }

  setMaximumSize(parentWidget()->size());
  adjustSize();

  QPoint p;

  switch(m_corner) {
    case BottomLeft:
      p.setX(0);
      p.setY(m_anchor->height());
      break;

    case BottomRight:
      p.setX(m_anchor->width() - width());
      p.setY(m_anchor->height());
      break;

    case TopLeft:
      p.setX(0);
      p.setY(-1 * height());
      break;

    case TopRight:
      p.setX(m_anchor->width() - width());
      p.setY(-1 * height());
  }

  // Position in the toplevelwidget's coordinates
  QPoint pTopLevel = m_anchor->mapTo(topLevelWidget(), p);
  // Position in the widget's parentWidget coordinates
  QPoint pParent = parentWidget()->mapFrom(topLevelWidget(), pTopLevel);
  // keep it on the screen
  if(pParent.x() < 0) {
    pParent.rx() = 0;
  }
  move(pParent);
}

bool OverlayWidget::eventFilter(QObject* object_, QEvent* event_) {
  if(object_ == m_anchor && (event_->type() == QEvent::Move || event_->type() == QEvent::Resize)) {
    reposition();
  }

  return QFrame::eventFilter(object_, event_);
}

void OverlayWidget::resizeEvent(QResizeEvent* event_) {
  reposition();
  QFrame::resizeEvent(event_);
}

bool OverlayWidget::event(QEvent* event_) {
  if(event_->type() == QEvent::ChildInserted) {
    adjustSize();
  }

  return QFrame::event(event_);
}

#include "overlaywidget.moc"
