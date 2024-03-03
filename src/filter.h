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

/**
 * @author Robby Stephenson
 */
class FilterRule {

public:
  /**
   * Operators for comparison of field and contents.
   * If you change the order or contents of the enum: do not forget
   * to change matches() and @ref FilterRuleWidget::initLists(), too.
   */
  enum Function {
    FuncContains=0, FuncNotContains,
    FuncEquals, FuncNotEquals,
    FuncRegExp, FuncNotRegExp,
    FuncBefore, FuncAfter,
    FuncLess, FuncGreater
  };

  FilterRule();
  FilterRule(const QString& fieldName, const QString& text, Function func);

  /**
   * A rule is empty if the pattern text is empty and we're not matching an equal string
   */
  bool isEmpty() const;
  /**
   * This is the primary function of the rule.
   *
   * @return Returns true if the entry is matched by the rule.
   */
  bool matches(Tellico::Data::EntryPtr entry) const;

  /**
   * Return filter function. This can be any of the operators
   * defined in @ref Function.
   */
  Function function() const { return m_function; }
  /**
   * Set filter function.
   */
  void setFunction(Function func);
  /**
   * Return field name
   */
  const QString& fieldName() const { return m_fieldName; }
  /**
   * Set field name
   */
  void setFieldName(const QString& fieldName) { m_fieldName = fieldName; }
  /**
   * Return pattern
   */
  QString pattern() const;
  /**
   * Set pattern
   */
//  void setPattern(const QString& pattern) { m_pattern = pattern; }

private:
  template <typename Func>
  bool numberCompare(Tellico::Data::EntryPtr entry, Func f) const;

  bool equals(Data::EntryPtr entry) const;
  bool contains(Data::EntryPtr entry) const;
  bool matchesRegExp(Data::EntryPtr entry) const;
  bool before(Data::EntryPtr entry) const;
  bool after(Data::EntryPtr entry) const;
  bool lessThan(Data::EntryPtr entry) const;
  bool greaterThan(Data::EntryPtr entry) const;
  void updatePattern();

  QString m_fieldName;
  Function m_function;
  QString m_pattern;
  QVariant m_patternVariant;
};

/**
 * Borrows from KMSearchPattern by Marc Mutz
 *
 * @author Robby Stephenson
 */
class Filter : public QList<FilterRule*>, public QSharedData {

public:
  enum FilterOp {
    MatchAny,
    MatchAll
  };

  Filter(FilterOp op) : QList<FilterRule*>(), m_op(op) {}
  Filter(const Filter& other);
  ~Filter();

  void setMatch(FilterOp op) { m_op = op; }
  FilterOp op() const { return m_op; }
  bool matches(Tellico::Data::EntryPtr entry) const;

  void setName(const QString& name) { m_name = name; }
  const QString& name() const { return m_name; }

  int count() const { return QList<FilterRule*>::count(); } // disambiguate

  bool operator==(const Filter& other) const;

  static void populateQuickFilter(FilterPtr filter, const QString& fieldName, const QString& text, bool allowRegExp);

private:
  Filter& operator=(const Filter& other);

  FilterOp m_op;
  QString m_name;
};

} // end namespace
#endif
