/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

// this code was original published to the kde-core-devel email list
// copyright 2003 Harri Porten <porten@kde.org>
// Originally licensed under LGPL, included here under GPL v2

#ifndef LATIN1LITERAL_H
#define LATIN1LITERAL_H

#include <qstring.h>

namespace Tellico {

/**
 * A class for explicit marking of string literals encoded in the ISO
 * 8859-1 character set. Allows for efficient, still (in terms of the
 * chosen encoding) safe comparison with QString instances. To be used
 * like this:
 *
 * \code
 *     QString s = .....
 *     if (s == Latin1Literal("o")) { ..... }
 * \endcode
 *
 */
#define Latin1Literal(s) \
    Tellico::Latin1LiteralInternal((s), sizeof(s)/sizeof(char)-1)

class Latin1LiteralInternal {

public:
  Latin1LiteralInternal(const char* s, size_t l)
    : str(s), len(s ? l : (size_t)-1) { }

  // this is lazy, leave these public since I can't figure out
  // how to declare a friend function that works for gcc 2.95
  const char* str;
  size_t len;
};

} // end namespace

inline
bool operator==(const QString& s1, const Tellico::Latin1LiteralInternal& s2) {
  const QChar* uc = s1.unicode();
  const char* c = s2.str;
  if(!c || !uc) {
    return (!c && !uc);
  }

  const size_t& l = s2.len;
  if(s1.length() != l) {
    return false;
  }

  for(size_t i = 0; i < l; ++i, ++uc, ++c) {
    if(uc->unicode() != static_cast<uchar>(*c)) {
      return false;
    }
  }
  return true;
}

inline
bool operator!=(const QString& s1, const Tellico::Latin1LiteralInternal& s2) {
  return !(s1 == s2);
}

inline
bool operator==(const Tellico::Latin1LiteralInternal& s1, const QString& s2) {
  return s2 == s1;
}

inline
bool operator!=(const Tellico::Latin1LiteralInternal& s1, const QString& s2) {
  return !(s2 == s1);
}

#endif
