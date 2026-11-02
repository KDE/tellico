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

#include "filterrule.h"
#include "entry.h"
#include "utils/string_utils.h"
#include "images/imageinfo.h"
#include "images/imagefactory.h"
#include "tellico_debug.h"

#include <QRegularExpression>

#include <functional>

using Tellico::FilterRule;

class Q_DECL_HIDDEN FilterRule::Data : public QSharedData {
public:
  QString fieldName;
  Function func{FuncEquals};
  QString pattern;
  QVariant patternVariant;

  Data() = default;
  Data(const Data&) = default;
  ~Data() = default;
};

FilterRule::FilterRule() : d(new Data) {
}

FilterRule::FilterRule(const FilterRule& other_)
  : d(other_.d) {
}

FilterRule& FilterRule::operator=(const FilterRule& other_) {
  d = other_.d;
  return *this;
}

FilterRule::FilterRule(const QString& fieldName_, const QString& pattern_, Function func_)
    : d(new Data) {
  d->fieldName = fieldName_;
  d->func = func_;
  d->pattern = pattern_;
  updatePattern();
}

FilterRule::~FilterRule() = default;

bool FilterRule::isEmpty() const {
  // FuncEquals and FuncNotEquals can match against empty string
  return d->pattern.isEmpty() && !(d->func == FuncEquals || d->func == FuncNotEquals);
}

