/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_FILTER_H
#define TELLICO_FILTER_H

#include "datavectors.h"

#include <ksharedptr.h>

#include <QList>
#include <QString>

namespace Tellico {
  namespace Data {
    class Entry;
  }

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
    FuncRegExp, FuncNotRegExp
  };

  FilterRule();
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
  bool matches(Data::EntryPtr entry) const;

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
  bool equals(Data::EntryPtr entry) const;
  bool contains(Data::EntryPtr entry) const;
  bool matchesRegExp(Data::EntryPtr entry) const;

  QString m_fieldName;
  Function m_function;
  QString m_pattern;
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
  bool matches(Data::EntryPtr entry) const;

  void setName(const QString& name) { m_name = name; }
  const QString& name() const { return m_name; }

  int count() const { return QList<FilterRule*>::count(); } // disambiguate

private:
  Filter& operator=(const Filter& other);

  FilterOp m_op;
  QString m_name;
};

} // end namespace
#endif
