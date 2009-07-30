/***************************************************************************
    Copyright (C) 2001-2009 Robby Stephenson <robby@periapsis.org>
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

#include "entry.h"
#include "entrygroup.h"
#include "collection.h"
#include "field.h"
#include "tellico_utils.h"
#include "tellico_debug.h"

#include <klocale.h>

#include <QRegExp>

using Tellico::Data::Entry;

Entry::Entry(Tellico::Data::CollPtr coll_) : QSharedData(), m_coll(coll_), m_id(-1) {
#ifndef NDEBUG
  if(!coll_) {
    myWarning() << "null collection pointer!";
  }
#endif
}

Entry::Entry(Tellico::Data::CollPtr coll_, ID id_) : QSharedData(), m_coll(coll_), m_id(id_) {
#ifndef NDEBUG
  if(!coll_) {
    myWarning() << "null collection pointer!";
  }
#endif
}

Entry::Entry(const Entry& entry_) :
    QSharedData(entry_),
    m_coll(entry_.m_coll),
    m_id(-1),
    m_fieldValues(entry_.m_fieldValues),
    m_formattedFields(entry_.m_formattedFields) {
}

Entry& Entry::operator=(const Entry& other_) {
  if(this == &other_) return *this;

//  static_cast<QSharedData&>(*this) = static_cast<const QSharedData&>(other_);
  m_coll = other_.m_coll;
  m_id = other_.m_id;
  m_fieldValues = other_.m_fieldValues;
  m_formattedFields = other_.m_formattedFields;
  return *this;
}

Entry::~Entry() {
}

Tellico::Data::CollPtr Entry::collection() const {
  return m_coll;
}

void Entry::setCollection(Tellico::Data::CollPtr coll_) {
  if(coll_ == m_coll) {
    myDebug() << "already belongs to collection!";
    return;
  }
  // special case adding a book to a bibtex collection
  // it would be better to do this in a real OO way, but this should work
  const bool addEntryType = m_coll->type() == Collection::Book &&
                            coll_->type() == Collection::Bibtex &&
                            !m_coll->hasField(QLatin1String("entry-type"));
  m_coll = coll_;
  m_id = -1;
  // set this after changing the m_coll pointer since setField() checks field validity
  if(addEntryType) {
    setField(QLatin1String("entry-type"), QLatin1String("book"));
  }
}

QString Entry::title() const {
  return formattedField(QLatin1String("title"));
}

QString Entry::field(Tellico::Data::FieldPtr field_, bool formatted_/*=false*/) const {
  return field(field_->name(), formatted_);
}

QString Entry::field(const QString& fieldName_, bool formatted_/*=false*/) const {
  if(formatted_) {
    return formattedField(fieldName_);
  }

  FieldPtr f = m_coll->fieldByName(fieldName_);
  if(!f) {
    return QString();
  }
  if(f->hasFlag(Field::Derived)) {
    return dependentValue(this, f->description(), false);
  }

  if(!m_fieldValues.isEmpty() && m_fieldValues.contains(fieldName_)) {
    return m_fieldValues[fieldName_];
  }
  return QString();
}

QString Entry::formattedField(Tellico::Data::FieldPtr field_) const {
  return formattedField(field_->name());
}

QString Entry::formattedField(const QString& fieldName_) const {
  FieldPtr f = m_coll->fieldByName(fieldName_);
  if(!f) {
    return QString();
  }

  Field::FormatFlag flag = f->formatFlag();
  if(f->hasFlag(Field::Derived)) {
    if(flag == Field::FormatNone) {
      return dependentValue(this, f->description(), false);
    } else {
      // format sub fields and whole string
      return Field::format(dependentValue(this, f->description(), true), flag);
    }
  }

  // if auto format is not set or FormatNone, then just return the value
  if(flag == Field::FormatNone) {
    return m_coll->prepareText(field(fieldName_));
  }

  if(m_formattedFields.isEmpty() || !m_formattedFields.contains(fieldName_)) {
    QString value = field(fieldName_);
    if(!value.isEmpty()) {
      value = Field::format(m_coll->prepareText(value), flag);
      m_formattedFields.insert(fieldName_, value);
    }
    return value;
  }
  // otherwise, just look it up
  return m_formattedFields[fieldName_];
}

QStringList Entry::fields(Tellico::Data::FieldPtr field_, bool formatted_) const {
  return fields(field_->name(), formatted_);
}

QStringList Entry::fields(const QString& field_, bool formatted_) const {
  QString s = formatted_ ? formattedField(field_) : field(field_);
  if(s.isEmpty()) {
    return QStringList();
  }
  return Field::split(s, true);
}

bool Entry::setField(Tellico::Data::FieldPtr field_, const QString& value_) {
  return setField(field_->name(), value_);
}

