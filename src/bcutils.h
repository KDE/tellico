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

class Bookcase;

#include <qobject.h>

/**
 * This file contains utility functions.
 *
 * @author Robby Stephenson
 * @version $Id: bcutils.h,v 1.5 2003/03/10 02:13:49 robby Exp $
 */
inline
Bookcase* bookcaseParent(QObject* obj) {
  while(obj && !obj->isA("Bookcase")) {
    obj = obj->parent();
  }
  return static_cast<Bookcase*>(obj);
}

#endif
