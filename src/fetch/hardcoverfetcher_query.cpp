/***************************************************************************
    Copyright (C) 2026 Robby Stephenson <robby@periapsis.org>
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

#include "hardcoverfetcher.h"

namespace {
  static const uint HARDCOVER_MAX_RESULTS = 20;
}

using Tellico::Fetch::HardcoverFetcher;

QString HardcoverFetcher::isbnQuery() {
  static const auto query(QStringLiteral(R"(
query GetBookByISBN($isbns: [String!]!) {
  editions(where: {
    _or: [
      { isbn_10: { _in: $isbns } },
      { isbn_13: { _in: $isbns } }
    ]
  }) {
    id
    title
    subtitle
    isbn_10
    isbn_13
    edition_format
    pages
    release_date
    language {
      language
    }
    image {
      url
    }
    publisher {
      name
    }
    book {
      id
      title
      description
      contributions {
        contribution
        author {
          name
        }
      }
      image {
        url
      }
    }
  }
}
)"));
  return query;
}
