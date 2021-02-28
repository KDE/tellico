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
#include "utils/string_utils.h"
#include "images/imageinfo.h"
#include "images/imagefactory.h"
#include "tellico_debug.h"

#include <QRegularExpression>

#include <functional>

using Tellico::Filter;
using Tellico::FilterRule;

FilterRule::FilterRule() : m_function(FuncEquals) {
}

FilterRule::FilterRule(const QString& fieldName_, const QString& pattern_, Function func_)
    : m_fieldName(fieldName_), m_function(func_), m_pattern(pattern_) {
  updatePattern();
}

bool FilterRule::isEmpty() const {
  // FuncEquals and FuncNotEquals can match against empty string
  return m_pattern.isEmpty() && !(m_function == FuncEquals || m_function == FuncNotEquals);
}

bool FilterRule::matches(Tellico::Data::EntryPtr entry_) const {
  Q_ASSERT(entry_);
  Q_ASSERT(entry_->collection());
  if(!entry_ || !entry_->collection()) {
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
    case FuncLess:
      return lessThan(entry_);
    case FuncGreater:
      return greaterThan(entry_);
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
  } else if(entry_->collection()->hasField(m_fieldName) &&
            entry_->collection()->fieldByName(m_fieldName)->type() == Data::Field::Image) {
    // this is just for image size comparison, all other number comparisons are ok
    // falling back to the string comparison after this
    return numberCompare(entry_, std::equal_to<double>());
  } else {
    return m_pattern.compare(entry_->field(m_fieldName), Qt::CaseInsensitive) == 0 ||
           (entry_->collection()->hasField(m_fieldName) &&
            entry_->collection()->fieldByName(m_fieldName)->formatType() != FieldFormat::FormatNone &&
            m_pattern.compare(entry_->formattedField(m_fieldName, FieldFormat::ForceFormat), Qt::CaseInsensitive) == 0);
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
    const QString value = entry_->field(m_fieldName);
    if(value.contains(m_pattern, Qt::CaseInsensitive)) {
      return true;
    }
    QString value2 = removeAccents(value);
    if(value2 != value && value2.contains(m_pattern, Qt::CaseInsensitive)) {
      return true;
    }
    if(entry_->collection()->hasField(m_fieldName) &&
       entry_->collection()->fieldByName(m_fieldName)->formatType() != FieldFormat::FormatNone) {
      const QString fvalue = entry_->formattedField(m_fieldName);
      if(fvalue == value) {
        return false; // if the formatted value is equal to original value, no need to recheck
      }
      if(fvalue.contains(m_pattern, Qt::CaseInsensitive)) {
        return true;
      }
      value2 = removeAccents(fvalue);
      if(value2 != fvalue && value2.contains(m_pattern, Qt::CaseInsensitive)) {
        return true;
      }
    }
  }

  return false;
}

bool FilterRule::matchesRegExp(Tellico::Data::EntryPtr entry_) const {
  // empty field name means search all
  const QRegularExpression pattern = m_patternVariant.toRegularExpression();
  if(m_fieldName.isEmpty()) {
    foreach(const QString& value, entry_->fieldValues()) {
      if(pattern.match(value).hasMatch()) {
        return true;
      }
    }
    foreach(const QString& value, entry_->formattedFieldValues()) {
      if(pattern.match(value).hasMatch()) {
        return true;
      }
    }
  } else {
    return pattern.match(entry_->field(m_fieldName)).hasMatch() ||
           (entry_->collection()->hasField(m_fieldName) &&
            entry_->collection()->fieldByName(m_fieldName)->formatType() != FieldFormat::FormatNone &&
            pattern.match(entry_->formattedField(m_fieldName, FieldFormat::ForceFormat)).hasMatch());
  }

  return false;
}

bool FilterRule::before(Tellico::Data::EntryPtr entry_) const {
  // empty field name means search all
  // but the rule widget should limit this function to date fields only
  if(m_fieldName.isEmpty()) {
    return false;
  }
  const QDate pattern = m_patternVariant.toDate();
//  const QDate value = QDate::fromString(entry_->field(m_fieldName), Qt::ISODate);
  // Bug 361625: some older versions of Tellico serialized the date with single digit month and day
  const QDate value = QDate::fromString(entry_->field(m_fieldName), QStringLiteral("yyyy-M-d"));
  return value.isValid() && value < pattern;
}

bool FilterRule::after(Tellico::Data::EntryPtr entry_) const {
  // empty field name means search all
  // but the rule widget should limit this function to date fields only
  if(m_fieldName.isEmpty()) {
    return false;
  }
  const QDate pattern = m_patternVariant.toDate();
//  const QDate value = QDate::fromString(entry_->field(m_fieldName), Qt::ISODate);
  // Bug 361625: some older versions of Tellico serialized the date with single digit month and day
  const QDate value = QDate::fromString(entry_->field(m_fieldName), QStringLiteral("yyyy-M-d"));
  return value.isValid() && value > pattern;
}

bool FilterRule::lessThan(Tellico::Data::EntryPtr entry_) const {
  return numberCompare(entry_, std::less<double>());
}

bool FilterRule::greaterThan(Tellico::Data::EntryPtr entry_) const {
  return numberCompare(entry_, std::greater<double>());
}

void FilterRule::updatePattern() {
  if(m_function == FuncRegExp || m_function == FuncNotRegExp) {
    m_patternVariant = QRegularExpression(m_pattern, QRegularExpression::CaseInsensitiveOption);
  } else if(m_function == FuncBefore || m_function == FuncAfter)  {
    m_patternVariant = QDate::fromString(m_pattern, Qt::ISODate);
  } else if(m_function == FuncLess || m_function == FuncGreater)  {
    m_patternVariant = m_pattern.toDouble();
  } else {
    if(m_pattern.isEmpty()) {
      m_pattern = m_patternVariant.toString();
    }
    // we don't even use it
    m_patternVariant = QVariant();
  }
}

void FilterRule::setFunction(Function func_) {
  m_function = func_;
  updatePattern();
}

QString FilterRule::pattern() const {
  return m_pattern;
}

template <typename Func>
bool FilterRule::numberCompare(Tellico::Data::EntryPtr entry_, Func func) const {
  // empty field name means search all
  // but the rule widget should limit this function to number fields only
  if(m_fieldName.isEmpty()) {
    return false;
  }

  bool ok = false;
  const QString valueString = entry_->field(m_fieldName);
  double value;
  if(entry_->collection()->hasField(m_fieldName) &&
     entry_->collection()->fieldByName(m_fieldName)->type() == Data::Field::Image) {
    ok = true;
    const Data::ImageInfo info = ImageFactory::imageInfo(valueString);
    // image size comparison presumes "fitting inside a box" so
    // consider the pattern value to be the size of the square and compare against biggest dimension
    // an empty empty (null info) should match against 0 size
    value = info.isNull() ? 0 : qMax(info.width(), info.height());
  } else {
    value = valueString.toDouble(&ok);
  }
  // the equal compare is assumed to use the pattern string and the variant will be empty
  // TODO: switch to using the variant for everything
  return ok && func(value, m_patternVariant.isNull() ? m_pattern.toDouble()
                                                     : m_patternVariant.toDouble());
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
      match = true;
      if(m_op == Filter::MatchAny) {
        break; // don't need to check other rules
      }
    } else {
      match = false;
      if(m_op == Filter::MatchAll) {
        break; // no need to check further
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
