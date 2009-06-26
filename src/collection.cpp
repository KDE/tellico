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
#include "tellico_utils.h"
#include "utils/stringset.h"
#include "entrycomparison.h"
#include "tellico_debug.h"

#include <klocale.h>

#include <QRegExp>
#include <QStack>

using Tellico::Data::Collection;

const QString Collection::s_peopleGroupName = QLatin1String("_people");

Collection::Collection(const QString& title_)
    : QObject(), QSharedData(), m_nextEntryId(0), m_title(title_), m_trackGroups(false) {
  m_id = getID();
}

Collection::Collection(bool addDefaultFields_, const QString& title_)
    : QObject(), QSharedData(), m_nextEntryId(0), m_title(title_), m_trackGroups(false) {
  m_id = getID();
  if(m_title.isEmpty()) {
    m_title = i18n("My Collection");
  }
  if(addDefaultFields_) {
    Data::FieldPtr field(new Field(QLatin1String("title"), i18n("Title")));
    field->setCategory(i18n("General"));
    field->setFlags(Field::NoDelete);
    field->setFormatFlag(Field::FormatTitle);
    addField(field);
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
  if(!field_) {
    return false;
  }

  // this essentially checks for duplicates
  if(hasField(field_->name())) {
    myDebug() << "replacing " << field_->name();
    removeField(fieldByName(field_->name()), true);
  }

  // it's not sufficient to merely check the new field
  if(dependentFieldHasRecursion(field_)) {
    field_->setDescription(QString());
  }

  m_fields.append(field_);
  if(field_->formatFlag() == Field::FormatName) {
    m_peopleFields.append(field_); // list of people attributes
    if(m_peopleFields.count() > 1) {
      // the second time that a person field is added, add a "pseudo-group" for people
      if(!m_entryGroupDicts.contains(s_peopleGroupName)) {
        EntryGroupDict* d = new EntryGroupDict();
        m_entryGroupDicts.insert(s_peopleGroupName, d);
        m_entryGroups.prepend(s_peopleGroupName);
      }
    }
  }
  m_fieldNameDict.insert(field_->name(), field_.data());
  m_fieldTitleDict.insert(field_->title(), field_.data());
  m_fieldNames << field_->name();
  m_fieldTitles << field_->title();
  if(field_->type() == Field::Image) {
    m_imageFields.append(field_);
  }

  if(!field_->category().isEmpty() && !m_fieldCategories.contains(field_->category())) {
    m_fieldCategories << field_->category();
  }

  if(field_->flags() & Field::AllowGrouped) {
    // m_entryGroupsDicts autoDeletes each QDict when the Collection d'tor is called
    EntryGroupDict* dict = new EntryGroupDict();
    m_entryGroupDicts.insert(field_->name(), dict);
    // cache the possible groups of entries
    m_entryGroups << field_->name();
  }

  if(m_defaultGroupField.isEmpty() && field_->flags() & Field::AllowGrouped) {
    m_defaultGroupField = field_->name();
  }

  // refresh all dependent fields, in case one references this new one
  foreach(FieldPtr existingField, m_fields) {
    if(existingField->type() == Field::Dependent) {
      emit signalRefreshField(existingField);
    }
  }

  return true;
}

bool Collection::mergeField(Tellico::Data::FieldPtr newField_) {
  if(!newField_) {
    return false;
  }

  FieldPtr currField = fieldByName(newField_->name());
  if(!currField) {
    // does not exist in current collection, add it
    Data::FieldPtr f(new Field(*newField_));
    bool success = addField(f);
    emit mergeAddedField(CollPtr(this), f);
    return success;
  }

  if(newField_->type() == Field::Table2) {
    newField_->setType(Data::Field::Table);
    newField_->setProperty(QLatin1String("columns"), QLatin1String("2"));
  }

  // the original field type is kept
  if(currField->type() != newField_->type()) {
    myDebug() << "skipping, field type mismatch for " << currField->title();
    return false;
  }

  // if field is a Choice, then make sure all values are there
  if(currField->type() == Field::Choice && currField->allowed() != newField_->allowed()) {
    QStringList allowed = currField->allowed();
    const QStringList& newAllowed = newField_->allowed();
    for(QStringList::ConstIterator it = newAllowed.begin(); it != newAllowed.end(); ++it) {
      if(!allowed.contains(*it)) {
        allowed.append(*it);
      }
    }
    currField->setAllowed(allowed);
  }

  // don't change original format flags
  // don't change original category
  // add new description if current is empty
  if(currField->description().isEmpty()) {
    currField->setDescription(newField_->description());
    if(dependentFieldHasRecursion(currField)) {
      currField->setDescription(QString());
    }
  }

  // if new field has additional extended properties, add those
  for(StringMap::const_iterator it = newField_->propertyList().begin(); it != newField_->propertyList().end(); ++it) {
    const QString propName = it.key();
    const QString currValue = currField->property(propName);
    if(currValue.isEmpty()) {
      currField->setProperty(propName, it.value());
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
        }
      } else if(currField->type() == Field::Rating && propName == QLatin1String("minimum")) {
        bool ok;
        uint currNum = Tellico::toUInt(currValue, &ok);
        uint newNum = Tellico::toUInt(it.value(), &ok);
        if(newNum < currNum) { // smaller values
          currField->setProperty(propName, QString::number(newNum));
        }
      }
    }
  }

  // combine flags
  currField->setFlags(currField->flags() | newField_->flags());
  return true;
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
  m_fieldNameDict.insert(fieldName, newField_.data());

  // update titles
  const QString oldTitle = oldField->title();
  const QString newTitle = newField_->title();
  if(oldTitle == newTitle) {
    m_fieldTitleDict.insert(newTitle, newField_.data());
  } else {
    m_fieldTitleDict.remove(oldTitle);
    m_fieldTitles.removeAll(oldTitle);
    m_fieldTitleDict.insert(newTitle, newField_.data());
    m_fieldTitles.append(newTitle);
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

  if(dependentFieldHasRecursion(newField_)) {
    newField_->setDescription(QString());
  }

  // keep track of if the entry groups will need to be reset
  bool resetGroups = false;

  // if format is different, go ahead and invalidate all formatted entry values
  if(oldField->formatFlag() != newField_->formatFlag()) {
    // invalidate cached format strings of all entry attributes of this name
    foreach(EntryPtr it, m_entries) {
      it->invalidateFormattedFieldValue(fieldName);
    }
    resetGroups = true;
  }

  // check to see if the people "pseudo-group" needs to be updated
  // only if only one of the two is a name
  bool wasPeople = oldField->formatFlag() == Field::FormatName;
  bool isPeople = newField_->formatFlag() == Field::FormatName;
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

  bool wasGrouped = oldField->flags() & Field::AllowGrouped;
  bool isGrouped = newField_->flags() & Field::AllowGrouped;
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
    if(!wasGrouped) {
      // cache the possible groups of entries
      m_entryGroups << fieldName;
    }
    resetGroups = true;
  }

  if(oldField->type() == Field::Image) {
    m_imageFields.removeAll(oldField);
  }
  if(newField_->type() == Field::Image) {
    m_imageFields.append(newField_);
  }

  if(resetGroups) {
    myLog() << "invalidating groups";
    invalidateGroups();
  }

  // now to update all entries if the field is a dependent and the description changed
  if(newField_->type() == Field::Dependent && oldField->description() != newField_->description()) {
    emit signalRefreshField(newField_);
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
      myDebug() << "false: " << field_->name();
    }
    return false;
  }
