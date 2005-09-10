/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_STRINGSET_H
#define TELLICO_STRINGSET_H

#include <qdict.h>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class StringSet {

public:
  StringSet(int size = 17) : m_dict(size) {}

  void add(const QString& value) { m_dict.insert(value, reinterpret_cast<const int *>(1)); }
  void clear() { m_dict.clear(); }
  bool has(const QString& value) const { return (m_dict.find(value) != 0); }

private:
  // use a dict for fast random access to keep track of the values
  QDict<int> m_dict;
};

}

#endif
