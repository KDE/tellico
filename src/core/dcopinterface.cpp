/***************************************************************************
    copyright            : (C) 2004-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "dcopinterface.h"

Tellico::Import::Action Tellico::DCOPInterface::actionType(const QString& actionName) {
  QString name = actionName.lower();
  if(name == QString::fromLatin1("append")) {
    return Import::Append;
  } else if(name == QString::fromLatin1("merge")) {
    return Import::Merge;
  }
  return Import::Replace;
}