//  myDebug() << "name = " << field_->name();

  // can't delete the title field
  if((field_->flags() & Field::NoDelete) && !force_) {
    return false;
  }

  bool success = true;
  if(field_->formatFlag() == Field::FormatName) {
    m_peopleFields.removeAll(field_);
  }

  if(field_->type() == Field::Image) {
    m_imageFields.removeAll(field_);
  }
  m_fieldNameDict.remove(field_->name());
  m_fieldTitleDict.remove(field_->title());
  m_fieldNames.removeAll(field_->name());
  m_fieldTitles.removeAll(field_->title());

  if(fieldsByCategory(field_->category()).count() == 1) {
    m_fieldCategories.removeAll(field_->category());
  }

  foreach(EntryPtr entry, m_entries) {
    // setting the fields to an empty string removes the value from the entry's list
    entry->setField(field_, QString());
  }

  if(field_->flags() & Field::AllowGrouped) {
    EntryGroupDict* dict = m_entryGroupDicts.take(field_->name());
    qDeleteAll(*dict);
    m_entryGroups.removeAll(field_->name());
    if(field_->name() == m_defaultGroupField) {
      setDefaultGroupField(m_entryGroups[0]);
    }
  }

  m_fields.removeAll(field_);

  // refresh all dependent fields, rather lazy, but there's
  // likely to be weird effects when checking dependent fields
  // while removing one, so refresh all of them
  foreach(FieldPtr field, m_fields) {
    if(field->type() == Field::Dependent) {
      emit signalRefreshField(field);
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
    bool foster = false;
    if(CollPtr(this) != entry->collection()) {
      entry->setCollection(CollPtr(this));
      foster = true;
    }

    m_entries.append(entry);
//  myDebug() << "added entry (" << entry->title() << ")";

    if(entry->id() >= m_nextEntryId) {
      m_nextEntryId = entry->id() + 1;
    } else if(entry->id() == -1) {
      entry->setId(m_nextEntryId);
      ++m_nextEntryId;
    } else if(m_entryIdDict[entry->id()]) {
      if(!foster) {
        myDebug() << "the collection already has an entry with id = " << entry->id();
      }
      entry->setId(m_nextEntryId);
      ++m_nextEntryId;
    }
    m_entryIdDict.insert(entry->id(), entry.data());
  }
  if(m_trackGroups) {
    populateCurrentDicts(entries_);
  }
}

