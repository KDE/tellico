/***************************************************************************
                             bctabcontrol.cpp
                             -------------------
    begin                : Sun Jan 6 2002
    copyright            : (C) 2002 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bctabcontrol.h"

#include <qobjectlist.h>

BCTabControl::BCTabControl(QWidget* parent_, const char* name_/*=0*/)
    : KTabCtl(parent_, name_) {
}

void BCTabControl::showTab(int i) {
  KTabCtl::showTab(i);
}

// TODO: fix this
void BCTabControl::setFocusToChild(int tabNum_) {
  //find the first focusable child in the visible tab
  QPtrListIterator<QObject> it(*pages[tabNum_]->children());
  QWidget* w;
  for( ; it.current(); ++it) {
    w = static_cast<QWidget*>(it.current());
    // for some reason isFocusEnabled() crashes things
//    if(w->isFocusEnabled()) {
    if(w->focusPolicy() == QWidget::TabFocus
        || w->focusPolicy() == QWidget::ClickFocus
        || w->focusPolicy() == QWidget::StrongFocus) {
      //w->setFocus();
      break;
    }
  }
}

int BCTabControl::count() {
  return tabs->count();
}
