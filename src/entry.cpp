/***************************************************************************
    copyright            : (C) 2001-2006 by Robby Stephenson
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
#include "tellico_utils.h"
#include "tellico_debug.h"
#include "latin1literal.h"

#include <klocale.h>

#include <qregexp.h>

using Tellico::Data::Entry;
using Tellico::Data::EntryGroup;

EntryGroup::EntryGroup(const QString& group, const QString& field)
   : QObject(), EntryVec(), m_group(Tellico::shareString(group)), m_field(Tellico::shareString(field)) {
}

EntryGroup::~EntryGroup() {
  // need a copy since we remove ourselves
  EntryVec vec = *this;
  for(Data::EntryVecIt entry = vec.begin(); entry != vec.end(); ++entry) {
    entry->removeFromGroup(this);
  }
}

bool Entry::operator==(const Entry& e1) {
// special case for file catalog, just check the url
  if(m_coll && m_coll->type() == Collection::File &&
     e1.m_coll && e1.m_coll->type() == Collection::File) {
    // don't forget case where both could have empty urls
    // but different values for other fields
    QString u = field(QString::fromLatin1("url"));
    if(!u.isEmpty()) {
      // versions before 1.2.7 could have saved the url without the protocol
      bool b = KURL::fromPathOrURL(u) == KURL::fromPathOrURL(e1.field(QString::fromLatin1("url")));
      if(b) {
        return true;
      } else {
        Data::FieldPtr f = m_coll->fieldByName(QString::fromLatin1("url"));
        if(f && f->property(QString::fromLatin1("relative")) == Latin1Literal("true")) {
          return KURL(Document::self()->URL(), u) == KURL::fromPathOrURL(e1.field(QString::fromLatin1("url")));
        }
      }
    }
  }
  if(e1.m_fields.count() != m_fields.count()) {
    return false;
  }
  for(StringMap::ConstIterator it = e1.m_fields.begin(); it != e1.m_fields.end(); ++it) {
    if(!m_fields.contains(it.key()) || m_fields[it.key()] != it.data()) {
      return false;
    }
  }
  return true;
}

Entry::Entry(CollPtr coll_) : KShared(), m_coll(coll_), m_id(-1) {
#ifndef NDEBUG
  if(!coll_) {
    kdWarning() << "Entry() - null collection pointer!" << endl;
  }
#endif
}

Entry::Entry(CollPtr coll_, int id_) : KShared(), m_coll(coll_), m_id(id_) {
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

Entry& Entry::operator=(const Entry& other_) {
  if(this == &other_) return *this;

//  myDebug() << "Entry::operator=()" << endl;
  static_cast<KShared&>(*this) = static_cast<const KShared&>(other_);
  m_coll = other_.m_coll;
  m_id = other_.m_id;
  m_fields = other_.m_fields;
  m_formattedFields = other_.m_formattedFields;
  return *this;
}

Entry::~Entry() {
}

Tellico::Data::CollPtr Entry::collection() const {
  return m_coll;
}

void Entry::setCollection(CollPtr coll_) {
  if(coll_ == m_coll) {
    myDebug() << "Entry::setCollection() - already belongs to collection!" << endl;
    return;
  }
  m_coll = coll_;
  m_id = -1;
}

QString Entry::title() const {
  return formattedField(QString::fromLatin1("title"));
}

QString Entry::field(Data::FieldPtr field_, bool formatted_/*=false*/) const {
  return field(field_->name(), formatted_);
}

QString Entry::field(const QString& fieldName_, bool formatted_/*=false*/) const {
  if(formatted_) {
    return formattedField(fieldName_);
  }

  FieldPtr f = m_coll->fieldByName(fieldName_);
  if(!f) {
    return QString::null;
  }
  if(f->type() == Field::Dependent) {
    return dependentValue(this, f->description(), false);
  }

  if(!m_fields.isEmpty() && m_fields.contains(fieldName_)) {
    return m_fields[fieldName_];
  }
  return QString::null;
}