void Collection::removeEntriesFromDicts(const Tellico::Data::EntryList& entries_) {
  QList<EntryGroup*> modifiedGroups;
  foreach(EntryPtr entry, entries_) {
    // need a copy of the vector since it gets changed
    QList<EntryGroup*> groups = entry->groups();
    foreach(EntryGroup* group, groups) {
      if(entry->removeFromGroup(group) && !modifiedGroups.contains(group)) {
        modifiedGroups.append(group);
      }
      if(group->isEmpty() && !m_groupsToDelete.contains(group)) {
        m_groupsToDelete.push_back(group);
      }
    }
  }
  emit signalGroupsModified(CollPtr(this), modifiedGroups);
}

// this function gets called whenever an entry is modified. Its purpose is to keep the
// groupDicts current. It first removes the entry from every group to which it belongs,
// then it repopulates the dicts with the entry's fields
void Collection::updateDicts(const Tellico::Data::EntryList& entries_) {
  if(entries_.isEmpty() || !m_trackGroups) {
    return;
  }

  removeEntriesFromDicts(entries_);
  populateCurrentDicts(entries_);
  cleanGroups();
}

bool Collection::removeEntries(const Tellico::Data::EntryList& vec_) {
  if(vec_.isEmpty()) {
    return false;
  }

//  myDebug() << "deleted entry - " << entry_->title();
  removeEntriesFromDicts(vec_);
  bool success = true;
  foreach(EntryPtr entry, vec_) {
    m_entryIdDict.remove(entry->id());
    m_entries.removeAll(entry);
  }
  cleanGroups();
  return success;
}

