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

#include "collection.h"
#include "field.h"
#include "entry.h"
#include "entrygroup.h"
#include "derivedvalue.h"
#include "fieldformat.h"
#include "utils/string_utils.h"
#include "utils/stringset.h"
#include "entrycomparison.h"
#include "tellico_debug.h"

#include <KLocalizedString>

#include <QDate>

using namespace Tellico;
using Tellico::Data::Collection;

const QString Collection::s_peopleGroupName = QStringLiteral("_people");

Collection::Collection(const QString& title_)
    : QObject(), QSharedData(), m_nextEntryId(1), m_title(title_), m_trackGroups(false) {
  m_id = getID();
}

Collection::Collection(bool addDefaultFields_, const QString& title_)
    : QObject(), QSharedData(), m_nextEntryId(1), m_title(title_), m_trackGroups(false) {
  if(m_title.isEmpty()) {
    m_title = i18n("My Collection");
  }
  m_id = getID();
  if(addDefaultFields_) {
    addField(Field::createDefaultField(Field::IDField));
    addField(Field::createDefaultField(Field::TitleField));
    addField(Field::createDefaultField(Field::CreatedDateField));
    addField(Field::createDefaultField(Field::ModifiedDateField));
  }
}

Collection::~Collection() {
  // maybe we should just call clear() ?
  foreach(EntryGroupDict* dict, m_entryGroupDicts) {
    qDeleteAll(*dict);
  }
  qDeleteAll(m_entryGroupDicts);
  m_entryGroupDicts.clear();
}

bool Collection::addFields(Tellico::Data::FieldList list_) {
  bool success = true;
  foreach(FieldPtr field, list_) {
    success &= addField(field);
  }
  return success;
}

bool Collection::addField(Tellico::Data::FieldPtr field_) {
  Q_ASSERT(field_);
  if(!field_) {
    return false;
  }

  // this essentially checks for duplicates
  if(hasField(field_->name())) {
    myDebug() << "replacing" << field_->name() << "in collection" << m_title;
    removeField(fieldByName(field_->name()), true);
  }

  m_fields.append(field_);
  m_fieldByName.insert(field_->name(), field_.data());
  m_fieldByTitle.insert(field_->title(), field_.data());

  // always default to using field with title name as title
  if(field_->name() == QLatin1String("title")) m_titleField = field_->name();

  if(field_->formatType() == FieldFormat::FormatName) {
    m_peopleFields.append(field_); // list of people attributes
    if(m_peopleFields.count() > 1) {
      // the second time that a person field is added, add a "pseudo-group" for people
      if(!m_entryGroupDicts.contains(s_peopleGroupName)) {
        EntryGroupDict* d = new EntryGroupDict();
        m_entryGroupDicts.insert(s_peopleGroupName, d);
        m_entryGroups.prepend(s_peopleGroupName);
      }
    }
  } else if(m_titleField.isEmpty() && field_->formatType() == FieldFormat::FormatTitle) {
    m_titleField = field_->name();
  }

  if(field_->type() == Field::Image) {
    m_imageFields.append(field_);
  }

  if(!field_->category().isEmpty() && !m_fieldCategories.contains(field_->category())) {
    m_fieldCategories << field_->category();
  }

  if(field_->hasFlag(Field::AllowGrouped)) {
    // m_entryGroupsDicts autoDeletes each QDict when the Collection d'tor is called
    EntryGroupDict* dict = new EntryGroupDict();
    m_entryGroupDicts.insert(field_->name(), dict);
    // cache the possible groups of entries
    m_entryGroups << field_->name();
  }

  if(m_defaultGroupField.isEmpty() && field_->hasFlag(Field::AllowGrouped)) {
    m_defaultGroupField = field_->name();
  }

  if(field_->hasFlag(Field::Derived)) {
    DerivedValue dv(field_);
    if(dv.isRecursive(this)) {
      field_->setProperty(QStringLiteral("template"), QString());
    }
  }

  // refresh all dependent fields, in case one references this new one
  foreach(FieldPtr existingField, m_fields) {
    if(existingField->hasFlag(Field::Derived)) {
      Q_EMIT signalRefreshField(existingField);
    }
  }

  return true;
}

