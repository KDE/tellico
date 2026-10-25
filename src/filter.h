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

#ifndef TELLICO_FILTER_H
#define TELLICO_FILTER_H

#include "datavectors.h"

#include <QList>
#include <QString>
#include <QVariant>

namespace Tellico {

class FilterRule;

/**
 * Borrows from KMSearchPattern by Marc Mutz
 *
 * @author Robby Stephenson
 */
class Filter {

public:
  enum FilterOp {
    MatchAny,
    MatchAll
  };

  Filter(FilterOp op);
  Filter(const Filter& other);
  ~Filter();

  void setMatch(FilterOp op);
  FilterOp op() const;
  void setName(const QString& name);
  QString name() const;
  QList<FilterRule*> rules() const;

  int ruleCount() const;
  void append(FilterRule* rule);
  FilterRule* rule(qsizetype i) const;
  void clear();

  bool matches(Tellico::Data::EntryPtr entry) const;

  bool hasSameRules(const Filter& other) const;
  bool operator==(const Filter& other) const;

private:
  class Data;
  QSharedDataPointer<Data> d;
};

} // end namespace
#endif
