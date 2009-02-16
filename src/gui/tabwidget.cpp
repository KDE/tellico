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

#include "tabwidget.h"

using Tellico::GUI::TabWidget;

TabWidget::TabWidget(QWidget* parent_)
    : KTabWidget(parent_) {
}

void TabWidget::setFocusToFirstChild() {
  QWidget* page = currentWidget();
  Q_ASSERT(page);
  QList<QWidget*> list = page->findChildren<QWidget*>();
  foreach(QWidget* w, list) {
    if(w->focusPolicy() != Qt::NoFocus) {
      w->setFocus();
      break;
    }
  }
}

#include "tabwidget.moc"