bool Collection::mergeField(Tellico::Data::FieldPtr newField_) {
  bool structuralChange = false;
  if(!newField_) {
    return structuralChange;
  }

  FieldPtr currField = fieldByName(newField_->name());
  if(!currField) {
    // does not exist in current collection, add it
    Data::FieldPtr f(new Field(*newField_));
    bool success = addField(f);
    if(success) {
      Q_EMIT mergeAddedField(CollPtr(this), f);
    } else {
      myDebug() << "Failed to add field:" << f->name();
    }
    // adding a new field is a structural change to the collection
    return success;
  }

  if(newField_->type() == Field::Table2) {
    newField_->setType(Data::Field::Table);
    newField_->setProperty(QStringLiteral("columns"), QStringLiteral("2"));
  }

  // the original field type is kept
  if(currField->type() != newField_->type()) {
    myDebug() << "skipping, field type mismatch for " << currField->title();
    return structuralChange;
  }

  // if field is a Choice, then make sure all values are there
  if(currField->type() == Field::Choice && currField->allowed() != newField_->allowed()) {
    QStringList allowed = currField->allowed();
    const QStringList& newAllowed = newField_->allowed();
    for(QStringList::ConstIterator it = newAllowed.begin(); it != newAllowed.end(); ++it) {
      if(!allowed.contains(*it)) {
        allowed.append(*it);
        structuralChange = true;
      }
    }
    currField->setAllowed(allowed);
  }

  // don't change original format flags
  // don't change original category
  // add new description if current is empty
  if(currField->description().isEmpty()) {
    currField->setDescription(newField_->description());
  }

  // if new field has additional extended properties, add those
  for(StringMap::const_iterator it = newField_->propertyList().begin(); it != newField_->propertyList().end(); ++it) {
    const QString propName = it.key();
    const QString currValue = currField->property(propName);
    if(currValue.isEmpty()) {
      currField->setProperty(propName, it.value());
      structuralChange = true;
    } else if (it.value() != currValue) {
      if(currField->type() == Field::URL && propName == QLatin1String("relative")) {
        myWarning() << "relative URL property does not match for " << currField->name();
      } else if((currField->type() == Field::Table && propName == QLatin1String("columns"))
             || (currField->type() == Field::Rating && propName == QLatin1String("maximum"))) {
        bool ok;
        uint currNum = Tellico::toUInt(currValue, &ok);
        uint newNum = Tellico::toUInt(it.value(), &ok);
        if(newNum > currNum) { // bigger values
          currField->setProperty(propName, QString::number(newNum));
          structuralChange = true;
        }
      } else if(currField->type() == Field::Rating && propName == QLatin1String("minimum")) {
        bool ok;
        uint currNum = Tellico::toUInt(currValue, &ok);
        uint newNum = Tellico::toUInt(it.value(), &ok);
        if(newNum < currNum) { // smaller values
          currField->setProperty(propName, QString::number(newNum));
          structuralChange = true;
        }
      }
    }
    if(propName == QLatin1String("template") && currField->hasFlag(Field::Derived)) {
      DerivedValue dv(currField);
      if(dv.isRecursive(this)) {
        currField->setProperty(QStringLiteral("template"), QString());
      }
    }
  }

  // combine flags
  currField->setFlags(currField->flags() | newField_->flags());
  return structuralChange;
}

