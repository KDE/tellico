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

#ifndef TELLICO_ENTRYCOMPARISON_H
#define TELLICO_ENTRYCOMPARISON_H

#include "datavectors.h"

#include <QUrl>

namespace Tellico {

class EntryComparison {
public:
  /**
   * Set the url for comparing relative url field values
   * this is totally not the way the comparison should be done, but it's too expensive to include
   * a connection to document.h here
   */
  static void setDocumentUrl(const QUrl& url);

  static int score(const Data::EntryPtr& entry1, const Data::EntryPtr& entry2, Data::FieldPtr field);
  static int score(const Data::EntryPtr& entry1, const Data::EntryPtr& entry2, const QString& field, const Data::Collection* coll);

  // match scores for individual fields
  enum MatchValue {
    MATCH_VALUE_NONE   = 0,
    MATCH_VALUE_WEAK   = 3,
    MATCH_VALUE_STRONG = 5
  };

  // weights for which individual fields get marked
  enum MatchWeight {
    MATCH_WEIGHT_LOW  = 1,
    MATCH_WEIGHT_MED  = 2,
    MATCH_WEIGHT_HIGH = 3
  };

  // these are the total values that should be compared against
  // the result from Collection::sameEntry()  ../entrycomparison.cpp
  enum EntryMatchValue {
    ENTRY_BAD_MATCH     = 0,
    ENTRY_GOOD_MATCH    = 10,
    ENTRY_PERFECT_MATCH = 20
  };

private:
  static QUrl s_documentUrl;
};

} // namespace
#endif
