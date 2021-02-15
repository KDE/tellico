/***************************************************************************
    Copyright (C) 2009-2021 Robby Stephenson <robby@periapsis.org>
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
  FetchRequest();
  FetchRequest(FetchKey key, const QString& value);
  FetchRequest(int type, FetchKey key, const QString& value);

  bool isNull() const;

  int collectionType() const { return m_collectionType; }
  void setCollectionType(int type) { m_collectionType = type; }

  FetchKey key() const { return m_key; }
  QString value() const { return m_value; }

private:
  int m_collectionType;
  FetchKey m_key;
  QString m_value;
};

  } // end namespace
} // end namespace

#endif
