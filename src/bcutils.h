/***************************************************************************
                                bcutils.h
                             -------------------
    begin                : Sat Jan 25 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BCUTILS_H
#define BCUTILS_H

#include "bookcase.h"
#include "bcfilterdialog.h"
#include "bcuniteditwidget.h"

#include <kdebug.h>

#include <qobject.h>

/**
 * This file contains utility functions.
 *
 * @author Robby Stephenson
 * @version $Id: bcutils.h,v 1.3 2003/05/02 06:04:21 robby Exp $
 */
inline
Bookcase* BookcaseAncestor(QObject* obj) {
  while(obj && !obj->isA("Bookcase")) {
    obj = obj->parent();
  }
  if(!obj) {
    kdWarning() << "BookcaseAncestor() - none found" << endl;
  }
  return static_cast<Bookcase*>(obj);
}

inline
BCFilterDialog* BCFilterDialogAncestor(QObject* obj) {
  while(obj && !obj->isA("BCFilterDialog")) {
    obj = obj->parent();
  }
  if(!obj) {
    kdWarning() << "BCFilterDialogAncestor() - none found" << endl;
  }
  return static_cast<BCFilterDialog*>(obj);
}

inline
BCUnitEditWidget* BCUnitEditWidgetAncestor(QObject* obj) {
  while(obj && !obj->isA("BCUnitEditWidget")) {
    obj = obj->parent();
  }
  if(!obj) {
    kdWarning() << "BCUnitEditWidgetAncestor() - none found" << endl;
  }
  return static_cast<BCUnitEditWidget*>(obj);
}

#endif