bool Entry::setField(const QString& name_, const QString& value_) {
  if(name_.isEmpty()) {
    myWarning() << "empty field name for value: " << value_;
    return false;
  }
  // an empty value means remove the field
  if(value_.isEmpty()) {
    if(!m_fieldValues.isEmpty() && m_fieldValues.contains(name_)) {
      m_fieldValues.remove(name_);
    }
    invalidateFormattedFieldValue(name_);
    return true;
  }

#ifndef NDEBUG
  if(m_coll && (m_coll->fields().count() == 0 || !m_coll->hasField(name_))) {
    myDebug() << "unknown collection entry field - " << name_;
    return false;
  }
#endif

  if(m_coll && !m_coll->isAllowed(name_, value_)) {
    myDebug() << "for " << name_ << ", value is not allowed - " << value_;
    return false;
  }

  Data::FieldPtr f = m_coll->fieldByName(name_);
  if(!f) {
    return false;
  }

  // the string store is probable only useful for fields with auto-completion or choice/number/bool
  bool shareType = f->type() == Field::Choice ||
                   f->type() == Field::Bool ||
                   f->type() == Field::Image ||
                   f->type() == Field::Rating ||
                   f->type() == Field::Number;
  if(!(f->hasFlag(Field::AllowMultiple)) &&
     (shareType ||
      (f->type() == Field::Line && (f->flags() & Field::AllowCompletion)))) {
    m_fieldValues.insert(Tellico::shareString(name_), Tellico::shareString(value_));
  } else {
    m_fieldValues.insert(Tellico::shareString(name_), value_);
  }
  invalidateFormattedFieldValue(name_);
  return true;
}

bool Entry::addToGroup(EntryGroup* group_) {
  if(!group_ || m_groups.contains(group_)) {
    return false;
  }

  m_groups.push_back(group_);
  group_->append(EntryPtr(this));
//  m_coll->groupModified(group_);
  return true;
}

bool Entry::removeFromGroup(EntryGroup* group_) {
  // if the removal isn't successful, just return
  bool success = m_groups.removeAll(group_);
  success = success && group_->removeAll(EntryPtr(this));
//  myDebug() << "removing from group - " << group_->fieldName() << "::" << group_->groupName();
  if(success) {
//    m_coll->groupModified(group_);
  } else {
    myDebug() << "failed!";
  }
  return success;
}

void Entry::clearGroups() {
  m_groups.clear();
}

// this function gets called before m_groups is updated. In fact, it is used to
// update that list. This is the function that actually parses the field values
// and returns the list of the group names.
QStringList Entry::groupNamesByFieldName(const QString& fieldName_) const {
//  myDebug() << fieldName_;
  FieldPtr f = m_coll->fieldByName(fieldName_);

  // easy if not allowing multiple values
  if(!(f->hasFlag(Field::AllowMultiple))) {
    return QStringList(formattedField(fieldName_));
  }

  QStringList groups = fields(fieldName_, true);
  if(groups.isEmpty()) {
    return QStringList(QString());
  } else if(f->type() == Field::Table) {
    // quick hack for tables, how often will a user have "::" in their value?
    // only use first column for group
    QStringList newGroupNames;
    foreach(const QString& group, groups) {
      QString newGroupName = group.section(QLatin1String("::"),  0,  0);
      if(!newGroupName.isEmpty()) {
        newGroupNames.append(newGroupName);
      }
    }
    groups = newGroupNames;
  }
  return groups;
}

bool Entry::isOwned() {
  return (m_coll && m_id > -1 && m_coll->entryCount() > 0 && m_coll->entries().contains(EntryPtr(this)));
}

// an empty string means invalidate all
void Entry::invalidateFormattedFieldValue(const QString& name_) {
  if(name_.isEmpty()) {
    m_formattedFields.clear();
  } else if(!m_formattedFields.isEmpty() && m_formattedFields.contains(name_)) {
    m_formattedFields.remove(name_);
  }
}

// format is something like "%{year} %{author}"
QString Entry::dependentValue(const Entry* entry_, const QString& format_, bool formatted_) {
  if(!entry_) {
    return format_;
  }

  QString result, fieldName;
  FieldPtr field;

  int endPos;
  int curPos = 0;
  int pctPos = format_.indexOf(QLatin1Char('%'), curPos);
  while(pctPos != -1 && pctPos+1 < static_cast<int>(format_.length())) {
    if(format_[pctPos+1] == QLatin1Char('{')) {
      endPos = format_.indexOf(QLatin1Char('}'), pctPos+2);
      if(endPos > -1) {
        result += format_.mid(curPos, pctPos-curPos);
        fieldName = format_.mid(pctPos+2, endPos-pctPos-2);
        field = entry_->collection()->fieldByName(fieldName);
        if(!field) {
          // allow the user to also use field titles
          field = entry_->collection()->fieldByTitle(fieldName);
        }
        if(field) {
          // don't format, just capitalize
          result += entry_->field(field, formatted_);
        } else if(fieldName == QLatin1String("_id")) {
          result += QString::number(entry_->id());
        } else {
          result += format_.mid(pctPos, endPos-pctPos+1);
        }
        curPos = endPos+1;
      } else {
        break;
      }
    } else {
      result += format_.mid(curPos, pctPos-curPos+1);
      curPos = pctPos+1;
    }
    pctPos = format_.indexOf(QLatin1Char('%'), curPos);
  }
  result += format_.mid(curPos, format_.length()-curPos);
//  myDebug() << "format_ << " = " << result;
  // sometimes field value might empty, resulting in multiple consecutive white spaces
  // so let's simplify that...
  return result.simplified();
}
