/***************************************************************************
    copyright            : (C) 2001-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "entry.h"
#include "collection.h"
#include "field.h"
#include "translators/bibtexhandler.h" // needed for BibtexHandler::cleanText()
#include "document.h"
#include "tellico_debug.h"

#include <kdebug.h>

#include <qregexp.h>

using Tellico::Data::Entry;

Entry::Entry(Collection* coll_) : KShared(), m_coll(coll_), m_id(-1) {
#ifndef NDEBUG
  if(!coll_) {
    kdWarning() << "Entry() - null collection pointer!" << endl;
  }
#endif
}

Entry::Entry(Collection* coll_, int id_) : KShared(), m_coll(coll_), m_id(id_) {
#ifndef NDEBUG
  if(!coll_) {
    kdWarning() << "Entry() - null collection pointer!" << endl;
  }
#endif
}

Entry::Entry(const Entry& entry_) :
    KShared(entry_),
    m_coll(entry_.m_coll),
    m_id(-1),
    m_fields(entry_.m_fields),
    m_formattedFields(entry_.m_formattedFields) {
}

Entry::Entry(const Entry& entry_, Collection* coll_) :
    KShared(entry_),
    m_coll(coll_),
    m_id(-1),
    m_fields(entry_.m_fields) {
  // merge fields from old collection
  Data::FieldVec fields = entry_.collection()->fields();
  for(FieldVec::Iterator it = fields.begin(); it != fields.end(); ++it) {
    // only add field if there's a value in the collection
    if(!entry_.field(it->name()).isEmpty()) {
      coll_->mergeField(it);
    }
  }
  // special case for adding a book entry to a bibliography
  if(m_coll->type() == Collection::Bibtex && entry_.collection()->type() == Collection::Book
     && field(QString::fromLatin1("entry-type")).isEmpty()) {
    setField(QString::fromLatin1("entry-type"), QString::fromLatin1("book"));
  }
}

Entry& Entry::operator=(const Entry& other_) {
  if(this == &other_) return *this;

//  kdDebug() << "Entry::operator=()" << endl;
  static_cast<KShared&>(*this) = static_cast<const KShared&>(other_);
  m_coll = other_.m_coll;
  m_id = other_.m_id;
  m_fields = other_.m_fields;
  m_formattedFields = other_.m_formattedFields;
  return *this;
}

Entry::~Entry() {
}

QString Entry::title() const {
  return formattedField(QString::fromLatin1("title"));
}

QString Entry::field(const QString& fieldName_, bool formatted_/*=false*/) const {
  if(formatted_) {
    return formattedField(fieldName_);
  }

  Field* f = m_coll->fieldByName(fieldName_);
  if(!f) {
    return QString::null;
  }
  if(f->type() == Field::Dependent) {
    return dependentValue(f->description(), false);
  }

  if(!m_fields.isEmpty() && m_fields.contains(fieldName_)) {
    return m_fields[fieldName_];
  }
  return QString::null;
}

QString Entry::formattedField(const QString& fieldName_) const {
  Field* f = m_coll->fieldByName(fieldName_);
  if(!f) {
    return QString::null;
  }
  if(f->type() == Field::Dependent) {
    return dependentValue(f->description(), true);
  }

  Field::FormatFlag flag = f->formatFlag();

  // if auto format is not set or FormatNone, then just return the value
  if(flag == Field::FormatNone) {
    return field(fieldName_);
  }

  if(m_formattedFields.isEmpty() || !m_formattedFields.contains(fieldName_)) {
    QString value = field(fieldName_);
    if(!value.isEmpty()) {
      // special for Bibtex collections
      if(m_coll->type() == Collection::Bibtex) {
        BibtexHandler::cleanText(value);
      }
      value = Field::format(value, flag);
      m_formattedFields.insert(fieldName_, value);
    }
    return value;
  }
  // otherwise, just look it up
  return m_formattedFields[fieldName_];
}

QStringList Entry::fields(const QString& field_, bool formatted_) const {
  return QStringList::split(QString::fromLatin1("; "), formatted_ ? formattedField(field_) : field(field_));
}