QString Entry::formattedField(Data::FieldPtr field_) const {
  return formattedField(field_->name());
}

QString Entry::formattedField(const QString& fieldName_) const {
  FieldPtr f = m_coll->fieldByName(fieldName_);
  if(!f) {
    return QString::null;
  }

  Field::FormatFlag flag = f->formatFlag();
  if(f->type() == Field::Dependent) {
    if(flag == Field::FormatNone) {
      return dependentValue(this, f->description(), false);
    } else {
      // format sub fields and whole string
      return Field::format(dependentValue(this, f->description(), true), flag);
    }
  }

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

QStringList Entry::fields(Data::FieldPtr field_, bool formatted_) const {
  return fields(field_->name(), formatted_);
}

QStringList Entry::fields(const QString& field_, bool formatted_) const {
  QString s = formatted_ ? formattedField(field_) : field(field_);
  if(s.isEmpty()) {
    return QStringList();
  }
  return Field::split(s, true);
}

bool Entry::setField(Data::FieldPtr field_, const QString& value_) {
  return setField(field_->name(), value_);
}

bool Entry::setField(const QString& name_, const QString& value_) {
  if(name_.isEmpty()) {
    kdWarning() << "Entry::setField() - empty field name for value: " << value_ << endl;
    return false;
  }
  // an empty value means remove the field
  if(value_.isEmpty()) {
    if(!m_fields.isEmpty() && m_fields.contains(name_)) {
      m_fields.remove(name_);
    }
    invalidateFormattedFieldValue(name_);
    return true;
  }

#ifndef NDEBUG
  if(m_coll && (m_coll->fields().count() == 0 || !m_coll->hasField(name_))) {
    myDebug() << "Entry::setField() - unknown collection entry field - "
              << name_ << endl;
    return false;
  }
#endif

  if(m_coll && !m_coll->isAllowed(name_, value_)) {
    myDebug() << "Entry::setField() - for " << name_
              << ", value is not allowed - " << value_ << endl;
    return false;
  }

  Data::FieldPtr f = m_coll->fieldByName(name_);
  if(!f) {
    return false;
  }

  // the string store is probable only useful for fields with auto-completion or choice/number/bool
  if(!(f->flags() & Field::AllowMultiple) &&
     ((f->type() == Field::Choice || f->type() == Field::Bool || f->type() == Field::Number) ||
      (f->type() == Field::Line && (f->flags() & Field::AllowCompletion)))) {
    m_fields.insert(Tellico::shareString(name_), Tellico::shareString(value_));
  } else {
    m_fields.insert(Tellico::shareString(name_), value_);
  }
  invalidateFormattedFieldValue(name_);
  return true;
}

bool Entry::addToGroup(EntryGroup* group_) {
  if(!group_ || m_groups.contains(group_)) {
    return false;
  }

  m_groups.push_back(group_);
  group_->append(this);
//  m_coll->groupModified(group_);
  return true;
}

bool Entry::removeFromGroup(EntryGroup* group_) {
  // if the removal isn't successful, just return
  bool success = m_groups.remove(group_);
  success = success && group_->remove(this);
//  myDebug() << "Entry::removeFromGroup() - removing from group - "
//              << group_->fieldName() << "::" << group_->groupName() << endl;
  if(success) {
//    m_coll->groupModified(group_);
  } else {
    myDebug() << "Entry::removeFromGroup() failed! " << endl;
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
//  myDebug() << "Entry::groupsByfieldName() - " << fieldName_ << endl;
  FieldPtr f = m_coll->fieldByName(fieldName_);

  // easy if not allowing multiple values
  if(!(f->flags() & Field::AllowMultiple)) {
    QString value = formattedField(fieldName_);
    if(value.isEmpty()) {
      return i18n(Collection::s_emptyGroupTitle);
    } else {
      return value;
    }
  }

  QStringList groups = fields(fieldName_, true);
  if(groups.isEmpty()) {
    return i18n(Collection::s_emptyGroupTitle);
  } else if(f->type() == Field::Table) {
      // quick hack for tables, how often will a user have "::" in their value?
      // only use first column for group
    QStringList::Iterator it = groups.begin();
    while(it != groups.end()) {
      (*it) = (*it).section(QString::fromLatin1("::"),  0,  0);
      if((*it).isEmpty()) {
        it = groups.remove(it); // points to next in list
      } else {
        ++it;
      }
    }
  }
  return groups;
}

bool Entry::isOwned() {
  return (m_coll && m_id > -1 && m_coll->entryCount() > 0 && m_coll->entries().contains(this));
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
QString Entry::dependentValue(ConstEntryPtr entry_, const QString& format_, bool formatted_) {
  if(!entry_) {
    return format_;
  }

  QString result, fieldName;
  FieldPtr field;

  int endPos;
  int curPos = 0;
  int pctPos = format_.find('%', curPos);
  while(pctPos != -1 && pctPos+1 < static_cast<int>(format_.length())) {
    if(format_[pctPos+1] == '{') {
      endPos = format_.find('}', pctPos+2);
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
//  myDebug() << "Entry::dependentValue() - " << format_ << " = " << result << endl;
  // sometimes field value might empty, resulting in multiple consecutive white spaces
  // so let's simplify that...
  return result.simplifyWhiteSpace();
}

int Entry::compareValues(EntryPtr e1, EntryPtr e2, const QString& f, ConstCollPtr c) {
  return compareValues(e1, e2, c->fieldByName(f));
}

int Entry::compareValues(EntryPtr e1, EntryPtr e2, FieldPtr f) {
  if(!e1 || !e2 || !f) {
    return 0;
  }
  QString s1 = e1->field(f).lower();
  QString s2 = e2->field(f).lower();
  if(s1.isEmpty() || s2.isEmpty()) {
    return 0;
  }
  // complicated string matching, here are the cases I want to match
  // "bend it like beckham" == "bend it like beckham (widescreen edition)"
  // "the return of the king" == "return of the king"
  if(s1 == s2) {
    return 5;
  }
  if(f->formatFlag() == Field::FormatName) {
    s1 = e1->field(f, true).lower();
    s2 = e2->field(f, true).lower();
    if(s1 == s2) {
      return 5;
    }
  }
  // try removing punctuation
  QRegExp notAlphaNum(QString::fromLatin1("[^\\s\\w]"));
  QString s1a = s1; s1a.remove(notAlphaNum);
  QString s2a = s2; s2a.remove(notAlphaNum);
  if(!s1a.isEmpty() && s1a == s2a) {
//    myDebug() << "match without punctuation" << endl;
    return 5;
  }
  Field::stripArticles(s1);
  Field::stripArticles(s2);
  if(!s1.isEmpty() && s1 == s2) {
//    myDebug() << "match without articles" << endl;
    return 3;
  }
  // try removing everything between parentheses
  QRegExp rx(QString::fromLatin1("\\s*\\(.*\\)\\s*"));
  s1.remove(rx);
  s2.remove(rx);
  if(!s1.isEmpty() && s1 == s2) {
//    myDebug() << "match without parentheses" << endl;
    return 2;
  }
  if(f->flags() & Field::AllowMultiple) {
    QStringList sl1 = e1->fields(f, false);
    QStringList sl2 = e2->fields(f, false);
    int matches = 0;
    for(QStringList::ConstIterator it = sl1.begin(); it != sl1.end(); ++it) {
      matches += sl2.contains(*it);
    }
    if(matches == 0 && f->formatFlag() == Field::FormatName) {
      sl1 = e1->fields(f, true);
      sl2 = e2->fields(f, true);
      for(QStringList::ConstIterator it = sl1.begin(); it != sl1.end(); ++it) {
        matches += sl2.contains(*it);
      }
    }
    return matches;
  }
  return 0;
}

#include "entry.moc"
