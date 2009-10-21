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
#include "fieldformat.h"
#include "derivedvalue.h"
#include "tellico_utils.h"
#include "utils/stringset.h"
#include "core/tellico_config.h"
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
  return field(QLatin1String("title"));
}

QString Entry::field(const QString& fieldName_) const {
  return field(m_coll->fieldByName(fieldName_));
}

QString Entry::field(Tellico::Data::FieldPtr field_) const {
  if(!field_) {
    return QString();
  }

  if(field_->hasFlag(Field::Derived)) {
    DerivedValue dv(field_);
    return dv.value(EntryPtr(const_cast<Entry*>(this)), false);
  }

  if(m_fieldValues.contains(field_->name())) {
    return m_fieldValues.value(field_->name());
  }
  return QString();
}

QString Entry::formattedField(const QString& fieldName_, FormatValue formatted_) const {
  return formattedField(m_coll->fieldByName(fieldName_), formatted_);
}

QString Entry::formattedField(Tellico::Data::FieldPtr field_, FormatValue formatted_) const {
  if(!field_) {
    return QString();
  }

  // if neither the capitalization or formatting option is turned on, don't format the value
  if(formatted_ == NoFormat ||
     (formatted_ == AutoFormat && !Config::autoCapitalization() && !Config::autoFormat())) {
    return field(field_);
  }

  const Field::FormatFlag flag = field_->formatFlag();
  if(field_->hasFlag(Field::Derived)) {
    DerivedValue dv(field_);
    // format sub fields and whole string
    return Field::format(dv.value(EntryPtr(const_cast<Entry*>(this)), true), flag);
  }

  // if auto format is not set or FormatNone, then just return the value
  if(flag == Field::FormatNone) {
    return m_coll->prepareText(field(field_));
  }

  if(!m_formattedFields.contains(field_->name())) {
    QString value = field(field_);
    if(!value.isEmpty()) {
      value = Field::format(m_coll->prepareText(value), flag);
      m_formattedFields.insert(field_->name(), value);
    }
    return value;
  }
  // otherwise, just look it up
  return m_formattedFields.value(field_->name());
}

bool Entry::setField(Tellico::Data::FieldPtr field_, const QString& value_) {
  return setField(field_->name(), value_);
}

bool Entry::setField(const QString& name_, const QString& value_) {
  if(name_.isEmpty()) {
    myWarning() << "empty field name for value: " << value_;
    return false;
  }

  if(m_coll->fields().isEmpty() || !m_coll->hasField(name_)) {
    myDebug() << "unknown collection entry field -" << name_;
    return false;
  }

  const bool res = setFieldImpl(name_, value_);
  // returning true means entry was successfully modified
  if(res && name_ != QLatin1String("mdate") && m_coll->hasField(QLatin1String("mdate"))) {
    setFieldImpl(QLatin1String("mdate"), QDate::currentDate().toString(Qt::ISODate));
  }
  return res;
}

bool Entry::setFieldImpl(const QString& name_, const QString& value_) {
  // an empty value means remove the field
  if(value_.isEmpty()) {
    if(m_fieldValues.contains(name_)) {
      m_fieldValues.remove(name_);
    }
    invalidateFormattedFieldValue(name_);
    return true;
  }

  if(m_coll && !m_coll->isAllowed(name_, value_)) {
    myDebug() << "for" << name_ << ", value is not allowed -" << value_;
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
//  myDebug() << "removing from group - " << group_->fieldName() << "--" << group_->groupName();
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
  if(!f) {
    myWarning() << "no field named" << fieldName_;
    return QStringList();
  }

  if(f->type() == Field::Table) {
    // we only take groups from the first column
    StringSet groups;
    foreach(const QString& row, FieldFormat::splitTable(field(f))) {
      const QStringList columns = FieldFormat::splitRow(row);
      groups.add(FieldFormat::splitValue(columns.at(0)));
    }
    return groups.toList();
  }

  if(f->hasFlag(Field::AllowMultiple)) {
    QStringList groups = FieldFormat::splitValue(field(f));
    // possible to be empty for no value
    // but we want to populate an empty group
    if(groups.isEmpty()) {
      groups << QString();
    }
    return groups;
  }

  // easy if not allowing multiple values
  return QStringList() << field(fieldName_);
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
