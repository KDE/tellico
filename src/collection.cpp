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

#include "collection.h"
#include "field.h"
#include "entry.h"
#include "tellico_debug.h"
#include "latin1literal.h"
#include "tellico_utils.h"
#include "controller.h"
#include "collectionfactory.h"
#include "stringset.h"
#include "tellico_kernel.h"

#include <klocale.h>

#include <qregexp.h>
#include <qvaluestack.h>

using Tellico::Data::Collection;

const char* Collection::s_emptyGroupTitle = I18N_NOOP("(Empty)");
const QString Collection::s_peopleGroupName = QString::fromLatin1("_people");

Collection::Collection(const QString& title_)
    : QObject(), KShared(), m_nextEntryId(0), m_title(title_), m_entryIdDict(997)
    , m_trackGroups(false) {
  m_entryGroupDicts.setAutoDelete(true);

  m_id = getID();
}

Collection::~Collection() {
}

QString Collection::typeName() const {
  return CollectionFactory::typeName(type());
}

bool Collection::addFields(FieldVec list_) {
  bool success = true;
  for(FieldVec::Iterator it = list_.begin(); it != list_.end(); ++it) {
    success &= addField(it);
  }
  return success;
}

bool Collection::addField(FieldPtr field_) {
  if(!field_) {
    return false;
  }

  // this essentially checks for duplicates
  if(hasField(field_->name())) {
    myDebug() << "Collection::addField() - replacing " << field_->name() << endl;
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
      if(m_entryGroupDicts.find(s_peopleGroupName) == 0) {
        EntryGroupDict* d = new EntryGroupDict();
        d->setAutoDelete(true);
        m_entryGroupDicts.insert(s_peopleGroupName, d);
        m_entryGroups.prepend(s_peopleGroupName);
      }
    }
  }
  m_fieldNameDict.insert(field_->name(), field_);
  m_fieldTitleDict.insert(field_->title(), field_);
  m_fieldNames << field_->name();
  m_fieldTitles << field_->title();
  if(field_->type() == Field::Image) {
    m_imageFields.append(field_);
  }

  if(!field_->category().isEmpty() && m_fieldCategories.findIndex(field_->category()) == -1) {
    m_fieldCategories << field_->category();
  }

  if(field_->flags() & Field::AllowGrouped) {
    // m_entryGroupsDicts autoDeletes each QDict when the Collection d'tor is called
    EntryGroupDict* dict = new EntryGroupDict();
    dict->setAutoDelete(true);
    m_entryGroupDicts.insert(field_->name(), dict);
    // cache the possible groups of entries
    m_entryGroups << field_->name();
  }

  if(m_defaultGroupField.isEmpty() && field_->flags() & Field::AllowGrouped) {
    m_defaultGroupField = field_->name();
  }

  // refresh all dependent fields, in case one references this new one
  for(FieldVec::Iterator it = m_fields.begin(); it != m_fields.end(); ++it) {
    if(it->type() == Field::Dependent) {
      emit signalRefreshField(it);
    }
  }

  return true;
}

