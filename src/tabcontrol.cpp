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

#include "tabcontrol.h"

#include <kapplication.h>

#include <qtabbar.h>
#include <qobjectlist.h>

using Tellico::TabControl;

TabControl::TabControl(QWidget* parent_, const char* name_/*=0*/)
    : QTabWidget(parent_, name_), m_resetFocus(true) {
  connect(this, SIGNAL(currentChanged(QWidget*)), SLOT(slotUpdateFocus(QWidget*)));
}

QTabBar* TabControl::tabBar() {
  return QTabWidget::tabBar();
}

void TabControl::setFocusToFirstChild(QWidget* page_) {
  QObjectList* list = page_->queryList("QWidget");
  for(QObjectListIt it(*list); it.current(); ++it) {
    QWidget* w = static_cast<QWidget*>(it.current());
    if(w->isFocusEnabled()) {
      w->setFocus();
      delete list;
      break;
    }
  }
}

void TabControl::slotUpdateFocus(QWidget* page_) {
  if(!page_) {
    return;
  }
  // if nothing is currently focused, then find the first focusable child widget and setFocus there
  if(kapp->focusWidget() == 0 || m_resetFocus) {
    setFocusToFirstChild(page_);
  }
}

// have to loop backwards since count() gets decremented on delete
void TabControl::clear() {
  for(int i = count(); i > 0; --i) {
    QWidget* w = page(i-1);
    if(w) {
      removePage(w);
      delete w;
    }
  }
}

void TabControl::setResetFocus(bool reset_) {
  m_resetFocus = reset_;
}

#include "tabcontrol.moc"