// be really careful with these field pointers, try not to call too many other functions
// which may depend on the field list
bool Collection::modifyField(Tellico::Data::FieldPtr newField_) {
  if(!newField_) {
    return false;
  }
//  myDebug() << ";

// the field name never changes
  const QString fieldName = newField_->name();
  FieldPtr oldField = fieldByName(fieldName);
  if(!oldField) {
    myDebug() << "no field named " << fieldName;
    return false;
  }

  // update name dict
  m_fieldByName.insert(fieldName, newField_.data());

  // update titles
  const QString oldTitle = oldField->title();
  const QString newTitle = newField_->title();
  if(oldTitle == newTitle) {
    m_fieldByTitle.insert(newTitle, newField_.data());
  } else {
    m_fieldByTitle.remove(oldTitle);
    m_fieldByTitle.insert(newTitle, newField_.data());
  }

  // now replace the field pointer in the list
  int pos = m_fields.indexOf(oldField);
  if(pos > -1) {
    m_fields.replace(pos, newField_);
  } else {
    myDebug() << "no index found!";
    return false;
  }

  // update category list.
  if(oldField->category() != newField_->category()) {
    m_fieldCategories.clear();
    foreach(FieldPtr it, m_fields) {
      // add category if it's not in the list yet
      if(!it->category().isEmpty() && !m_fieldCategories.contains(it->category())) {
        m_fieldCategories += it->category();
      }
    }
  }

  if(newField_->hasFlag(Field::Derived)) {
    DerivedValue dv(newField_);
    if(dv.isRecursive(this)) {
      newField_->setProperty(QStringLiteral("template"), QString());
    }
  }

  // keep track of if the entry groups will need to be reset
  bool resetGroups = false;

  // if format is different, go ahead and invalidate all formatted entry values
  if(oldField->formatType() != newField_->formatType()) {
    // invalidate cached format strings of all entry attributes of this name
    foreach(EntryPtr entry, m_entries) {
      entry->invalidateFormattedFieldValue(fieldName);
    }
    resetGroups = true;
  }

  // check to see if the people "pseudo-group" needs to be updated
  // only if only one of the two is a name
  bool wasPeople = oldField->formatType() == FieldFormat::FormatName;
  bool isPeople = newField_->formatType() == FieldFormat::FormatName;
  if(wasPeople) {
    m_peopleFields.removeAll(oldField);
    if(!isPeople) {
      resetGroups = true;
    }
  }
  if(isPeople) {
    // if there's more than one people field and no people dict exists yet, add it
    if(m_peopleFields.count() > 1 && !m_entryGroupDicts.contains(s_peopleGroupName)) {
      EntryGroupDict* d = new EntryGroupDict();
      m_entryGroupDicts.insert(s_peopleGroupName, d);
      // put it at the top of the list
      m_entryGroups.prepend(s_peopleGroupName);
    }
    m_peopleFields.append(newField_);
    if(!wasPeople) {
      resetGroups = true;
    }
  }

  bool wasGrouped = oldField->hasFlag(Field::AllowGrouped);
  bool isGrouped = newField_->hasFlag(Field::AllowGrouped);
  if(wasGrouped) {
    if(!isGrouped) {
      // in order to keep list in the same order, don't remove unless new field is not groupable
      m_entryGroups.removeAll(fieldName);
      delete m_entryGroupDicts.take(fieldName); // no auto-delete here
      myDebug() << "no longer grouped: " << fieldName;
      resetGroups = true;
    } else {
      // don't do this, it wipes out the old groups!
//      m_entryGroupDicts.replace(fieldName, new EntryGroupDict());
    }
  } else if(isGrouped) {
    EntryGroupDict* d = new EntryGroupDict();
    m_entryGroupDicts.insert(fieldName, d);
    // cache the possible groups of entries
    m_entryGroups << fieldName;
    resetGroups = true;
  }

  if(oldField->type() == Field::Image) {
    m_imageFields.removeAll(oldField);
  }
  if(newField_->type() == Field::Image) {
    m_imageFields.append(newField_);
  }

  if(resetGroups) {
//    myLog() << "invalidating groups";
    invalidateGroups();
  }

  // now to update all entries if the field is a derived value and the template changed
  if(newField_->hasFlag(Field::Derived) &&
     oldField->property(QStringLiteral("template")) != newField_->property(QStringLiteral("template"))) {
    Q_EMIT signalRefreshField(newField_);
  }

  return true;
}

bool Collection::removeField(const QString& name_, bool force_) {
  return removeField(fieldByName(name_), force_);
}

