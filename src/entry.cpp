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
#include "derivedvalue.h"
#include "utils/string_utils.h"
#include "utils/stringset.h"
#include "tellico_debug.h"

#include <KLocalizedString>

using namespace Tellico;
using namespace Tellico::Data;
using Tellico::Data::Entry;

Entry::Entry(Tellico::Data::CollPtr coll_) : QSharedData(), m_coll(coll_), m_id(-1) {
#ifndef NDEBUG
  if(!coll_) {
    myWarning() << "null collection pointer!";
  }
#endif
}

Entry::Entry(Tellico::Data::CollPtr coll_, Data::ID id_) : QSharedData(), m_coll(coll_), m_id(id_) {
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
  // special case for creation date since it gets set in Collection::addEntry IF cdate is empty
  m_fieldValues.remove(QStringLiteral("cdate"));
  m_fieldValues.remove(QStringLiteral("mdate"));
}

Entry& Entry::operator=(const Entry& other_) {
  if(this == &other_) return *this;

//  static_cast<QSharedData&>(*this) = static_cast<const QSharedData&>(other_);
  m_coll = other_.m_coll;
  m_id = other_.m_id;
  m_fieldValues = other_.m_fieldValues;
  // special case for creation date field which gets set in Collection::addEntry IF cdate is empty
  m_fieldValues.remove(QStringLiteral("cdate"));
  m_fieldValues.remove(QStringLiteral("mdate"));
  m_formattedFields = other_.m_formattedFields;
  return *this;
}

Entry::~Entry() = default;

Tellico::Data::CollPtr Entry::collection() const {
  return m_coll;
}

void Entry::setCollection(Tellico::Data::CollPtr coll_) {
  if(coll_ == m_coll) {
//    myDebug() << "already belongs to collection!";
    return;
  }
  // special case adding a book to a bibtex collection
  // it would be better to do this in a real object-oriented way, but this should work
  const bool addEntryType = m_coll->type() == Collection::Book &&
                            coll_->type() == Collection::Bibtex &&
                            !m_coll->hasField(QStringLiteral("entry-type"));
  m_coll = coll_;
  m_id = -1;
  // set this after changing the m_coll pointer since setField() checks field validity
  if(addEntryType) {
    setField(QStringLiteral("entry-type"), QStringLiteral("book"));
  }
}

QString Entry::title(FieldFormat::Request request_) const {
  return formattedField(m_coll ? m_coll->titleField() : QStringLiteral("title"), request_);
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

  return m_fieldValues.value(field_->name());
}

QString Entry::formattedField(const QString& fieldName_, FieldFormat::Request request_) const {
  return formattedField(m_coll->fieldByName(fieldName_), request_);
}

QString Entry::formattedField(Tellico::Data::FieldPtr field_, FieldFormat::Request request_) const {
  if(!field_) {
    return QString();
  }

  // don't format the value unless it's requested to do so
  if(request_ == FieldFormat::AsIsFormat) {
    return field(field_);
  }

  const FieldFormat::Type flag = field_->formatType();
  if(field_->hasFlag(Field::Derived)) {
    DerivedValue dv(field_);
    // format sub fields and whole string
    return FieldFormat::format(dv.value(EntryPtr(const_cast<Entry*>(this)), true), flag, request_);
  }

  // if auto format is not set or FormatNone, then just return the value
  if(flag == FieldFormat::FormatNone) {
    return m_coll->prepareText(field(field_));
  }

  if(!m_formattedFields.contains(field_->name())) {
    QString formattedValue;
    if(field_->type() == Field::Table) {
      QStringList rows;
      // we only format the first column
      foreach(const QString& row, FieldFormat::splitTable(field(field_))) {
        QStringList columns = FieldFormat::splitRow(row);
        QStringList newValues;
        if(!columns.isEmpty()) {
          foreach(const QString& value, FieldFormat::splitValue(columns.at(0))) {
            newValues << FieldFormat::format(value, field_->formatType(), FieldFormat::DefaultFormat);
          }
          columns.replace(0, newValues.join(FieldFormat::delimiterString()));
        }
        rows << columns.join(FieldFormat::columnDelimiterString());
      }
      formattedValue = rows.join(FieldFormat::rowDelimiterString());
    } else {
      QStringList values;
      if(field_->hasFlag(Field::AllowMultiple)) {
        values = FieldFormat::splitValue(field(field_));
      } else {
        values << field(field_);
      }
      QStringList formattedValues;
      foreach(const QString& value, values) {
        formattedValues << FieldFormat::format(m_coll->prepareText(value), flag, request_);
      }
      formattedValue = formattedValues.join(FieldFormat::delimiterString());
    }
    if(!formattedValue.isEmpty()) {
      m_formattedFields.insert(field_->name(), Tellico::shareString(formattedValue));
    }
    return formattedValue;
  }
  // otherwise, just look it up
  return m_formattedFields.value(field_->name());
}