bool Collection::mergeField(FieldPtr newField_) {
  if(!newField_) {
    return false;
  }

  FieldPtr currField = fieldByName(newField_->name());
  if(!currField) {
    // does not exist in current collection, add it
    Data::FieldPtr f = new Field(*newField_);
    bool success = addField(f);
    Controller::self()->addedField(this, f);
    return success;
  }

  if(newField_->type() == Field::Table2) {
    newField_->setType(Data::Field::Table);
    newField_->setProperty(QString::fromLatin1("columns"), QChar('2'));
  }

  // the original field type is kept
  if(currField->type() != newField_->type()) {
    myDebug() << "Collection::mergeField() - skipping, field type mismatch for " << currField->title() << endl;
    return false;
  }

  // if field is a Choice, then make sure all values are there
  if(currField->type() == Field::Choice && currField->allowed() != newField_->allowed()) {
    QStringList allowed = currField->allowed();
    const QStringList& newAllowed = newField_->allowed();
    for(QStringList::ConstIterator it = newAllowed.begin(); it != newAllowed.end(); ++it) {
      if(allowed.findIndex(*it) == -1) {
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
  for(StringMap::ConstIterator it = newField_->propertyList().begin(); it != newField_->propertyList().end(); ++it) {
    const QString propName = it.key();
    const QString currValue = currField->property(propName);
    if(currValue.isEmpty()) {
      currField->setProperty(propName, it.data());
    } else if (it.data() != currValue) {
      if(currField->type() == Field::URL && propName == Latin1Literal("relative")) {
        kdWarning() << "Collection::mergeField() - relative URL property does not match for " << currField->name() << endl;
      } else if((currField->type() == Field::Table && propName == Latin1Literal("columns"))
             || (currField->type() == Field::Rating && propName == Latin1Literal("maximum"))) {
        bool ok;
        uint currNum = Tellico::toUInt(currValue, &ok);
        uint newNum = Tellico::toUInt(it.data(), &ok);
        if(newNum > currNum) { // bigger values
          currField->setProperty(propName, QString::number(newNum));
        }
      } else if(currField->type() == Field::Rating && propName == Latin1Literal("minimum")) {
        bool ok;
        uint currNum = Tellico::toUInt(currValue, &ok);
        uint newNum = Tellico::toUInt(it.data(), &ok);
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
bool Collection::modifyField(FieldPtr newField_) {
  if(!newField_) {
    return false;
  }
//  myDebug() << "Collection::modifyField() - " << newField_->name() << endl;

// the field name never changes
  const QString fieldName = newField_->name();
  FieldPtr oldField = fieldByName(fieldName);
  if(!oldField) {
    myDebug() << "Collection::modifyField() - no field named " << fieldName << endl;
    return false;
  }

  // update name dict
  m_fieldNameDict.replace(fieldName, newField_);

  // update titles
  const QString oldTitle = oldField->title();
  const QString newTitle = newField_->title();
  if(oldTitle == newTitle) {
    m_fieldTitleDict.replace(newTitle, newField_);
  } else {
    m_fieldTitleDict.remove(oldTitle);
    m_fieldTitles.remove(oldTitle);
    m_fieldTitleDict.insert(newTitle, newField_);
    m_fieldTitles.append(newTitle);
  }

  // now replace the field pointer in the list
  FieldVec::Iterator it = m_fields.find(oldField);
  if(it != m_fields.end()) {
    m_fields.insert(it, newField_);
    m_fields.remove(oldField);
  } else {
    myDebug() << "Collection::modifyField() - no index found!" << endl;
    return false;
  }

  // update category list.
  if(oldField->category() != newField_->category()) {
    m_fieldCategories.clear();
    for(FieldVec::Iterator it = m_fields.begin(); it != m_fields.end(); ++it) {
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
    for(EntryVecIt it = m_entries.begin(); it != m_entries.end(); ++it) {
      it->invalidateFormattedFieldValue(fieldName);
    }
    resetGroups = true;
  }

  // check to see if the people "pseudo-group" needs to be updated
  // only if only one of the two is a name
  bool wasPeople = oldField->formatFlag() == Field::FormatName;
  bool isPeople = newField_->formatFlag() == Field::FormatName;
  if(wasPeople) {
    m_peopleFields.remove(oldField);
    if(!isPeople) {
      resetGroups = true;
    }
  }
  if(isPeople) {
    // if there's more than one people field and no people dict exists yet, add it
    if(m_peopleFields.count() > 1 && m_entryGroupDicts.find(s_peopleGroupName) == 0) {
      EntryGroupDict* d = new EntryGroupDict();
      d->setAutoDelete(true);
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
      m_entryGroups.remove(fieldName);
      m_entryGroupDicts.remove(fieldName);
      myDebug() << "Collection::modifyField() - no longer grouped: " << fieldName << endl;
      resetGroups = true;
    } else {
      // don't do this, it wipes out the old groups!
//      m_entryGroupDicts.replace(fieldName, new EntryGroupDict());
    }
  } else if(isGrouped) {
    EntryGroupDict* d = new EntryGroupDict();
    d->setAutoDelete(true);
    m_entryGroupDicts.insert(fieldName, d);
    if(!wasGrouped) {
      // cache the possible groups of entries
      m_entryGroups << fieldName;
    }
    resetGroups = true;
  }

  if(oldField->type() == Field::Image) {
    m_imageFields.remove(oldField);
  }
  if(newField_->type() == Field::Image) {
    m_imageFields.append(newField_);
  }

  if(resetGroups) {
    myLog() << "Collection::modifyField() - invalidating groups" << endl;
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
bool Collection::removeField(FieldPtr field_, bool force_/*=false*/) {
  if(!field_ || !m_fields.contains(field_)) {
    if(field_) {
      myDebug() << "Collection::removeField - false: " << field_->name() << endl;
    }
    return false;
  }
//  myDebug() << "Collection::removeField() - name = " << field_->name() << endl;

  // can't delete the title field
  if((field_->flags() & Field::NoDelete) && !force_) {
    return false;
  }

  bool success = true;
  if(field_->formatFlag() == Field::FormatName) {
    success &= m_peopleFields.remove(field_);
  }

  if(field_->type() == Field::Image) {
    success &= m_imageFields.remove(field_);
  }
  success &= m_fieldNameDict.remove(field_->name());
  success &= m_fieldTitleDict.remove(field_->title());
  success &= m_fieldNames.remove(field_->name());
  success &= m_fieldTitles.remove(field_->title());

  if(fieldsByCategory(field_->category()).count() == 1) {
    success &= m_fieldCategories.remove(field_->category());
  }

  for(EntryVecIt it = m_entries.begin(); it != m_entries.end(); ++it) {
    // setting the fields to an empty string removes the value from the entry's list
    it->setField(field_, QString::null);
  }

  if(field_->flags() & Field::AllowGrouped) {
    success &= m_entryGroupDicts.remove(field_->name());
    success &= m_entryGroups.remove(field_->name());
    if(field_->name() == m_defaultGroupField) {
      setDefaultGroupField(m_entryGroups[0]);
    }
  }

  success &= m_fields.remove(field_);

  // refresh all dependent fields, rather lazy, but there's
  // likely to be weird effects when checking dependent fields
  // while removing one, so refresh all of them
  for(FieldVec::Iterator it = m_fields.begin(); it != m_fields.end(); ++it) {
    if(it->type() == Field::Dependent) {
      emit signalRefreshField(it);
    }
  }

  return success;
}

void Collection::reorderFields(const FieldVec& list_) {
// assume the lists have the same pointers!
  m_fields = list_;

  // also reset category list, since the order may have changed
  m_fieldCategories.clear();
  for(FieldVec::Iterator it = m_fields.begin(); it != m_fields.end(); ++it) {
    if(!it->category().isEmpty() && !m_fieldCategories.contains(it->category())) {
      m_fieldCategories << it->category();
    }
  }
}

void Collection::addEntries(EntryVec entries_) {
  if(entries_.isEmpty()) {
    return;
  }

  for(EntryVec::Iterator entry = entries_.begin(); entry != entries_.end(); ++entry) {
    bool foster = false;
    if(this != entry->collection()) {
      entry->setCollection(this);
      foster = true;
    }

    m_entries.append(entry);
//  myDebug() << "Collection::addEntries() - added entry (" << entry->title() << ")" << endl;

    if(entry->id() >= m_nextEntryId) {
      m_nextEntryId = entry->id() + 1;
    } else if(entry->id() == -1) {
      entry->setId(m_nextEntryId);
      ++m_nextEntryId;
    } else if(m_entryIdDict.find(entry->id())) {
      if(!foster) {
        myDebug() << "Collection::addEntries() - the collection already has an entry with id = " << entry->id() << endl;
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

void Collection::removeEntriesFromDicts(EntryVec entries_) {
  PtrVector<EntryGroup> modifiedGroups;
  for(EntryVecIt entry = entries_.begin(); entry != entries_.end(); ++entry) {
    // need a copy of the vector since it gets changed
    PtrVector<EntryGroup> groups = entry->groups();
    for(PtrVector<EntryGroup>::Iterator group = groups.begin(); group != groups.end(); ++group) {
      if(entry->removeFromGroup(group.ptr()) && !modifiedGroups.contains(group.ptr())) {
        modifiedGroups.push_back(group.ptr());
      }
      if(group->isEmpty() && !m_groupsToDelete.contains(group.ptr())) {
        m_groupsToDelete.push_back(group.ptr());
      }
    }
  }
  emit signalGroupsModified(this, modifiedGroups);
}

// this function gets called whenever an entry is modified. Its purpose is to keep the
// groupDicts current. It first removes the entry from every group to which it belongs,
// then it repopulates the dicts with the entry's fields
void Collection::updateDicts(EntryVec entries_) {
  if(entries_.isEmpty()) {
    return;
  }

  removeEntriesFromDicts(entries_);
  populateCurrentDicts(entries_);
  cleanGroups();
}

bool Collection::removeEntries(EntryVec vec_) {
  if(vec_.isEmpty()) {
    return false;
  }

//  myDebug() << "Collection::deleteEntry() - deleted entry - " << entry_->title() << endl;
  removeEntriesFromDicts(vec_);
  bool success = true;
  for(EntryVecIt entry = vec_.begin(); entry != vec_.end(); ++entry) {
    m_entryIdDict.remove(entry->id());
    success &= m_entries.remove(entry);
  }
  cleanGroups();
  return success;
}

Tellico::Data::FieldVec Collection::fieldsByCategory(const QString& cat_) {
#ifndef NDEBUG
  if(m_fieldCategories.findIndex(cat_) == -1) {
    myDebug() << "Collection::fieldsByCategory() - '" << cat_ << "' is not in category list" << endl;
  }
#endif
  if(cat_.isEmpty()) {
    myDebug() << "Collection::fieldsByCategory() - empty category!" << endl;
    return FieldVec();
  }

  FieldVec list;
  for(FieldVec::Iterator it = m_fields.begin(); it != m_fields.end(); ++it) {
    if(it->category() == cat_) {
      list.append(it);
    }
  }
  return list;
}

const QString& Collection::fieldNameByTitle(const QString& title_) const {
  if(title_.isEmpty()) {
    return QString::null;
  }
  FieldPtr f = fieldByTitle(title_);
  if(!f) { // might happen in MainWindow::saveCollectionOptions
    return QString::null;
  }
  return f->name();
}

const QString& Collection::fieldTitleByName(const QString& name_) const {
  if(name_.isEmpty()) {
    return QString::null;
  }
  FieldPtr f = fieldByName(name_);
  if(!f) {
    kdWarning() << "Collection::fieldTitleByName() - no field named " << name_ << endl;
    return QString::null;
  }
  return f->title();
}

QStringList Collection::valuesByFieldName(const QString& name_) const {
  if(name_.isEmpty()) {
    return QStringList();
  }
  bool multiple = (fieldByName(name_)->flags() & Field::AllowMultiple);

  StringSet values;
  for(EntryVec::ConstIterator it = m_entries.begin(); it != m_entries.end(); ++it) {
    if(multiple) {
      values.add(it->fields(name_, false));
    } else {
      values.add(it->field(name_));
    }
  } // end entry loop

  return values.toList();
}

Tellico::Data::FieldPtr Collection::fieldByName(const QString& name_) const {
  return m_fieldNameDict.isEmpty() ? 0 : name_.isEmpty() ? 0 : m_fieldNameDict.find(name_);
}

Tellico::Data::FieldPtr Collection::fieldByTitle(const QString& title_) const {
  return m_fieldTitleDict.isEmpty() ? 0 : title_.isEmpty() ? 0 : m_fieldTitleDict.find(title_);
}

bool Collection::hasField(const QString& name_) const {
  return fieldByName(name_) != 0;
}

bool Collection::isAllowed(const QString& key_, const QString& value_) const {
  // empty string is always allowed
  if(value_.isEmpty()) {
    return true;
  }

  // find the field with a name of 'key_'
  FieldPtr field = fieldByName(key_);

  // if the type is not multiple choice or if value_ is allowed, return true
  if(field && (field->type() != Field::Choice || field->allowed().findIndex(value_) > -1)) {
    return true;
  }

  return false;
}

Tellico::Data::EntryGroupDict* Collection::entryGroupDictByName(const QString& name_) {
//  myDebug() << "Collection::entryGroupDictByName() - " << name_ << endl;
  if(name_.isEmpty()) {
    return 0;
  }
  EntryGroupDict* dict = m_entryGroupDicts.isEmpty() ? 0 : m_entryGroupDicts.find(name_);
  if(dict && dict->isEmpty()) {
    GUI::CursorSaver cs;
    const bool b = signalsBlocked();
    // block signals so all the group created/modified signals don't fire
    blockSignals(true);
    populateDict(dict, name_, m_entries);
    blockSignals(b);
  }
  return dict;
}

void Collection::populateDict(EntryGroupDict* dict_, const QString& fieldName_, EntryVec entries_) {
//  myDebug() << "Collection::populateDict() - " << fieldName_ << endl;
  bool isBool = hasField(fieldName_) && fieldByName(fieldName_)->type() == Field::Bool;

  PtrVector<EntryGroup> modifiedGroups;
  for(EntryVecIt entry = entries_.begin(); entry != entries_.end(); ++entry) {
    QStringList groups = entryGroupNamesByField(entry, fieldName_);
    for(QStringList::ConstIterator groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {
      // find the group for this group name
      // bool fields used the field title
      QString groupTitle = *groupIt;
      if(isBool && groupTitle != i18n(s_emptyGroupTitle)) {
        groupTitle = fieldTitleByName(fieldName_);
      }
      EntryGroup* group = dict_->find(groupTitle);
      // if the group doesn't exist, create it
      if(!group) {
        group = new EntryGroup(groupTitle, fieldName_);
        dict_->insert(groupTitle, group);
      } else if(group->isEmpty()) {
        // if it's empty, then it was added to the vector of groups to delete
        // remove it from that vector now that we're adding to it
        m_groupsToDelete.remove(group);
      }
      if(entry->addToGroup(group)) {
        modifiedGroups.push_back(group);
      }
    } // end group loop
  } // end entry loop
  emit signalGroupsModified(this, modifiedGroups);
}

void Collection::populateCurrentDicts(EntryVec entries_) {
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
  QDictIterator<EntryGroupDict> dictIt(m_entryGroupDicts);
  for( ; dictIt.current(); ++dictIt) {
    // only populate if it's not empty, since they are
    // populated on demand
    if(!dictIt.current()->isEmpty()) {
      populateDict(dictIt.current(), dictIt.currentKey(), entries_);
      allEmpty = false;
    }
  }

  if(allEmpty) {
    // need to populate the current group dict
    const QString group = Controller::self()->groupBy();
    EntryGroupDict* dict = m_entryGroupDicts[group];
    if(dict) {
      populateDict(dict, group, entries_);
    }
  }
}

// return a string list for all the groups that the entry belongs to
// for a given field. Normally, this would just be splitting the entry's value
// for the field, but if the field name is the people pseudo-group, then it gets
// a bit more complicated
QStringList Collection::entryGroupNamesByField(EntryPtr entry_, const QString& fieldName_) {
  if(fieldName_ != s_peopleGroupName) {
    return entry_->groupNamesByFieldName(fieldName_);
  }

  StringSet values;
  for(FieldVec::Iterator it = m_peopleFields.begin(); it != m_peopleFields.end(); ++it) {
    values.add(entry_->groupNamesByFieldName(it->name()));
  }
  values.remove(i18n(s_emptyGroupTitle));
  return values.toList();
}

void Collection::invalidateGroups() {
  QDictIterator<EntryGroupDict> dictIt(m_entryGroupDicts);
  for( ; dictIt.current(); ++dictIt) {
    dictIt.current()->clear();
  }

  // populateDicts() will make signals that the group view is connected to, block those
  blockSignals(true);
  for(EntryVecIt it = m_entries.begin(); it != m_entries.end(); ++it) {
    it->invalidateFormattedFieldValue();
    it->clearGroups();
  }
  blockSignals(false);
}

Tellico::Data::EntryPtr Collection::entryById(long id_) {
  return m_entryIdDict[id_];
}

void Collection::addBorrower(Data::BorrowerPtr borrower_) {
  if(!borrower_) {
    return;
  }
  m_borrowers.append(borrower_);
}

void Collection::addFilter(FilterPtr filter_) {
  if(!filter_) {
    return;
  }

  m_filters.append(filter_);
}

bool Collection::removeFilter(FilterPtr filter_) {
  if(!filter_) {
    return false;
  }

  // TODO: test for success
  m_filters.remove(filter_);
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
  m_entryGroupDicts.clear();
  m_groupsToDelete.clear();
  m_filters.clear();
  m_borrowers.clear();
}

void Collection::cleanGroups() {
  for(PtrVector<EntryGroup>::Iterator it = m_groupsToDelete.begin(); it != m_groupsToDelete.end(); ++it) {
    EntryGroupDict* dict = entryGroupDictByName(it->fieldName());
    if(!dict) {
      continue;
    }
    dict->remove(it->groupName());
  }
  m_groupsToDelete.clear();
}

bool Collection::dependentFieldHasRecursion(FieldPtr field_) {
  if(!field_ || field_->type() != Field::Dependent) {
    return false;
  }

  StringSet fieldNamesFound;
  fieldNamesFound.add(field_->name());
  QValueStack<FieldPtr> fieldsToCheck;
  fieldsToCheck.push(field_);
  while(!fieldsToCheck.isEmpty()) {
    FieldPtr f = fieldsToCheck.pop();
    const QStringList depFields = f->dependsOn();
    for(QStringList::ConstIterator it = depFields.begin(); it != depFields.end(); ++it) {
      if(fieldNamesFound.has(*it)) {
        // we have recursion
        return true;
      }
      fieldNamesFound.add(*it);
      FieldPtr f = fieldByName(*it);
      if(!f) {
        f = fieldByTitle(*it);
      }
      if(f) {
        fieldsToCheck.push(f);
      }
    }
  }
  return false;
}

int Collection::sameEntry(Data::EntryPtr entry1_, Data::EntryPtr entry2_) const {
  if(!entry1_ || !entry2_) {
    return 0;
  }
  // used to just return 0, but we really want a default generic implementation
  // that specific collections can override.

  // start with twice the title score
  // and since the minimum is > 10, then need more than just a perfect title match
  int res = 2*Entry::compareValues(entry1_, entry2_, QString::fromLatin1("title"), this);
  // then add score for each field
  FieldVec fields = entry1_->collection()->fields();
  for(Data::FieldVecIt it = fields.begin(); it != fields.end(); ++it) {
    res += Entry::compareValues(entry1_, entry2_, it->name(), this);
  }
  return res;
}

// static
// merges values from e2 into e1
bool Collection::mergeEntry(EntryPtr e1, EntryPtr e2, bool overwrite_, bool askUser_) {
  if(!e1 || !e2) {
    myDebug() << "Collection::mergeEntry() - bad entry pointer" << endl;
    return false;
  }
  bool ret = true;
  FieldVec fields = e1->collection()->fields();
  for(FieldVec::Iterator field = fields.begin(); field != fields.end(); ++field) {
    if(e2->field(field).isEmpty()) {
      continue;
    }
//    myLog() << "Collection::mergeEntry() - reading field: " << field->name() << endl;
    if(overwrite_ || e1->field(field).isEmpty()) {
//      myLog() << e1->title() << ": updating field(" << field->name() << ") to " << e2->field(field->name()) << endl;
      e1->setField(field, e2->field(field));
      ret = true;
    } else if(e1->field(field) == e2->field(field)) {
      continue;
    } else if(field->type() == Field::Para) {
      // for paragraph fields, concatenate the values, if they're not equal
      e1->setField(field, e1->field(field) + QString::fromLatin1("<br/><br/>") + e2->field(field));
      ret = true;
    } else if(field->type() == Field::Table) {
      // if field F is a table-type field (album tracks, files, etc.), merge rows (keep their position)
      // if e1's F val in [row i, column j] empty, replace with e2's val at same position
      // if different (non-empty) vals at same position, CONFLICT!
      const QString sep = QString::fromLatin1("::");
      QStringList vals1 = e1->fields(field, false);
      QStringList vals2 = e2->fields(field, false);
      while(vals1.count() < vals2.count()) {
        vals1 += QString();
      }
      for(uint i = 0; i < vals2.count(); ++i) {
        if(vals2[i].isEmpty()) {
          continue;
        }
        if(vals1[i].isEmpty()) {
          vals1[i] = vals2[i];
          ret = true;
        } else {
          QStringList parts1 = QStringList::split(sep, vals1[i], true);
          QStringList parts2 = QStringList::split(sep, vals2[i], true);
          bool changedPart = false;
          while(parts1.count() < parts2.count()) {
            parts1 += QString();
          }
          for(uint j = 0; j < parts2.count(); ++j) {
            if(parts2[j].isEmpty()) {
              continue;
            }
            if(parts1[j].isEmpty()) {
              parts1[j] = parts2[j];
              changedPart = true;
            } else if(askUser_ && parts1[j] != parts2[j]) {
              int ret = Kernel::self()->askAndMerge(e1, e2, field, parts1[j], parts2[j]);
              if(ret == 0) {
                return false; // we got cancelled
              }
              if(ret == 1) {
                parts1[j] = parts2[j];
                changedPart = true;
              }
            }
          }
          if(changedPart) {
            vals1[i] = parts1.join(sep);
            ret = true;
          }
        }
      }
      if(ret) {
        e1->setField(field, vals1.join(QString::fromLatin1("; ")));
      }
// remove the merging due to use comments
// maybe in the future have a more intelligent way
#if 0
    } else if(field->flags() & Data::Field::AllowMultiple) {
      // if field F allows multiple values and not a Table (see above case),
      // e1's F values = (e1's F values) U (e2's F values) (union)
      // replace e1's field with union of e1's and e2's values for this field
      QStringList items1 = e1->fields(field, false);
      QStringList items2 = e2->fields(field, false);
      for(QStringList::ConstIterator it = items2.begin(); it != items2.end(); ++it) {
        // possible to have one value formatted and the other one not...
        if(!items1.contains(*it) && !items1.contains(Field::format(*it, field->formatFlag()))) {
          items1.append(*it);
        }
      }
// not sure if I think it should be sorted or not
//      items1.sort();
      e1->setField(field, items1.join(QString::fromLatin1("; ")));
      ret = true;
#endif
    } else if(askUser_ && e1->field(field) != e2->field(field)) {
      int ret = Kernel::self()->askAndMerge(e1, e2, field);
      if(ret == 0) {
        return false; // we got cancelled
      }
      if(ret == 1) {
        e1->setField(field, e2->field(field));
      }
    }
  }
  return ret;
}

long Collection::getID() {
  static long id = 0;
  return ++id;
}

#include "collection.moc"
