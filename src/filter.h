/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef FILTER_H
#define FILTER_H

#include <qptrlist.h>
#include <qstring.h>

static const int FILTER_MAX_RULES = 8;

namespace Bookcase {
  namespace Data {
    class Entry;
  }

/**
 * @author Robby Stephenson
 * @version $Id: filter.h 576 2004-03-26 01:21:30Z robby $
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
    FuncRegExp, FuncNotRegExp
  };

  FilterRule(const QString& fieldName, const QString& text, Function func);

  /**
   * A rule is empty if the pattern text is empty
   */
  bool isEmpty() const { return m_pattern.isEmpty(); }
  /**
   * This is the primary function of the rule.
   *
   * @return Returns true if the entry is matched by the rule.
   */
  bool matches(const Data::Entry* const unit) const;

  /**
   * Return filter function. This can be any of the operators
   * defined in @ref Function.
   */
  Function function() const { return m_function; }
  /**
   * Set filter function.
   */
  void setFunction(Function func) { m_function = func; }
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
  const QString& pattern() const { return m_pattern; }
  /**
   * Set pattern
   */
//  void setPattern(const QString& pattern) { m_pattern = pattern; }

private:
  bool equals(const Data::Entry* const unit_) const;
  bool contains(const Data::Entry* const unit_) const;
  bool matchesRegExp(const Data::Entry* const unit_) const;

  QString m_fieldName;
  Function m_function;
  QString m_pattern;
};

/**
 * Borrows from KMSearchPattern by Marc Mutz
 *
 * @author Robby Stephenson
 * @version $Id: filter.h 576 2004-03-26 01:21:30Z robby $
 */
class Filter : public QPtrList<FilterRule> {

public:
  enum FilterOp {
    MatchAny,
    MatchAll
  };

  Filter(FilterOp op_) : QPtrList<FilterRule>(), m_op(op_) { setAutoDelete(true); }

  FilterOp op() const { return m_op; }
  bool matches(const Data::Entry* const unit) const;

private:
  FilterOp m_op;
};

} // end namespace
#endif
