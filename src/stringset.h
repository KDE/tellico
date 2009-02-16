/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
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

#include <QSet>
#include <QStringList>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class StringSet : public QSet<QString> {

public:
  StringSet() {}

  void add(const QString& val) { if(!val.isEmpty()) insert(val); }
  void add(const QStringList& values) {
    foreach(const QString& value, values) {
      add(value);
    }
  }
  bool has(const QString& val) const { return contains(val); }
  QStringList toList() const { return QSet<QString>::toList(); }
};

}

#endif