bool FilterRule::matches(Tellico::Data::EntryPtr entry_) const {
  Q_ASSERT(entry_);
  Q_ASSERT(entry_->collection());
  if(!entry_ || !entry_->collection()) {
    return false;
  }
  switch (d->func) {
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
  if(d->fieldName.isEmpty()) {
    foreach(const QString& value, entry_->fieldValues()) {
      if(d->pattern.compare(value, Qt::CaseInsensitive) == 0) {
        return true;
      }
    }
    foreach(const QString& value, entry_->formattedFieldValues()) {
      if(d->pattern.compare(value, Qt::CaseInsensitive) == 0) {
        return true;
      }
    }
  } else if(entry_->collection()->hasField(d->fieldName) &&
            entry_->collection()->fieldByName(d->fieldName)->type() == Tellico::Data::Field::Image) {
    // this is just for image size comparison, all other number comparisons are ok
    // falling back to the string comparison after this
    return numberCompare(entry_, std::equal_to<double>());
  } else {
    return d->pattern.compare(entry_->field(d->fieldName), Qt::CaseInsensitive) == 0 ||
           (entry_->collection()->hasField(d->fieldName) &&
            entry_->collection()->fieldByName(d->fieldName)->formatType() != FieldFormat::FormatNone &&
            d->pattern.compare(entry_->formattedField(d->fieldName, FieldFormat::ForceFormat), Qt::CaseInsensitive) == 0);
  }

  return false;
}

bool FilterRule::contains(Tellico::Data::EntryPtr entry_) const {
  // empty field name means search all
  if(d->fieldName.isEmpty()) {
    QString value2;
    // match is true if any strings match
    foreach(const QString& value, entry_->fieldValues()) {
      if(value.contains(d->pattern, Qt::CaseInsensitive)) {
        return true;
      }
      value2 = removeAccents(value);
      if(value2 != value && value2.contains(d->pattern, Qt::CaseInsensitive)) {
        return true;
      }
    }
    // match is true if any strings match
    foreach(const QString& value,entry_->formattedFieldValues()) {
      if(value.contains(d->pattern, Qt::CaseInsensitive)) {
        return true;
      }
      value2 = removeAccents(value);
      if(value2 != value && value2.contains(d->pattern, Qt::CaseInsensitive)) {
        return true;
      }
    }
  } else {
    const QString value = entry_->field(d->fieldName);
    if(value.contains(d->pattern, Qt::CaseInsensitive)) {
      return true;
    }
    QString value2 = removeAccents(value);
    if(value2 != value && value2.contains(d->pattern, Qt::CaseInsensitive)) {
      return true;
    }
    if(entry_->collection()->hasField(d->fieldName) &&
       entry_->collection()->fieldByName(d->fieldName)->formatType() != FieldFormat::FormatNone) {
      const QString fvalue = entry_->formattedField(d->fieldName);
      if(fvalue == value) {
        return false; // if the formatted value is equal to original value, no need to recheck
      }
      if(fvalue.contains(d->pattern, Qt::CaseInsensitive)) {
        return true;
      }
      value2 = removeAccents(fvalue);
      if(value2 != fvalue && value2.contains(d->pattern, Qt::CaseInsensitive)) {
        return true;
      }
    }
  }

  return false;
}

bool FilterRule::matchesRegExp(Tellico::Data::EntryPtr entry_) const {
  // empty field name means search all
  const QRegularExpression pattern = d->patternVariant.toRegularExpression();
  if(d->fieldName.isEmpty()) {
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
    return pattern.match(entry_->field(d->fieldName)).hasMatch() ||
           (entry_->collection()->hasField(d->fieldName) &&
            entry_->collection()->fieldByName(d->fieldName)->formatType() != FieldFormat::FormatNone &&
            pattern.match(entry_->formattedField(d->fieldName, FieldFormat::ForceFormat)).hasMatch());
  }

  return false;
}

bool FilterRule::before(Tellico::Data::EntryPtr entry_) const {
  // empty field name means search all
  // but the rule widget should limit this function to date fields only
  if(d->fieldName.isEmpty()) {
    return false;
  }
  const QDate pattern = d->patternVariant.toDate();
//  const QDate value = QDate::fromString(entry_->field(d->fieldName), Qt::ISODate);
  // Bug 361625: some older versions of Tellico serialized the date with single digit month and day
  const QDate value = QDate::fromString(entry_->field(d->fieldName), QStringLiteral("yyyy-M-d"));
  return value.isValid() && value < pattern;
}

bool FilterRule::after(Tellico::Data::EntryPtr entry_) const {
  // empty field name means search all
  // but the rule widget should limit this function to date fields only
  if(d->fieldName.isEmpty()) {
    return false;
  }
  const QDate pattern = d->patternVariant.toDate();
//  const QDate value = QDate::fromString(entry_->field(d->fieldName), Qt::ISODate);
  // Bug 361625: some older versions of Tellico serialized the date with single digit month and day
  const QDate value = QDate::fromString(entry_->field(d->fieldName), QStringLiteral("yyyy-M-d"));
  return value.isValid() && value > pattern;
}

bool FilterRule::lessThan(Tellico::Data::EntryPtr entry_) const {
  return numberCompare(entry_, std::less<double>());
}

bool FilterRule::greaterThan(Tellico::Data::EntryPtr entry_) const {
  return numberCompare(entry_, std::greater<double>());
}

void FilterRule::updatePattern() {
  if(d->func == FuncRegExp || d->func == FuncNotRegExp) {
    d->patternVariant = QRegularExpression(d->pattern, QRegularExpression::CaseInsensitiveOption);
  } else if(d->func == FuncBefore || d->func == FuncAfter)  {
    d->patternVariant = QDate::fromString(d->pattern, Qt::ISODate);
  } else if(d->func == FuncLess || d->func == FuncGreater)  {
    d->patternVariant = d->pattern.toDouble();
  } else {
    if(d->pattern.isEmpty()) {
      d->pattern = d->patternVariant.toString();
    }
    // we don't even use it
    d->patternVariant = QVariant();
  }
}

FilterRule::Function FilterRule::function() const {
  return d->func;
}

void FilterRule::setFunction(Function func_) {
  d->func = func_;
  updatePattern();
}

QString FilterRule::fieldName() const {
  return d->fieldName;
}

void FilterRule::setFieldName(const QString& fieldName_) {
  d->fieldName = fieldName_;
}

QString FilterRule::pattern() const {
  return d->pattern;
}

bool FilterRule::operator==(const FilterRule& other_) const {
  return fieldName() == other_.fieldName() &&
         function() == other_.function() &&
         pattern() == other_.pattern();
}

bool FilterRule::operator!=(const FilterRule& other_) const {
  return !(*this == other_);
}

template <typename Func>
bool FilterRule::numberCompare(Tellico::Data::EntryPtr entry_, Func func) const {
  // empty field name means search all
  // but the rule widget should limit this function to number fields only
  if(d->fieldName.isEmpty()) {
    return false;
  }

  bool ok = false;
  const QString valueString = entry_->field(d->fieldName);
  double value;
  if(entry_->collection()->hasField(d->fieldName) &&
     entry_->collection()->fieldByName(d->fieldName)->type() == Tellico::Data::Field::Image) {
    ok = true;
    const auto info = ImageFactory::imageInfo(valueString);
    // image size comparison presumes "fitting inside a box" so
    // consider the pattern value to be the size of the square and compare against biggest dimension
    // an empty empty (null info) should match against 0 size
    value = info.isNull() ? 0 : qMax(info.width(), info.height());
  } else {
    value = valueString.toDouble(&ok);
  }
  // the equal compare is assumed to use the pattern string and the variant will be empty
  // TODO: switch to using the variant for everything
  return ok && func(value, d->patternVariant.isNull() ? d->pattern.toDouble()
                                                      : d->patternVariant.toDouble());
}
