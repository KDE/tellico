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

#include <kdebug.h>

#include <qtabbar.h>
#include <qobjectlist.h>
#include <qlayout.h>

BCTabControl::BCTabControl(QWidget* parent_, const char* name_/*=0*/)
    : QTabWidget(parent_, name_) {
  connect(tabBar(), SIGNAL(selected(int)), SLOT(setFocusToChild(int)));
}

void BCTabControl::setFocusToChild(int tabNum_) {
  // I had some screwy errors and this fixed them, though
  // I don't know why QTabeBar::selected() would signal a non-existent page
  if(!page(tabNum_)) {
    return;
  }
  //find the first focusable widget in the visible tab
  QObjectList* list = page(tabNum_)->queryList("QWidget");
  QPtrListIterator<QObject> it(*list);
  QWidget* w;
  for( ; it.current(); ++it) {
    w = static_cast<QWidget*>(it.current());
    if(w->isFocusEnabled()) {
      w->setFocus();
      break;
    }
  }
}

void BCTabControl::clear() {
  for(int i = count(); i > 0; --i) {
    QWidget* w = page(i-1);
    if(w) {
      removePage(w);
      delete w;
    }
  }
}
