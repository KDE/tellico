/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_FETCH_FETCHREQUEST_H
#define TELLICO_FETCH_FETCHREQUEST_H

#include "fetch.h"

#include <QString>

namespace Tellico {
  namespace Fetch {

class FetchRequest {
public:
  FetchRequest() : collectionType(0), key(FetchFirst) {}
  FetchRequest(FetchKey key_, const QString& value_) : collectionType(0), key(key_), value(value_) {}
  FetchRequest(int type_, FetchKey key_, const QString& value_) : collectionType(type_), key(key_), value(value_) {}

  bool isNull() const { return key == FetchFirst || value.isEmpty(); }

  int collectionType;
  FetchKey key;
  QString value;
};

  } // end namespace
} // end namespace

#endif
