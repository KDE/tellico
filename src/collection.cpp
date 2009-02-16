/***************************************************************************
    copyright            : (C) 2001-2008 by Robby Stephenson
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
#include "tellico_utils.h"
#include "controller.h"
#include "collectionfactory.h"
#include "stringset.h"
#include "entrycomparison.h"
#include "tellico_kernel.h"

#include <klocale.h>

#include <QRegExp>
#include <QStack>

using Tellico::Data::Collection;

const char* Collection::s_emptyGroupTitle = I18N_NOOP("(Empty)");
const QString Collection::s_peopleGroupName = QLatin1String("_people");

Collection::Collection(const QString& title_)
    : QObject(), KShared(), m_nextEntryId(0), m_title(title_), m_entryIdDict()
    , m_trackGroups(false) {
  m_id = getID();
}

Collection::~Collection() {
  // maybe we should just call clear() ?
  foreach(EntryGroupDict* dict, m_entryGroupDicts) {
    qDeleteAll(*dict);
  }
  qDeleteAll(m_entryGroupDicts);
  m_entryGroupDicts.clear();
}

QString Collection::typeName() const {
  return CollectionFactory::typeName(type());
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

  if(!field_->category().isEmpty() && m_fieldCategories.indexOf(field_->category()) == -1) {
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
  foreach(FieldPtr it, m_fields) {
    if(it->type() == Field::Dependent) {
      emit signalRefreshField(it);
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
    Controller::self()->addedField(CollPtr(this), f);
    return success;
  }

  if(newField_->type() == Field::Table2) {
    newField_->setType(Data::Field::Table);
    newField_->setProperty(QLatin1String("columns"), QChar('2'));
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
        kWarning() << "Collection::mergeField() - relative URL property does not match for " << currField->name();
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
//  myDebug() << "Collection::modifyField() - " << newField_->name() << endl;

// the field name never changes
  const QString fieldName = newField_->name();
  FieldPtr oldField = fieldByName(fieldName);
  if(!oldField) {
    myDebug() << "Collection::modifyField() - no field named " << fieldName << endl;
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
    myDebug() << "Collection::modifyField() - no index found!" << endl;
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
      myDebug() << "Collection::modifyField() - no longer grouped: " << fieldName << endl;
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
bool Collection::removeField(Tellico::Data::FieldPtr field_, bool force_/*=false*/) {
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
    entry->setField(field_, QString::null);
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
//  myDebug() << "Collection::addEntries() - added entry (" << entry->title() << ")" << endl;

    if(entry->id() >= m_nextEntryId) {
      m_nextEntryId = entry->id() + 1;
    } else if(entry->id() == -1) {
      entry->setId(m_nextEntryId);
      ++m_nextEntryId;
    } else if(m_entryIdDict[entry->id()]) {
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
  if(entries_.isEmpty()) {
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

//  myDebug() << "Collection::deleteEntry() - deleted entry - " << entry_->title() << endl;
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
  if(m_fieldCategories.indexOf(cat_) == -1) {
    myDebug() << "Collection::fieldsByCategory() - '" << cat_ << "' is not in category list" << endl;
  }
#endif
  if(cat_.isEmpty()) {
    myDebug() << "Collection::fieldsByCategory() - empty category!" << endl;
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
    return QString::null;
  }
  FieldPtr f = fieldByTitle(title_);
  if(!f) { // might happen in MainWindow::saveCollectionOptions
    return QString::null;
  }
  return f->name();
}

QString Collection::fieldTitleByName(const QString& name_) const {
  if(name_.isEmpty()) {
    return QString::null;
  }
  FieldPtr f = fieldByName(name_);
  if(!f) {
    kWarning() << "Collection::fieldTitleByName() - no field named " << name_;
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
  if(field && (field->type() != Field::Choice || field->allowed().indexOf(value_) > -1)) {
    return true;
  }

  return false;
}

Tellico::Data::EntryGroupDict* Collection::entryGroupDictByName(const QString& name_) {
//  myDebug() << "Collection::entryGroupDictByName() - " << name_ << endl;
  if(name_.isEmpty() || !m_entryGroupDicts.contains(name_)) {
    return 0;
  }
  EntryGroupDict* dict = m_entryGroupDicts[name_];
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

void Collection::populateDict(Tellico::Data::EntryGroupDict* dict_, const QString& fieldName_, const Tellico::Data::EntryList& entries_) {
//  myDebug() << "Collection::populateDict() - " << fieldName_ << endl;
  bool isBool = hasField(fieldName_) && fieldByName(fieldName_)->type() == Field::Bool;

  QList<EntryGroup*> modifiedGroups;
  foreach(EntryPtr entry, entries_) {
    QStringList groups = entryGroupNamesByField(entry, fieldName_);
    foreach(QString groupTitle, groups) {
      // find the group for this group name
      // bool fields used the field title
      if(isBool && groupTitle != i18n(s_emptyGroupTitle)) {
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
QStringList Collection::entryGroupNamesByField(Tellico::Data::EntryPtr entry_, const QString& fieldName_) {
  if(fieldName_ != s_peopleGroupName) {
    return entry_->groupNamesByFieldName(fieldName_);
  }

  StringSet values;
  foreach(FieldPtr field, m_peopleFields) {
    values.add(entry_->groupNamesByFieldName(field->name()));
  }
  values.remove(i18n(s_emptyGroupTitle));
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

Tellico::Data::EntryPtr Collection::entryById(long id_) {
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

// static
// merges values from e2 into e1
bool Collection::mergeEntry(Tellico::Data::EntryPtr e1, Tellico::Data::EntryPtr e2, bool overwrite_, bool askUser_) {
  if(!e1 || !e2) {
    myDebug() << "Collection::mergeEntry() - bad entry pointer" << endl;
    return false;
  }
  bool ret = true;
  FieldList fields = e1->collection()->fields();
  foreach(FieldPtr field, fields) {
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
      e1->setField(field, e1->field(field) + QLatin1String("<br/><br/>") + e2->field(field));
      ret = true;
    } else if(field->type() == Field::Table) {
      // if field F is a table-type field (album tracks, files, etc.), merge rows (keep their position)
      // if e1's F val in [row i, column j] empty, replace with e2's val at same position
      // if different (non-empty) vals at same position, CONFLICT!
      const QString sep = QLatin1String("::");
      QStringList vals1 = e1->fields(field, false);
      QStringList vals2 = e2->fields(field, false);
      while(vals1.count() < vals2.count()) {
        vals1 += QString();
      }
      for(int i = 0; i < vals2.count(); ++i) {
        if(vals2[i].isEmpty()) {
          continue;
        }
        if(vals1[i].isEmpty()) {
          vals1[i] = vals2[i];
          ret = true;
        } else {
          QStringList parts1 = vals1[i].split(sep);
          QStringList parts2 = vals2[i].split(sep);
          bool changedPart = false;
          while(parts1.count() < parts2.count()) {
            parts1 += QString();
          }
          for(int j = 0; j < parts2.count(); ++j) {
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
        e1->setField(field, vals1.join(QLatin1String("; ")));
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
      foreach(const QString& item2, items2) {
        // possible to have one value formatted and the other one not...
        if(!items1.contains(item2) && !items1.contains(Field::format(item2, field->formatFlag()))) {
          items1.append(item2);
        }
      }
// not sure if I think it should be sorted or not
//      items1.sort();
      e1->setField(field, items1.join(QLatin1String("; ")));
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
