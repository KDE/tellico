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

#include "filter.h"
#include "filterrule.h"
#include "entry.h"
#include "utils/string_utils.h"
#include "tellico_debug.h"

using Tellico::Filter;

class Q_DECL_HIDDEN Filter::Data : public QSharedData {
public:
  FilterOp op;
  QString name;
  QList<FilterRule*> rules;

  Data() {}
  ~Data() {
    qDeleteAll(rules);
    rules.clear();
  }
};

Filter::Filter(FilterOp op_) : d(new Data) {
  d->op = op_;
}

Filter::Filter(const Filter& other_) : d(other_.d) {
  d.detach();
  d->rules.clear();
  foreach(const FilterRule* rule, other_.d->rules) {
    d->rules.append(new FilterRule(*rule));
  }
}

Filter::~Filter() = default;

void Filter::setMatch(FilterOp op) {
  d->op = op;
}

Tellico::Filter::FilterOp Filter::op() const {
  return d->op;
}

void Filter::setName(const QString& name_) {
  d->name = name_;
}

QString Filter::name() const {
  return d->name;
}

QList<Tellico::FilterRule*> Filter::rules() const {
  return d->rules;
}

int Filter::ruleCount() const {
  return d->rules.count();
}

void Filter::append(FilterRule* rule_) {
  d->rules.append(rule_);
}

Tellico::FilterRule* Filter::rule(qsizetype i) const {
  return d->rules.at(i);
}

void Filter::clear() {
  d->rules.clear();
}

bool Filter::matches(Tellico::Data::EntryPtr entry_) const {
  if(d->rules.isEmpty()) {
    return true;
  }

  bool match = false;
  for(auto rule : d->rules) {
    if(rule->matches(entry_)) {
      match = true;
      if(d->op == Filter::MatchAny) {
        break; // don't need to check other rules
      }
    } else {
      match = false;
      if(d->op == Filter::MatchAll) {
        break; // no need to check further
      }
    }
  }
  return match;
}

bool Filter::hasSameRules(const Filter& other_) const {
  // don't compare rule count since could have duplicate ones
  if(op() != other_.op()) {
    return false;
  }

  QVector<bool> matched(other_.ruleCount(), false);
  for(int i = 0; i < ruleCount(); ++i) {
    const FilterRule* rule1 = rule(i);
    bool found = false;

    for(int j = 0; j < other_.ruleCount(); ++j) {
      if(matched.at(j)) {
        continue;
      }

      const FilterRule* rule2 = other_.rule(j);
      if(rule1 && rule2 && *rule1 == *rule2) {
        matched[j] = true;
        found = true;
        break;
      }
    }

    if(!found) {
      return false;
    }
  }

  return true;
}

bool Filter::operator==(const Filter& other) const {
  return d->op == other.d->op &&
         d->name == other.d->name &&
         d->rules == other.d->rules;
}
