/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef UTILS_H
#define UTILS_H

#include <kdeversion.h>
#ifndef KDE_MAKE_VERSION
#define KDE_MAKE_VERSION( a,b,c ) (((a) << 16) | ((b) << 8) | (c))
#endif

#ifndef KDE_VERSION
#define KDE_VERSION \
  KDE_MAKE_VERSION(KDE_VERSION_MAJOR,KDE_VERSION_MINOR,KDE_VERSION_RELEASE)
#endif

#ifndef KDE_IS_VERSION
#define KDE_IS_VERSION(a,b,c) ( KDE_VERSION >= KDE_MAKE_VERSION(a,b,c) )
#endif

#ifndef NDEBUG
#include <kdebug.h>
#endif

#include <qobject.h>

/**
 * This file contains utility functions.
 *
 * @author Robby Stephenson
 * @version $Id: utils.h 391 2004-01-24 06:27:14Z robby $
 */
inline
QObject* QObjectAncestor(QObject* obj, const char* className_) {
  while(obj && !obj->isA(className_)) {
    obj = obj->parent();
  }
#ifndef NDEBUG
  if(!obj) {
    kdWarning() << "QObjectAncestor() - none found of class: " << className_ << endl;
  }
#endif
  return obj;
}
#endif
