/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
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
#include <qstringlist.h>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class StringSet {

public:
  StringSet(int size = 17) : m_dict(size) {}

  // replace instead of insert, to ensure unique keys
  void add(const QString& val) { if(!val.isEmpty()) m_dict.replace(val, reinterpret_cast<const int *>(1)); }
  void add(const QStringList& vals) {
    for(QStringList::ConstIterator it = vals.begin(), end = vals.end(); it != end; ++it) {
      add(*it);
    }
  }
  void remove(const QString& val) { if(!val.isEmpty()) m_dict.remove(val); }
  void clear() { m_dict.clear(); }
  bool has(const QString& val) const { return !val.isEmpty() && (m_dict.find(val) != 0); }
  bool isEmpty() const { return m_dict.isEmpty(); }
  uint count() const { return m_dict.count(); }

  QStringList toList() const {
    QStringList list;
    for(QDictIterator<int> it(m_dict); it.current(); ++it) {
      list << it.currentKey();
    }
    return list;
  }

private:
  // use a dict for fast random access to keep track of the values
  QDict<int> m_dict;
};

}

#endif
