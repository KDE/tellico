/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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

#include "tellico_debug.h"

#include <qregexp.h>

using Tellico::Filter;
using Tellico::FilterRule;

FilterRule::FilterRule() : m_function(FuncEquals) {
}

FilterRule::FilterRule(const QString& fieldName_, const QString& pattern_, Function func_)
 : m_fieldName(fieldName_), m_function(func_), m_pattern(pattern_) {
}

bool FilterRule::matches(Data::EntryPtr entry_) const {
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
    default:
      kdWarning() << "FilterRule::matches() - invalid function!" << endl;
      break;
  }
  return true;
}

bool FilterRule::equals(Data::EntryPtr entry_) const {
  // empty field name means search all
  if(m_fieldName.isEmpty()) {
    QStringList list = entry_->fieldValues() + entry_->formattedFieldValues();
    for(QStringList::ConstIterator it = list.begin(); it != list.end(); ++it) {
      if(QString::compare((*it).lower(), m_pattern.lower()) == 0) {
        return true;
      }
    }
  } else {
    return QString::compare(entry_->field(m_fieldName).lower(), m_pattern.lower()) == 0
           || QString::compare(entry_->formattedField(m_fieldName).lower(), m_pattern.lower()) == 0;
  }

  return false;
}

bool FilterRule::contains(Data::EntryPtr entry_) const {
  // empty field name means search all
  if(m_fieldName.isEmpty()) {
    QStringList list = entry_->fieldValues() + entry_->formattedFieldValues();
    // match is true if any strings match
    for(QStringList::ConstIterator it = list.begin(); it != list.end(); ++it) {
      if((*it).find(m_pattern, 0, false) >= 0) {
        return true;
      }
    }
  } else {
    return entry_->field(m_fieldName).find(m_pattern, 0, false) >= 0
           || entry_->formattedField(m_fieldName).find(m_pattern, 0, false) >= 0;
  }

  return false;
}

bool FilterRule::matchesRegExp(Data::EntryPtr entry_) const {
  QRegExp rx(m_pattern, false);
  // empty field name means search all
  if(m_fieldName.isEmpty()) {
    QStringList list = entry_->fieldValues() + entry_->formattedFieldValues();
    for(QStringList::ConstIterator it = list.begin(); it != list.end(); ++it) {
      if((*it).find(rx) >= 0) {
        return true;
        break;
      }
    }
  } else {
    return entry_->field(m_fieldName).find(rx) >= 0
           || entry_->formattedField(m_fieldName).find(rx) >= 0;
  }

  return false;
}


/*******************************************************/

Filter::Filter(const Filter& other_) : QPtrList<FilterRule>(), KShared(), m_op(other_.op()), m_name(other_.name()) {
  for(QPtrListIterator<FilterRule> it(other_); it.current(); ++it) {
    append(new FilterRule(*it.current()));
  }
  setAutoDelete(true);
}

bool Filter::matches(Data::EntryPtr entry_) const {
  if(isEmpty()) {
    return true;
  }

  bool match = false;
  for(QPtrListIterator<FilterRule> it(*this); it.current(); ++it) {
    if(it.current()->matches(entry_)) {
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
