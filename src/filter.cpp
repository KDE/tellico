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

#include "filter.h"
#include "entry.h"

#include <kdebug.h>

#include <qregexp.h>

using Bookcase::Filter;
using Bookcase::FilterRule;

FilterRule::FilterRule(const QString& fieldName_, const QString& pattern_, Function func_)
 : m_fieldName(fieldName_), m_function(func_), m_pattern(pattern_) {
}

bool FilterRule::matches(const Data::Entry* const unit_) const {
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
      kdWarning() << "FilterRule::matches() - invalid function!" << endl;
      break;
  }
  return true;
}

bool FilterRule::equals(const Data::Entry* const unit_) const {
  bool match = false;

  // empty field name means search all
  if(m_fieldName.isEmpty()) {
    QStringList list = unit_->fieldValues();
    for(QStringList::ConstIterator it = list.begin(); it != list.end(); ++it) {
      if(QString::compare((*it).lower(), m_pattern.lower()) == 0) {
        match = true;
        break;
      }
    }
  } else {
    // TODO: for Bool types, should match anything?
    QString value = unit_->field(m_fieldName);
    match = (QString::compare(value.lower(), m_pattern.lower()) == 0);
  }

  return match;
}

bool FilterRule::contains(const Data::Entry* const unit_) const {
  bool match = false;

  // empty field name means search all
  if(m_fieldName.isEmpty()) {
    QStringList list = unit_->fieldValues();
    // match is true if any strings match
    for(QStringList::ConstIterator it = list.begin(); it != list.end(); ++it) {
      if((*it).find(m_pattern, 0, false) >= 0) {
        match = true;
        break;
      }
    }
  } else {
    QString value = unit_->field(m_fieldName);
    match = (value.find(m_pattern, 0, false) >= 0);
  }

  return match;
}

bool FilterRule::matchesRegExp(const Data::Entry* const unit_) const {
  bool match = false;

  QRegExp rx(m_pattern, false);
  // empty field name means search all
  if(m_fieldName.isEmpty()) {
    QStringList list = unit_->fieldValues();
    for(QStringList::ConstIterator it = list.begin(); it != list.end(); ++it) {
      if((*it).find(rx) >= 0) {
        match = true;
        break;
      }
    }
  } else {
    QString value = unit_->field(m_fieldName);
    match = (value.find(rx) >= 0);
  }

  return match;
}


/*******************************************************/

bool Filter::matches(const Data::Entry* const unit_) const {
  if(isEmpty()) {
    return true;
  }

  bool match = false;
  for(QPtrListIterator<FilterRule> it(*this); it.current(); ++it) {
    if(it.current()->matches(unit_)) {
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
