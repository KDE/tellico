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

#include "tellico_utils.h"

#include <qregexp.h>

QString Tellico::decodeHTML(const QString& s) {
  QRegExp rx(QString::fromLatin1("&#(\\d+);"));
  QString out = s;
  int pos = rx.search(out);
  while(pos > -1) {
    out.replace(pos, rx.cap(0).length(), static_cast<char>(rx.cap(1).toInt()));
    pos = rx.search(out, pos+1);
  }
  return out;
}
