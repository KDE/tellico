/***************************************************************************
    Copyright (C) 2024 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FILTERPARSER_H
#define TELLICO_FILTERPARSER_H

#include "datavectors.h"

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class FilterParser {

public:
  FilterParser(const QString& text, bool allowRegExp=false);

  void setCollection(Tellico::Data::CollPtr coll_) { m_coll = coll_; }
  FilterPtr filter();

private:
  void parseToken(FilterPtr filter, const QString& fieldName, const QString& text);

  QString m_text;
  Data::CollPtr m_coll;
  bool m_allowRegExp;
};

} // end namespace
#endif
