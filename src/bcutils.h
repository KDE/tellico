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

#include <kdebug.h>

/**
 * This file contains utility functions.
 *
 * @author Robby Stephenson
 * @version $Id: bcutils.h 200 2003-10-14 00:21:13Z robby $
 */
inline
QObject* QObjectAncestor(QObject* obj, const char* className_) {
  while(obj && !obj->isA(className_)) {
    obj = obj->parent();
  }
  if(!obj) {
    kdWarning() << "QObjectAncestor() - none found of class: " << className_ << endl;
  }
  return obj;
}
#endif
