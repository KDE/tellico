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
#include "entry.h"
#include "tellico_utils.h"
#include "tellico_debug.h"

#include <QRegExp>

using Tellico::Filter;
using Tellico::FilterRule;

FilterRule::FilterRule() : m_function(FuncEquals) {
}

FilterRule::FilterRule(const QString& fieldName_, const QString& pattern_, Function func_)
    : m_fieldName(fieldName_), m_function(func_), m_pattern(pattern_) {
  if(m_function == FuncRegExp || m_function == FuncNotRegExp) {
    m_patternRx = QRegExp(m_pattern, Qt::CaseInsensitive);
  }
}

bool FilterRule::matches(Tellico::Data::EntryPtr entry_) const {
  Q_ASSERT(entry_);
  if(!entry_) {
    return false;
  }
  switch (m_function) {
    case FuncEquals:
      return equals(entry_);
    case FuncNotEquals:
      return !equals(entry_);
    case FuncContains:
      return contains(entry_);
    case FuncNotContains:
      return !contains(entry_);
    case FuncRegExp:
      return matchesRegExp(entry_);
    case FuncNotRegExp:
      return !matchesRegExp(entry_);
    case FuncBefore:
      return before(entry_);
    case FuncAfter:
      return after(entry_);
    default:
      myWarning() << "invalid function!";
      break;
  }
  return false;
}

bool FilterRule::equals(Tellico::Data::EntryPtr entry_) const {
  // empty field name means search all
  if(m_fieldName.isEmpty()) {
    foreach(const QString& value, entry_->fieldValues()) {
      if(m_pattern.compare(value, Qt::CaseInsensitive) == 0) {
        return true;
      }
    }
    foreach(const QString& value, entry_->formattedFieldValues()) {
      if(m_pattern.compare(value, Qt::CaseInsensitive) == 0) {
        return true;
      }
    }
  } else {
    return m_pattern.compare(entry_->field(m_fieldName), Qt::CaseInsensitive) == 0 ||
           m_pattern.compare(entry_->formattedField(m_fieldName, FieldFormat::ForceFormat), Qt::CaseInsensitive) == 0;
  }

  return false;
}

bool FilterRule::contains(Tellico::Data::EntryPtr entry_) const {
  // empty field name means search all
  if(m_fieldName.isEmpty()) {
    QString value2;
    // match is true if any strings match
    foreach(const QString& value, entry_->fieldValues()) {
      if(value.contains(m_pattern, Qt::CaseInsensitive)) {
        return true;
      }
      value2 = removeAccents(value);
      if(value2 != value && value2.contains(m_pattern, Qt::CaseInsensitive)) {
        return true;
      }
    }
    // match is true if any strings match
    foreach(const QString& value, entry_->formattedFieldValues()) {
      if(value.contains(m_pattern, Qt::CaseInsensitive)) {
        return true;
      }
      value2 = removeAccents(value);
      if(value2 != value && value2.contains(m_pattern, Qt::CaseInsensitive)) {
        return true;
      }
    }
  } else {
    QString value = entry_->field(m_fieldName);
    if(value.contains(m_pattern, Qt::CaseInsensitive)) {
        return true;
    }
    QString value2 = removeAccents(value);
    if(value2 != value && value2.contains(m_pattern, Qt::CaseInsensitive)) {
      return true;
    }
    value = entry_->formattedField(m_fieldName);
    if(value.contains(m_pattern, Qt::CaseInsensitive)) {
      return true;
    }
    value2 = removeAccents(value);
    if(value2 != value && value2.contains(m_pattern, Qt::CaseInsensitive)) {
      return true;
    }
  }

  return false;
}

bool FilterRule::matchesRegExp(Tellico::Data::EntryPtr entry_) const {
  // empty field name means search all
  if(m_fieldName.isEmpty()) {
    foreach(const QString& value, entry_->fieldValues()) {
      if(m_patternRx.indexIn(value) >= 0) {
        return true;
      }
    }
    foreach(const QString& value, entry_->formattedFieldValues()) {
      if(m_patternRx.indexIn(value) >= 0) {
        return true;
      }
    }
  } else {
    return m_patternRx.indexIn(entry_->field(m_fieldName)) >= 0 ||
           m_patternRx.indexIn(entry_->formattedField(m_fieldName, FieldFormat::ForceFormat)) >= 0;
  }

  return false;
}

bool FilterRule::before(Tellico::Data::EntryPtr entry_) const {
  // empty field name means search all
  // but the rule widget should limit this function to date fields only
  if(m_fieldName.isEmpty()) {
    return false;
  }
  const QDate pattern = QDate::fromString(m_pattern, Qt::ISODate);
  const QDate value = QDate::fromString(entry_->field(m_fieldName), Qt::ISODate);
  return value.isValid() && value < pattern;
}

bool FilterRule::after(Tellico::Data::EntryPtr entry_) const {
  // empty field name means search all
  // but the rule widget should limit this function to date fields only
  if(m_fieldName.isEmpty()) {
    return false;
  }
  const QDate pattern = QDate::fromString(m_pattern, Qt::ISODate);
  const QDate value = QDate::fromString(entry_->field(m_fieldName), Qt::ISODate);
  return value.isValid() && value > pattern;
}

void FilterRule::setFunction(Function func_) {
  m_function = func_;
  if(m_function == FuncRegExp || m_function == FuncNotRegExp) {
    m_patternRx = QRegExp(m_pattern, Qt::CaseInsensitive);
  }
}

/*******************************************************/

Filter::Filter(const Filter& other_) : QList<FilterRule*>(), QSharedData()
    , m_op(other_.op())
    , m_name(other_.name()) {
  foreach(const FilterRule* rule, static_cast<const QList<FilterRule*>&>(other_)) {
    append(new FilterRule(*rule));
  }
}

Filter::~Filter() {
  qDeleteAll(*this);
  clear();
}

bool Filter::matches(Tellico::Data::EntryPtr entry_) const {
  if(isEmpty()) {
    return true;
  }

  bool match = false;
  foreach(const FilterRule* rule, *this) {
    if(rule->matches(entry_)) {
      if(m_op == Filter::MatchAny) {
        return true;
      } else {
        match = true;
      }
    } else {
      if(m_op == Filter::MatchAll) {
        return false;
      }
    }
  }

  return match;
}

bool Filter::operator==(const Filter& other) const {
  return m_op == other.m_op &&
         m_name == other.m_name &&
         *static_cast<const QList<FilterRule*>*>(this) == static_cast<const QList<FilterRule*>&>(other);
}
