/***************************************************************************
                                bcfilter.cpp
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

#include "bcfilter.h"
#include "bcunit.h"

#include <kdebug.h>

#include <qregexp.h>

BCFilterRule::BCFilterRule(const QString& attName_, const QString& pattern_, Function func_)
 : m_attributeName(attName_), m_function(func_), m_pattern(pattern_) {
}

bool BCFilterRule::matches(const BCUnit* const unit_) const {
  switch (m_function) {
    case FuncEquals:
      return equals(unit_);
    case FuncNotEquals:
      return !equals(unit_);
    case FuncContains:
      return contains(unit_);
    case FuncNotContains:
      return !contains(unit_);
    case FuncRegExp:
      return matchesRegExp(unit_);
    case FuncNotRegExp:
      return !matchesRegExp(unit_);
    default:
      kdWarning() << "BCFilterRule::matches() - invalid function!" << endl;
      break;
  }
  return true;
}

bool BCFilterRule::isEmpty() const {
  return m_pattern.isEmpty();
}

BCFilterRule::Function BCFilterRule::function() const {
  return m_function;
}

void BCFilterRule::setFunction(BCFilterRule::Function func_) {
  m_function = func_;
}

const QString& BCFilterRule::attribute() const {
  return m_attributeName;
}

void BCFilterRule::setAttribute(const QString& attName_) {
  m_attributeName = attName_;
}

const QString& BCFilterRule::pattern() const {
  return m_pattern;
}

void BCFilterRule::setPattern(const QString& pattern_) {
  m_pattern = pattern_;
}

bool BCFilterRule::equals(const BCUnit* const unit_) const {
  bool match = false;

  // empty attribute name means search all
  if(m_attributeName.isEmpty()) {
    QStringList list = unit_->attributeValues();
    for(QStringList::ConstIterator it = list.begin(); it != list.end(); ++it) {
      if(QString::compare((*it).lower(), m_pattern.lower()) == 0) {
        match = true;
        break;
      }
    }
  } else {
    // TODO: for Bool types, should match anything?
    QString value = unit_->attribute(m_attributeName);
    match = (QString::compare(value.lower(), m_pattern.lower()) == 0);
  }

  return match;
}

bool BCFilterRule::contains(const BCUnit* const unit_) const {
  bool match = false;

  // empty attribute name means search all
  if(m_attributeName.isEmpty()) {
    QStringList list = unit_->attributeValues();
    // match is true if any strings match
    for(QStringList::ConstIterator it = list.begin(); it != list.end(); ++it) {
      if((*it).find(m_pattern, 0, false) >= 0) {
        match = true;
        break;
      }
    }
  } else {
    QString value = unit_->attribute(m_attributeName);
    match = (value.find(m_pattern, 0, false) >= 0);
  }

  return match;
}

bool BCFilterRule::matchesRegExp(const BCUnit* const unit_) const {
  bool match = false;

  QRegExp rx(m_pattern, false);
  // empty attribute name means search all
  if(m_attributeName.isEmpty()) {
    QStringList list = unit_->attributeValues();
    for(QStringList::ConstIterator it = list.begin(); it != list.end(); ++it) {
      if((*it).find(rx) >= 0) {
        match = true;
        break;
      }
    }
  } else {
    QString value = unit_->attribute(m_attributeName);
    match = (value.find(rx) >= 0);
  }

  return match;
}


/*******************************************************/

BCFilter::BCFilter(FilterOp op_) : QPtrList<BCFilterRule>(), m_op(op_) {
  setAutoDelete(true);
}

BCFilter::FilterOp BCFilter::op() const {
  return m_op;
}

bool BCFilter::matches(const BCUnit* const unit_) const {
  if(isEmpty()) {
    return true;
  }

  bool match = false;
  QPtrListIterator<BCFilterRule> it(*this);
  for( ; it.current(); ++it) {
    if(it.current()->matches(unit_)) {
      if(m_op == BCFilter::MatchAny) {
        return true;
      } else {
        match = true;
      }
    } else {
      if(m_op == BCFilter::MatchAll) {
        return false;
      }
    }
  }
  
  return match;
}