Tellico::Data::FieldList Collection::fieldsByCategory(const QString& cat_) {
#ifndef NDEBUG
  if(!m_fieldCategories.contains(cat_)) {
    myDebug() << cat_ << "' is not in category list";
  }
#endif
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

QString Collection::fieldNameByTitle(const QString& title_) const {
  if(title_.isEmpty()) {
    return QString();
  }
  FieldPtr f = fieldByTitle(title_);
  if(!f) { // might happen in MainWindow::saveCollectionOptions
    return QString();
  }
  return f->name();
}

QString Collection::fieldTitleByName(const QString& name_) const {
  if(name_.isEmpty()) {
    return QString();
  }
  FieldPtr f = fieldByName(name_);
  if(!f) {
    myWarning() << "no field named " << name_;
    return QString();
  }
  return f->title();
}

QStringList Collection::valuesByFieldName(const QString& name_) const {
  if(name_.isEmpty()) {
    return QStringList();
  }
  bool multiple = (fieldByName(name_)->flags() & Field::AllowMultiple);

  StringSet values;
  foreach(EntryPtr entry, m_entries) {
    if(multiple) {
      values.add(entry->fields(name_, false));
    } else {
      values.add(entry->field(name_));
    }
  } // end entry loop

  return values.toList();
}

Tellico::Data::FieldPtr Collection::fieldByName(const QString& name_) const {
  return FieldPtr(m_fieldNameDict[name_]);
}

Tellico::Data::FieldPtr Collection::fieldByTitle(const QString& title_) const {
  return FieldPtr(m_fieldTitleDict[title_]);
}

bool Collection::hasField(const QString& name_) const {
  return fieldByName(name_);
}

bool Collection::isAllowed(const QString& key_, const QString& value_) const {
  // empty string is always allowed
  if(value_.isEmpty()) {
    return true;
  }

  // find the field with a name of 'key_'
  FieldPtr field = fieldByName(key_);

  // if the type is not multiple choice or if value_ is allowed, return true
  if(field && (field->type() != Field::Choice || field->allowed().contains(value_))) {
    return true;
  }

  return false;
}

Tellico::Data::EntryGroupDict* Collection::entryGroupDictByName(const QString& name_) {
//  myDebug() << name_;
  m_lastGroupField = name_; // keep track, even if it's invalid
  if(name_.isEmpty() || !m_entryGroupDicts.contains(name_) || m_entries.isEmpty()) {
    return 0;
  }
  EntryGroupDict* dict = m_entryGroupDicts[name_];
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
  bool isBool = hasField(fieldName_) && fieldByName(fieldName_)->type() == Field::Bool;

  QList<EntryGroup*> modifiedGroups;
  foreach(EntryPtr entry, entries_) {
    const QStringList groups = entryGroupNamesByField(entry, fieldName_);
    foreach(QString groupTitle, groups) { // krazy:exclude=foreach
      // find the group for this group name
      // bool fields use the field title
      if(isBool && !groupTitle.isEmpty()) {
        groupTitle = fieldTitleByName(fieldName_);
      }
      EntryGroup* group = (*dict_)[groupTitle];
      // if the group doesn't exist, create it
      if(!group) {
        group = new EntryGroup(groupTitle, fieldName_);
        dict_->insert(groupTitle, group);
      } else if(group->isEmpty()) {
        // if it's empty, then it was added to the vector of groups to delete
        // remove it from that vector now that we're adding to it
        m_groupsToDelete.removeAll(group);
      }
      if(entry->addToGroup(group)) {
        modifiedGroups.push_back(group);
      }
    } // end group loop
  } // end entry loop
  emit signalGroupsModified(CollPtr(this), modifiedGroups);
}

void Collection::populateCurrentDicts(const Tellico::Data::EntryList& entries_) {
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
    EntryGroupDict* dict = m_entryGroupDicts[m_lastGroupField];
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

  StringSet values;
  foreach(FieldPtr field, m_peopleFields) {
    values.add(entry_->groupNamesByFieldName(field->name()));
  }
  values.remove(QString());
  return values.toList();
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

Tellico::Data::EntryPtr Collection::entryById(ID id_) {
  return EntryPtr(m_entryIdDict[id_]);
}

void Collection::addBorrower(Tellico::Data::BorrowerPtr borrower_) {
  if(!borrower_) {
    return;
  }
  m_borrowers.append(borrower_);
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

  // TODO: test for success
  m_filters.removeAll(filter_);
  return true;
}

void Collection::clear() {
  // since the collection holds a pointer to each entry and each entry
  // hold a pointer to the collection, and they're both sharedptrs,
  // neither will ever get deleted, unless the collection removes
  // all held pointers, specifically to entries
  m_fields.clear();
  m_peopleFields.clear();
  m_imageFields.clear();
  m_fieldNameDict.clear();
  m_fieldTitleDict.clear();

  m_entries.clear();
  m_entryIdDict.clear();
  foreach(EntryGroupDict* dict, m_entryGroupDicts) {
    qDeleteAll(*dict);
  }
  qDeleteAll(m_entryGroupDicts);
  m_entryGroupDicts.clear();
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

Tellico::Data::FieldList Collection::fieldDependsOn(FieldPtr field_) const {
  FieldList vec;
  if(field_->type() != Field::Dependent) {
    return vec;
  }

  const QStringList fieldNames = field_->dependsOn();
  // do NOT call recursively!
  foreach(const QString& fieldName, fieldNames) {
    FieldPtr targetField = fieldByName(fieldName);
    if(!targetField) {
      // allow the user to also use field titles
      targetField = fieldByTitle(fieldName);
    }
    if(targetField) {
      vec.append(targetField);
    }
  }
  return vec;
}

QString Collection::prepareText(const QString& text_) const {
  return text_;
}

bool Collection::dependentFieldHasRecursion(Tellico::Data::FieldPtr field_) {
  if(!field_ || field_->type() != Field::Dependent) {
    return false;
  }

  StringSet fieldNamesFound;
  fieldNamesFound.add(field_->name());
  QStack<FieldPtr> fieldsToCheck;
  fieldsToCheck.push(field_);
  while(!fieldsToCheck.isEmpty()) {
    FieldPtr f = fieldsToCheck.pop();
    const QStringList depFields = f->dependsOn();
    foreach(const QString& depFieldName, depFields) {
      if(fieldNamesFound.has(depFieldName)) {
        myLog() << "found recursion for" << field_->name() << ": refers to" << depFieldName << "more than one";
        // we have recursion
        return true;
      }
      fieldNamesFound.add(depFieldName);
      FieldPtr f = fieldByName(depFieldName);
      if(!f) {
        f = fieldByTitle(depFieldName);
      }
      if(f) {
        fieldsToCheck.push(f);
      }
    }
  }
  return false;
}

int Collection::sameEntry(Tellico::Data::EntryPtr entry1_, Tellico::Data::EntryPtr entry2_) const {
  if(!entry1_ || !entry2_) {
    return 0;
  }
  // used to just return 0, but we really want a default generic implementation
  // that specific collections can override.

  // start with twice the title score
  // and since the minimum is > 10, then need more than just a perfect title match
  int res = 2*EntryComparison::score(entry1_, entry2_, QLatin1String("title"), this);
  // then add score for each field
  FieldList fields = entry1_->collection()->fields();
  foreach(FieldPtr field, fields) {
    res += EntryComparison::score(entry1_, entry2_, field->name(), this);
  }
  return res;
}

Tellico::Data::ID Collection::getID() {
  static ID id = 0;
  return ++id;
}

#include "collection.moc"