// force allows me to force the deleting of the title field if I need to
bool Collection::removeField(Tellico::Data::FieldPtr field_, bool force_/*=false*/) {
  if(!field_ || !m_fields.contains(field_)) {
    if(field_) {
      myDebug() << "can't delete field:" << field_->name();
    }
    return false;
  }
//  myDebug() << "name = " << field_->name();

  // can't delete the title field
  if((field_->hasFlag(Field::NoDelete)) && !force_) {
    return false;
  }

  foreach(EntryPtr entry, m_entries) {
    // setting the fields to an empty string removes the value from the entry's list
    entry->setField(field_, QString());
  }

  bool success = true;
  if(field_->formatType() == FieldFormat::FormatName) {
    m_peopleFields.removeAll(field_);
  }

  if(field_->type() == Field::Image) {
    m_imageFields.removeAll(field_);
  }
  m_fieldByName.remove(field_->name());
  m_fieldByTitle.remove(field_->title());

  if(fieldsByCategory(field_->category()).count() == 1) {
    m_fieldCategories.removeAll(field_->category());
  }

  if(field_->hasFlag(Field::AllowGrouped)) {
    EntryGroupDict* dict = m_entryGroupDicts.take(field_->name());
    qDeleteAll(*dict);
    m_entryGroups.removeAll(field_->name());
    if(field_->name() == m_defaultGroupField && !m_entryGroups.isEmpty()) {
      setDefaultGroupField(m_entryGroups.first());
    }
  }

  m_fields.removeAll(field_);

  // refresh all dependent fields, rather lazy, but there's
  // likely to be weird effects when checking dependent fields
  // while removing one, so refresh all of them
  foreach(FieldPtr field, m_fields) {
    if(field->hasFlag(Field::Derived)) {
      Q_EMIT signalRefreshField(field);
    }
  }

  return success;
}

void Collection::reorderFields(const Tellico::Data::FieldList& list_) {
// assume the lists have the same pointers!
  m_fields = list_;

  // also reset category list, since the order may have changed
  m_fieldCategories.clear();
  foreach(FieldPtr field, m_fields) {
    if(!field->category().isEmpty() && !m_fieldCategories.contains(field->category())) {
      m_fieldCategories << field->category();
    }
  }
}

void Collection::addEntries(const Tellico::Data::EntryList& entries_) {
  if(entries_.isEmpty()) {
    return;
  }

  foreach(EntryPtr entry, entries_) {
    if(!entry) {
      Q_ASSERT(entry);
      continue;
    }
    bool foster = false;
    if(this != entry->collection().data()) {
      entry->setCollection(CollPtr(this));
      foster = true;
    }

    m_entries.append(entry);
//    myDebug() << "added entry (" << entry->title() << ")" <<  entry->id();

    if(entry->id() >= m_nextEntryId) {
      m_nextEntryId = entry->id() + 1;
    } else if(entry->id() == -1) {
      entry->setId(m_nextEntryId);
      ++m_nextEntryId;
    } else if(m_entryById.contains(entry->id())) {
      if(!foster) {
        myDebug() << "the collection already has an entry with id = " << entry->id();
      }
      entry->setId(m_nextEntryId);
      ++m_nextEntryId;
    }
    m_entryById.insert(entry->id(), entry.data());

    if(hasField(QStringLiteral("cdate")) && entry->field(QStringLiteral("cdate")).isEmpty()) {
      // use mdate if it exists
      QString cdate = entry->field(QStringLiteral("mdate"));
      if(cdate.isEmpty()) {
        cdate = QDate::currentDate().toString(Qt::ISODate);
      }
      entry->setField(QStringLiteral("cdate"), cdate, false);
    }
    if(hasField(QStringLiteral("mdate")) && entry->field(QStringLiteral("mdate")).isEmpty()) {
      entry->setField(QStringLiteral("mdate"), QDate::currentDate().toString(Qt::ISODate), false);
    }
  }
  if(m_trackGroups) {
    populateCurrentDicts(entries_, fieldNames());
  }
}

void Collection::removeEntriesFromDicts(const Tellico::Data::EntryList& entries_, const QStringList& fields_) {
  QSet<EntryGroup*> modifiedGroups;
  foreach(EntryPtr entry, entries_) {
    // need a copy of the vector since it gets changed
    QList<EntryGroup*> groups = entry->groups();
    foreach(EntryGroup* group, groups) {
      // only clear groups for the modified fields, skip the others
      // also clear for all derived values, just in case
      if(!fields_.contains(group->fieldName()) && hasField(group->fieldName()) && !fieldByName(group->fieldName())->hasFlag(Field::Derived))  {
        continue;
      }
      if(entry->removeFromGroup(group)) {
        modifiedGroups.insert(group);
      }
      if(group->isEmpty() && !m_groupsToDelete.contains(group)) {
        m_groupsToDelete.push_back(group);
      }
    }
  }
  if(!modifiedGroups.isEmpty()) {
    Q_EMIT signalGroupsModified(CollPtr(this), modifiedGroups.values());
  }
}