bool Entry::setField(const QString& name_, const QString& value_) {
  // an empty value means remove the field
  if(value_.isEmpty()) {
    if(!m_fields.isEmpty() && m_fields.contains(name_)) {
      m_fields.remove(name_);
    }
    invalidateFormattedFieldValue(name_);
    return true;
  }

#ifndef NDEBUG
  if(m_coll && (m_coll->fields().count() == 0 || m_coll->fieldByName(name_) == 0)) {
    kdDebug() << "Entry::setField() - unknown collection entry field - "
              << name_ << endl;
    return false;
  }
#endif

  if(m_coll && !m_coll->isAllowed(name_, value_)) {
    kdDebug() << "Entry::setField() - for " << name_
              << ", value is not allowed - " << value_ << endl;
    return false;
  }

  m_fields.insert(name_, value_);
  invalidateFormattedFieldValue(name_);
  return true;
}

bool Entry::addToGroup(EntryGroup* group_) {
  if(!group_ || m_groups.contains(group_)) {
    return false;
  }

//  kdDebug() << "Entry::addToGroup() - adding group (" << group_->groupName() << ")" << endl;
  m_groups.push_back(group_);
  group_->append(this);
//  m_coll->groupModified(group_);
  return true;
}

bool Entry::removeFromGroup(EntryGroup* group_) {
  // if the removal isn't successful, just return
  bool success = m_groups.remove(group_);
  success = success && group_->remove(this);
//  kdDebug() << "Entry::removeFromGroup() - removing from group - "
//              << group_->fieldName() << "::" << group_->groupName() << endl;
  if(success) {
//    m_coll->groupModified(group_);
  } else {
    kdDebug() << "Entry::removeFromGroup() failed! " << endl;
  }
  return success;
}

// this function gets called before m_groups is updated. In fact, it is used to
// update that list. This is the function that actually parses the field values
// and returns the list of the group names.
QStringList Entry::groupNamesByFieldName(const QString& fieldName_) const {
//  kdDebug() << "Entry::groupsByfieldName() - " << fieldName_ << endl;
  Field* f = m_coll->fieldByName(fieldName_);

  // easy if not allowing multiple values
  if(!(f->flags() & Field::AllowMultiple)) {
    QString value = formattedField(fieldName_);
    if(value.isEmpty()) {
      return Collection::s_emptyGroupTitle;
    } else {
      return value;
    }
  }

  QStringList groups = fields(fieldName_, true);
  if(groups.isEmpty()) {
    groups += Collection::s_emptyGroupTitle;
  } else if(f->type() == Field::Table) {
    for(QStringList::Iterator it = groups.begin(); it != groups.end(); ++it) {
      // quick hack for tables, how often will a user have "::" in their value?
      // only use first column for group
      (*it) = (*it).section(QString::fromLatin1("::"),  0,  0);
    }
  }
  return groups;
}

bool Entry::isOwned() {
  return (m_coll && m_coll->entryCount() > 0 && m_coll->entries().contains(this));
}

// a null string means invalidate all
void Entry::invalidateFormattedFieldValue(const QString& name_) {
  if(name_.isNull()) {
    m_formattedFields.clear();
  } else if(!m_formattedFields.isEmpty() && m_formattedFields.contains(name_)) {
    m_formattedFields.remove(name_);
  }
}

// format is something like "%{year} %{author}"
QString Entry::dependentValue(const QString& format_, bool autoCapitalize_) const {
  QString result, fieldName;
  Field* f;

  int endPos;
  int curPos = 0;
  int pctPos = format_.find('%', curPos);
  while(pctPos != -1 && pctPos+1 < static_cast<int>(format_.length())) {
    if(format_[pctPos+1] == '{') {
      endPos = format_.find('}', pctPos+2);
      if(endPos > -1) {
        result += format_.mid(curPos, pctPos-curPos);
        fieldName = format_.mid(pctPos+2, endPos-pctPos-2);
        f = m_coll->fieldByName(fieldName);
        if(!f) {
          // allow the user to also use field titles
          f = m_coll->fieldByTitle(fieldName);
        }
        if(f) {
          // don't format, just capitalize
          result += (autoCapitalize_ ? Field::capitalize(field(fieldName)) : field(fieldName));
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
    pctPos = format_.find('%', curPos);
  }
  result += format_.mid(curPos, format_.length()-curPos);
//  kdDebug() << "Entry::dependentValue() - " << format_ << " = " << result << endl;
  return result;
}
