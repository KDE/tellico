/***************************************************************************
                                 bcfilter.h
                             -------------------
    begin                : Wed Apr 9 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BCFILTER_H
#define BCFILTER_H

class BCUnit;

#include <qptrlist.h>
#include <qstring.h>

static const int FILTER_MAX_RULES = 8;

/**
 *
 *@author Robby Stephenson
 */
class BCFilterRule {

public:
  /**
   * Operators for comparison of field and contents.
   * If you change the order or contents of the enum: do not forget
   * to change matches() and @ref BCFilterRuleWidget::initLists(), too.
   */
  enum Function {
    FuncContains=0, FuncNotContains,
    FuncEquals, FuncNotEquals,
    FuncRegExp, FuncNotRegExp
  };
      
  BCFilterRule(const QString& attName, const QString& text, Function func);

  /**
   * A rule is empty if the pattern text is empty
   */
  bool isEmpty() const;
  bool matches(const BCUnit* const unit) const;

  /**
   * Return filter function. This can be any of the operators
   * defined in @ref Function.
   */
  Function function() const;
  /**
   * Set filter function.
   */
  void setFunction(Function func);

  /**
   * Return attribute
   */
  const QString& attribute() const;
  /**
   * Set attribute name
   */
  void setAttribute(const QString& attName);
  
  /**
   * Return pattern
   */
  const QString& pattern() const;
  /**
   * Set pattern
   */
  void setPattern(const QString& pattern);

protected:
  bool equals(const BCUnit* const unit_) const;
  bool contains(const BCUnit* const unit_) const;
  bool matchesRegExp(const BCUnit* const unit_) const;
  
private:
  QString m_attributeName;
  Function m_function;
  QString m_pattern;
};

/**
 * Borrows from KMSearchPattern by Marc Mutz
 *
 * @author Robby Stephenson
 */
class BCFilter:public QPtrList<BCFilterRule> {

public:
  enum FilterOp {
    MatchAny,
    MatchAll
  };
  
  BCFilter(FilterOp op);

  FilterOp op() const;
  bool matches(const BCUnit* const unit) const;

private:
  FilterOp m_op; 
};

#endif