// this function gets called whenever an entry is modified. Its purpose is to keep the
// groupDicts current. It first removes the entry from every group to which it belongs,
// then it repopulates the dicts with the entry's fields
void Collection::updateDicts(const Tellico::Data::EntryList& entries_, const QStringList& fields_) {
  if(entries_.isEmpty() || !m_trackGroups) {
    return;
  }
  QStringList modifiedFields = fields_;
  if(modifiedFields.isEmpty()) {
//    myDebug() << "updating all fields";
    modifiedFields = fieldNames();
  }
  removeEntriesFromDicts(entries_, modifiedFields);
  populateCurrentDicts(entries_, modifiedFields);
  cleanGroups();
}

bool Collection::removeEntries(const Tellico::Data::EntryList& vec_) {
  if(vec_.isEmpty()) {
    return false;
  }

  removeEntriesFromDicts(vec_, fieldNames());
  bool success = true;
  foreach(EntryPtr entry, vec_) {
    m_entryById.remove(entry->id());
    m_entries.removeAll(entry);
  }
  cleanGroups();
  return success;
}

Tellico::Data::FieldList Collection::fieldsByCategory(const QString& cat_) {
  if(cat_.isEmpty()) {
    myDebug() << "empty category!";
    return FieldList();
  }

  FieldList list;
  foreach(FieldPtr field, m_fields) {
    if(field->category() == cat_) {
      list.append(field);
    }
  }
  return list;
}

QStringList Collection::fieldNames() const {
  return m_fieldByName.keys();
}

QStringList Collection::fieldTitles() const {
  return m_fieldByTitle.keys();
}

QStringList Collection::valuesByFieldName(const QString& name_) const {
  if(name_.isEmpty()) {
    return QStringList();
  }

  StringSet values;
  foreach(EntryPtr entry, m_entries) {
    values.add(FieldFormat::splitValue(entry->field(name_)));
  } // end entry loop

  return values.values();
}

Tellico::Data::FieldPtr Collection::fieldByName(const QString& name_) const {
  return FieldPtr(m_fieldByName.value(name_));
}

Tellico::Data::FieldPtr Collection::fieldByTitle(const QString& title_) const {
  return FieldPtr(m_fieldByTitle.value(title_));
}

bool Collection::hasField(const QString& name_) const {
  return m_fieldByName.contains(name_);
}

bool Collection::isAllowed(const QString& field_, const QString& value_) const {
  // empty string is always allowed
  if(value_.isEmpty()) {
    return true;
  }

  FieldPtr field = fieldByName(field_);
  if(!field) return false;
  // non-choice or single-value choice fields with allowed values
  if(field->type() != Field::Choice ||
     (!field->hasFlag(Field::AllowMultiple) && field->allowed().contains(value_))) {
    return true;
  }

  // now have to split the text into separate values and check each one
  const auto values = FieldFormat::splitValue(value_, FieldFormat::StringSplit);
  for(const auto& value : values) {
    if(!field->allowed().contains(value)) return false;
  }

  return true;
}

Tellico::Data::EntryGroupDict* Collection::entryGroupDictByName(const QString& name_) {
  m_lastGroupField = name_; // keep track, even if it's invalid
  if(name_.isEmpty() || !m_entryGroupDicts.contains(name_) || m_entries.isEmpty()) {
    return nullptr;
  }
  EntryGroupDict* dict = m_entryGroupDicts.value(name_);
  if(dict && dict->isEmpty()) {
    const bool b = signalsBlocked();
    // block signals so all the group created/modified signals don't fire
    blockSignals(true);
    populateDict(dict, name_, m_entries);
    blockSignals(b);
  }
  return dict;
}