// updating the modified date of the entry is expensive with the call to QDate::currentDate
// when loading a collection from a file (in particular), it's faster to ignore that date
bool Entry::setField(Tellico::Data::FieldPtr field_, const QString& value_, bool updateMDate_) {
  if(!field_) return false;

  const auto name = field_->name();
  const auto oldValue = field(name);
  const bool wasDifferent = oldValue != value_;
  // returning true means entry was successfully modified
  const bool res = setFieldImpl(field_, value_);

  static const QString mdate(QStringLiteral("mdate"));
  if(res && wasDifferent && updateMDate_ && name != mdate && m_coll->hasField(mdate)) {
    setFieldImpl(m_coll->fieldByName(mdate), QDate::currentDate().toString(Qt::ISODate));
  }
  if(field_->type() == Field::Image && !oldValue.isEmpty() && wasDifferent) {
    // need to track changes in image values to potentially remove images
    // no longer in the collection when the collection is saved
    m_coll->imageChanged(oldValue, value_);
  }
  return res;
}

bool Entry::setField(const QString& name_, const QString& value_, bool updateMDate_) {
  if(name_.isEmpty()) {
    myWarning() << "empty field name for value:" << value_;
    return false;
  }

  auto field = m_coll->fieldByName(name_);
  if(!field) {
    myDebug() << "unknown collection field -" << name_
              << "in collection" << m_coll->title() << "- not adding" << value_;
    return false;
  }

  return setField(field, value_, updateMDate_);
}

bool Entry::setFieldImpl(Data::FieldPtr field_, const QString& value_) {
  if(!field_) return false;
  const auto name = field_->name();
  // an empty value means remove the field
  if(value_.isEmpty()) {
    if(m_fieldValues.remove(name)) {
      invalidateFormattedFieldValue(name);
    }
    return true;
  }

  if(m_coll && !m_coll->isAllowed(name, value_)) {
    myDebug() << "for" << name << ", value is not allowed -" << value_;
    return false;
  }

  // the string store is probably only useful for fields with auto-completion or choice/number/bool
  const bool shareType = field_->type() == Field::Choice ||
                         field_->type() == Field::Bool ||
                         field_->type() == Field::Image ||
                         field_->type() == Field::Rating ||
                         field_->type() == Field::Number ||
                         (field_->type() == Field::Line &&
                          field_->hasFlag(Field::AllowCompletion);
  if(!field_->hasFlag(Field::AllowMultiple) && shareType) {
    m_fieldValues.insert(Tellico::shareString(name), Tellico::shareString(value_));
  } else {
    m_fieldValues.insert(Tellico::shareString(name), value_);
  }
  invalidateFormattedFieldValue(name);
  return true;
}

bool Entry::addToGroup(EntryGroup* group_) {
  if(!group_ || m_groups.contains(group_)) {
    return false;
  }

  m_groups.push_back(group_);
  group_->append(EntryPtr(this));
  return true;
}

bool Entry::removeFromGroup(EntryGroup* group_) {
  // if the removal isn't successful, just return
  bool success = m_groups.removeOne(group_);
  success = group_->removeOne(EntryPtr(this)) && success;
//  myDebug() << "removing from group - " << group_->fieldName() << "--" << group_->groupName();
  if(!success) {
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

  StringSet groups;
  // check table before multiple since tables are always multiple
  if(f->type() == Field::Table) {
    // we only take groups from the first column
    foreach(const QString& row, FieldFormat::splitTable(field(f))) {
      const QStringList columns = FieldFormat::splitRow(row);
      const QStringList values = columns.isEmpty() ? QStringList() : FieldFormat::splitValue(columns.at(0));
      foreach(const QString& value, values) {
        groups.add(FieldFormat::format(value, f->formatType(), FieldFormat::DefaultFormat));
      }
    }
  } else if(f->hasFlag(Field::AllowMultiple)) {
    // use a string split instead of regexp split, since we've already enforced the space after the semi-comma
    groups.add(FieldFormat::splitValue(formattedField(f), FieldFormat::StringSplit));
  } else {
    groups.add(formattedField(f));
  }

  // possible to be empty for no value
  // but we want to populate an empty group
  return groups.isEmpty() ? QStringList(QString()) : groups.values();
}

bool Entry::isOwned() {
  return (m_coll && m_id > -1 && m_coll->entryCount() > 0 && m_coll->entries().contains(EntryPtr(this)));
}

// an empty string means invalidate all
void Entry::invalidateFormattedFieldValue(const QString& name_) {
  if(name_.isEmpty()) {
    m_formattedFields.clear();
  } else if(!m_formattedFields.isEmpty()) {
    m_formattedFields.remove(name_);
  }
}
