/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_MAP_H
#define TELLICO_MAP_H

#include <qmap.h>

/**
 * This file contains some template functions for maps
 *
 * @author Robby Stephenson
 */
namespace Tellico {

/**
 * Reverse a map's keys and values
 */
template<class M>
QMap<typename M::T, typename M::Key> flipMap(const M& map_) {
  QMap<typename M::T, typename M::Key> map;
  typename M::ConstIterator it = map_.begin(), end = map_.end();
  for( ; it != end; ++it) {
    map.insert(it.data(), it.key());
  }
  return map;
}

}

#endif