void Collection::populateDict(Tellico::Data::EntryGroupDict* dict_, const QString& fieldName_, const Tellico::Data::EntryList& entries_) {
//  myDebug() << fieldName_;
  Q_ASSERT(dict_);
  auto f = fieldByName(fieldName_);
  const bool isBool = f && f->type() == Field::Bool;

  QSet<EntryGroup*> modifiedGroups;
  foreach(EntryPtr entry, entries_) {
    const QStringList groups = entryGroupNamesByField(entry, fieldName_);
    foreach(QString groupTitle, groups) { // krazy:exclude=foreach
      // find the group for this group name
      // bool fields use the field title
      if(isBool && !groupTitle.isEmpty()) {
        // the f value is valid since isBool is true
        groupTitle = f->title();
      }
      EntryGroup* group = dict_->value(groupTitle);
      // if the group doesn't exist, create it
      if(!group) {
        group = new EntryGroup(groupTitle, fieldName_);
        dict_->insert(groupTitle, group);
      } else if(group->isEmpty()) {
        // if it's empty, then it was previously added to the vector of groups to delete
        // remove it from that vector now that we're adding to it
        m_groupsToDelete.removeOne(group);
      }
      if(entry->addToGroup(group)) {
        modifiedGroups.insert(group);
      }
    } // end group loop
  } // end entry loop
  if(!modifiedGroups.isEmpty()) {
    Q_EMIT signalGroupsModified(CollPtr(this), modifiedGroups.values());
  }
}

void Collection::populateCurrentDicts(const Tellico::Data::EntryList& entries_, const QStringList& fields_) {
  if(m_entryGroupDicts.isEmpty()) {
    return;
  }

  // special case when adding an entry to a new empty collection
  // there are no existing non-empty groups
  bool allEmpty = true;

  // iterate over all the possible groupDicts
  // for each dict, get the value of that field for the entry
  // if multiple values are allowed, split the value and then insert the
  // entry pointer into the dict for each value
  QHash<QString, EntryGroupDict*>::const_iterator dictIt = m_entryGroupDicts.constBegin();
  for( ; dictIt != m_entryGroupDicts.constEnd(); ++dictIt) {
    // skip dicts for fields not in the modified list
    if(!fields_.contains(dictIt.key())) {
      continue;
    }
    // only populate if it's not empty, since they are
    // populated on demand
    if(!dictIt.value()->isEmpty()) {
      populateDict(dictIt.value(), dictIt.key(), entries_);
      allEmpty = false;
    }
  }

  if(allEmpty) {
//    myDebug() << "all collection dicts are empty";
    // still need to populate the current group dict
    EntryGroupDict* dict = m_entryGroupDicts.value(m_lastGroupField);
    if(dict) {
      populateDict(dict, m_lastGroupField, entries_);
    }
  }
}

// return a string list for all the groups that the entry belongs to
// for a given field. Normally, this would just be splitting the entry's value
// for the field, but if the field name is the people pseudo-group, then it gets
// a bit more complicated
QStringList Collection::entryGroupNamesByField(Tellico::Data::EntryPtr entry_, const QString& fieldName_) {
  if(fieldName_ != s_peopleGroupName) {
    return entry_->groupNamesByFieldName(fieldName_);
  }

  // the empty group is only returned if the entry has an empty list for every people field
  bool allEmpty = true;
  StringSet values;
  foreach(FieldPtr field, m_peopleFields) {
    const QStringList groups = entry_->groupNamesByFieldName(field->name());
    if(allEmpty && (groups.count() != 1 || !groups.at(0).isEmpty())) {
      allEmpty = false;
    }
    values.add(groups);
  }
  if(!allEmpty) {
    // we don't want the empty string
    values.remove(QString());
  }
  return values.values();
}

void Collection::invalidateGroups() {
  foreach(EntryGroupDict* dict, m_entryGroupDicts) {
    qDeleteAll(*dict);
    dict->clear();
    // don't delete the dict, just clear it
  }

  // populateDicts() will make signals that the group view is connected to, block those
  blockSignals(true);
  foreach(EntryPtr entry, m_entries) {
    entry->invalidateFormattedFieldValue();
    entry->clearGroups();
  }
  blockSignals(false);
}

Tellico::Data::EntryPtr Collection::entryById(Data::ID id_) {
  return EntryPtr(m_entryById.value(id_));
}

