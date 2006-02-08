/***************************************************************************
    copyright            : (C) 2002-2006 by Robby Stephenson
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

#include <qtabbar.h>
#include <qobjectlist.h>

using Tellico::GUI::TabControl;

TabControl::TabControl(QWidget* parent_, const char* name_/*=0*/)
    : QTabWidget(parent_, name_) {
}

QTabBar* TabControl::tabBar() const {
  return QTabWidget::tabBar();
}

void TabControl::setFocusToFirstChild() {
  QWidget* page = currentPage();
  QObjectList* list = page->queryList("QWidget");
  for(QObjectListIt it(*list); it.current(); ++it) {
    QWidget* w = static_cast<QWidget*>(it.current());
    if(w->isFocusEnabled()) {
      w->setFocus();
      break;
    }
  }
  delete list;
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

void TabControl::setTabBarHidden(bool hide_) {
  QWidget* rightcorner = cornerWidget(TopRight);
  QWidget* leftcorner = cornerWidget(TopLeft);

  if(hide_) {
    if(leftcorner) {
      leftcorner->hide();
    }
    if(rightcorner) {
      rightcorner->hide();
    }
    tabBar()->hide();
  } else {
    tabBar()->show();
    if(leftcorner) {
      leftcorner->show();
    }
    if(rightcorner) {
      rightcorner->show();
    }
  }
}

#include "tabcontrol.moc"
