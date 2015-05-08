/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FETCH_FETCHRESULT_H
#define TELLICO_FETCH_FETCHRESULT_H

#include "../datavectors.h"

#include <QExplicitlySharedDataPointer>
#include <QString>

namespace Tellico {
  namespace Fetch {

class Fetcher;

class FetchResult {
public:
  FetchResult(QExplicitlySharedDataPointer<Fetcher> f, Data::EntryPtr entry);
  FetchResult(QExplicitlySharedDataPointer<Fetcher> f, const QString& t, const QString& d, const QString& i = QString());

  Data::EntryPtr fetchEntry();

  uint uid;
  QExplicitlySharedDataPointer<Fetcher> fetcher;
  QString title;
  QString desc;
  QString isbn;

private:
  static QString makeDescription(Data::EntryPtr entry);
};

  } // end namespace
} // end namespace

#endif