void Collection::addBorrower(Tellico::Data::BorrowerPtr borrower_) {
  if(!borrower_) {
    return;
  }
  // check against existing borrower uid
  BorrowerPtr existingBorrower;
  foreach(BorrowerPtr bor, m_borrowers) {
    if(bor->uid() == borrower_->uid()) {
      existingBorrower = bor;
      break;
    }
  }
  if(!existingBorrower) {
    m_borrowers.append(borrower_);
  } else if(existingBorrower != borrower_) {
    // need to merge loans
    QHash<QString, LoanPtr> existingLoans;
    foreach(LoanPtr loan, existingBorrower->loans()) {
      existingLoans.insert(loan->uid(), loan);
    }
    foreach(LoanPtr loan, borrower_->loans()) {
      if(!existingLoans.contains(loan->uid())) {
        existingBorrower->addLoan(loan);
      }
    }
  }
}

void Collection::addFilter(Tellico::FilterPtr filter_) {
  if(!filter_) {
    return;
  }

  m_filters.append(filter_);
}

bool Collection::removeFilter(Tellico::FilterPtr filter_) {
  if(!filter_) {
    return false;
  }

  return m_filters.removeAll(filter_) > 0;
}

void Collection::clear() {
  // since the collection holds a pointer to each entry and each entry
  // hold a pointer to the collection, and they're both sharedptrs,
  // neither will ever get deleted, unless the collection removes
  // all held pointers, specifically to entries
  m_fields.clear();
  m_peopleFields.clear();
  m_imageFields.clear();
  m_fieldCategories.clear();
  m_fieldByName.clear();
  m_fieldByTitle.clear();
  m_defaultGroupField.clear();

  m_entries.clear();
  m_entryById.clear();
  foreach(EntryGroupDict* dict, m_entryGroupDicts) {
    qDeleteAll(*dict);
  }
  qDeleteAll(m_entryGroupDicts);
  m_entryGroupDicts.clear();
  m_entryGroups.clear();
  m_groupsToDelete.clear();
  m_filters.clear();
  m_borrowers.clear();
}

void Collection::cleanGroups() {
  foreach(EntryGroup* group, m_groupsToDelete) {
    EntryGroupDict* dict = entryGroupDictByName(group->fieldName());
    if(!dict) {
      continue;
    }
    EntryGroup* groupToDelete = dict->take(group->groupName());
    delete groupToDelete;
  }
  m_groupsToDelete.clear();
}

QString Collection::prepareText(const QString& text_) const {
  return text_;
}

int Collection::sameEntry(Tellico::Data::EntryPtr entry1_, Tellico::Data::EntryPtr entry2_) const {
  if(!entry1_ || !entry2_) {
    return 0;
  }
  // used to just return 0, but we really want a default generic implementation
  // that specific collections can override.

  int res = 0;
  // start with twice the title score
  // and since the minimum is > 10, then need more than just a perfect title match
  res += EntryComparison::MATCH_WEIGHT_MED*EntryComparison::score(entry1_, entry2_, QStringLiteral("title"), this);
  // then add score for each field
  foreach(FieldPtr field, entry1_->collection()->fields()) {
    // skip title field and personal category
    if(field->name() == QLatin1String("title") ||
       field->category() == i18n("Personal")) {
      continue;
    }
    // url link is extra
    if(field->type() == Field::URL) {
      res += EntryComparison::MATCH_WEIGHT_MED*EntryComparison::score(entry1_, entry2_, field->name(), this);
    } else {
      res += EntryComparison::MATCH_WEIGHT_LOW*EntryComparison::score(entry1_, entry2_, field->name(), this);
    }
    if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;
  }
  return res;
}

Tellico::Data::ID Collection::getID() {
  static ID id = 0;
  return ++id;
}

Data::FieldPtr Collection::primaryImageField() const {
  return m_imageFields.isEmpty() ? Data::FieldPtr() : fieldByName(m_imageFields.front()->name());
}

QString Collection::titleField() const {
  return m_titleField.isEmpty()
    ? (m_fields.isEmpty() ? QString() : m_fields.at(0)->name())
    : m_titleField;
}
