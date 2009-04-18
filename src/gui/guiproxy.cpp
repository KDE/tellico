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

#include "guiproxy.h"
#include "cursorsaver.h"

#include <KMessageBox>

#include <QWidget>
#include <QMetaObject>

using Tellico::GUI::Proxy;

QWidget* Proxy::s_widget = 0;

void Proxy::setMainWidget(QWidget* widget_) {
  s_widget = widget_;
}

QWidget* Proxy::widget() {
  return s_widget;
}

void Proxy::sorry(const QString& text_, QWidget* widget_/* =0 */) {
  if(text_.isEmpty()) {
    return;
  }
  GUI::CursorSaver cs(Qt::ArrowCursor);
  KMessageBox::sorry(widget_ ? widget_ : s_widget, text_);
}

