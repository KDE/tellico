/***************************************************************************
    copyright            : (C) 2001-2004 by Robby Stephenson
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

#include <klocale.h>
#include <kdebug.h>

#include <qregexp.h>

using Bookcase::Data::Collection;

const QString Collection::s_emptyGroupTitle = i18n("(Empty)");
const QString Collection::s_peopleGroupName = QString::fromLatin1("_people");

Collection::Collection(const QString& title_, const QString& entryName_, const QString& entryTitle_)
    : QObject(), m_title(title_), m_entryName(entryName_), m_entryTitle(entryTitle_) {
  m_entryList.setAutoDelete(true);
  m_fieldList.setAutoDelete(true);
  m_entryGroupDicts.setAutoDelete(true);

  m_id = getID();
  m_iconName = entryName_ + QString::fromLatin1("s");
}

Collection::~Collection() {
}

bool Collection::addFields(const FieldList& list_) {
  bool success = true;
  for(FieldListIterator it(list_); it.current(); ++it) {
    success &= addField(it.current());
  }
  return success;
}

bool Collection::addField(Field* field_) {
  if(!field_) {
    return false;
  }

  // fieldByName() returns 0 if there's no field by that name
  // this essentially checks for duplicates
  if(fieldByName(field_->name())) {
//    kdDebug() << "Collection::addField() - replacing " << field_->name() << endl;
    deleteField(fieldByName(field_->name()), true);
  }

//  kdDebug() << "Collection::addField() - adding " << field_->name() << endl;
  m_fieldList.append(field_);
  if(field_->formatFlag() == Field::FormatName) {
    m_peopleFields.append(field_); // list of people attributes
    if(m_peopleFields.count() > 1) {
      // the second time that a person field is added, add a "pseudo-group" for people
      if(m_entryGroupDicts.find(s_peopleGroupName) == 0) {
        m_entryGroupDicts.insert(s_peopleGroupName, new EntryGroupDict());
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

  //field_->category() will never be empty
  if(m_fieldCategories.findIndex(field_->category()) == -1) {
    m_fieldCategories << field_->category();
  }

  if(field_->flags() & Field::AllowGrouped) {
    // m_entryGroupDicts autoDeletes each QDict when the Collection d'tor is called
    EntryGroupDict* dict = new EntryGroupDict();
    // don't autoDelete, since the group is deleted when it becomes
    // empty in Entry::removeFromGroup()
    m_entryGroupDicts.insert(field_->name(), dict);
    // cache the possible groups of entries
    m_entryGroups << field_->name();
  }

  for(EntryListIterator it(m_entryList); it.current(); ++it) {
    populateDicts(it.current());
  }

  if(m_defaultGroupField.isEmpty() && field_->flags() & Field::AllowGrouped) {
    m_defaultGroupField = field_->name();
  }

  emit signalFieldAdded(this, field_);

  return true;
}

bool Collection::mergeField(Field* newField_) {
  if(!newField_) {
    return false;
  }

  Field* currField = fieldByName(newField_->name());
  if(!currField) {
    // does not exist in current collection, add it
    return addField(newField_->clone());
  }

  // the original field type is kept
  if(currField->type() != newField_->type()) {
    kdDebug() << "Collection::mergeField() - skipping, field type mismatch for " << currField->title() << endl;
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
  }

  // if new field has additional extended properties, add those
  Data::StringMap::ConstIterator it;
  for(it = newField_->propertyList().begin(); it != newField_->propertyList().end(); ++it) {
    if(currField->property(it.key()).isEmpty()) {
      currField->setProperty(it.key(), it.data());
    }
  }

  // combine flags
  currField->setFlags(currField->flags() | newField_->flags());
  return true;
}

// be really careful with these field pointers, try not to call to many other functions
// which may depend on the field list
bool Collection::modifyField(Field* newField_) {
  if(!newField_) {
    return false;
  }
//  kdDebug() << "Collection::modifyField() - " << newField_->name() << endl;

// the field name never changes
  const QString& fieldName = newField_->name();
  Field* oldField = fieldByName(fieldName);
  if(!oldField) {
    kdDebug() << "Collection::modifyField() - no field named " << fieldName << endl;
    return false;
  }

  // update name dict
  m_fieldNameDict.replace(fieldName, newField_);

  // update titles
  const QString& oldTitle = oldField->title();
  const QString& newTitle = newField_->title();
  if(oldTitle == newTitle) {
    m_fieldTitleDict.replace(newTitle, newField_);
  } else {
    m_fieldTitleDict.remove(oldTitle);
    m_fieldTitles.remove(oldTitle);
    m_fieldTitleDict.insert(newTitle, newField_);
    m_fieldTitles.append(newTitle);
  }

  // now replace the field pointer in the list
  int idx = m_fieldList.findRef(oldField);
  if(idx > -1) {
    m_fieldList.setAutoDelete(false);
    m_fieldList.replace(idx, newField_);
    m_fieldList.setAutoDelete(true);
  } else {
    kdDebug() << "Collection::modifyField() - no index found!" << endl;
    return false;
  }

  // update category list.
  if(oldField->category() != newField_->category()) {
    m_fieldCategories.clear();
    for(FieldListIterator it(m_fieldList); it.current(); ++it) {
      if(m_fieldCategories.findIndex(it.current()->category()) == -1) {
        m_fieldCategories += it.current()->category();
      }
    }
  }

  // if format is different, go ahead and invalidate all formatted entry values
  if(oldField->formatFlag() != newField_->formatFlag()) {
    // invalidate cached format strings of all entry attributes of this name
    for(EntryListIterator it(m_entryList); it.current(); ++it) {
      it.current()->invalidateFormattedFieldValue(fieldName);
    }
  }

  // keep track of if the entries will need to be added to groups
  bool addToGroups = false;
  // keep track of if the entries will need to be removed from groups
  bool removeFromGroups = false;

  // check to see if the people "pseudo-group" needs to be updated
  // only if only one of the two is a name
  bool wasPeople = oldField->formatFlag() == Field::FormatName;
  bool isPeople = newField_->formatFlag() == Field::FormatName;
  if(wasPeople) {
    m_peopleFields.removeRef(oldField);
    if(!isPeople) {
      removeFromGroups = true;
    }
  }
  if(isPeople) {
    // if there's more than one people field and no people dict exists yet, add it
    if(m_peopleFields.count() > 1 && !m_entryGroupDicts.find(s_peopleGroupName)) {
      m_entryGroupDicts.insert(s_peopleGroupName, new EntryGroupDict());
      // put it at the top of the list
      m_entryGroups.prepend(s_peopleGroupName);
    }
    m_peopleFields.append(newField_);
    if(!wasPeople) {
      addToGroups = true;
    }
  }

  bool wasGrouped = oldField->flags() & Field::AllowGrouped;
  bool isGrouped = newField_->flags() & Field::AllowGrouped;
  if(wasGrouped) {
    if(!isGrouped) {
      // in order to keep list in the same order, don't remove unless new field is not groupable
      m_entryGroups.remove(fieldName);
      m_entryGroupDicts.remove(fieldName);
      removeFromGroups = true;
    } else {
      m_entryGroupDicts.replace(fieldName, new EntryGroupDict());
    }
  } else if(isGrouped) {
    m_entryGroupDicts.insert(fieldName, new EntryGroupDict());
    // cache the possible groups of entries
    m_entryGroups << fieldName;
    addToGroups = true;
  }

  if(oldField->type() == Field::Image) {
    m_imageFields.removeRef(oldField);
  }
  if(newField_->type() == Field::Image) {
    m_imageFields.append(newField_);
  }

  if(removeFromGroups || addToGroups) {
    invalidateGroups();
  }

  // now to update all entries if the field is a dependent and the description changed
  if(newField_->type() == Field::Dependent && oldField->description() != newField_->description()) {
    emit signalRefreshField(newField_);
  }

  emit signalFieldModified(this, newField_, oldField);
  delete oldField;

  return true;
}

// force allows me to force the deleting of the title field if I need to
bool Collection::deleteField(Field* field_, bool force_/*=false*/) {
  if(!field_ || !m_fieldList.containsRef(field_)) {
    return false;
  }
//  kdDebug() << "Collection::deleteField() - name = " << field_->name() << endl;

  // can't delete the title field
  if(field_->flags() & Field::NoDelete && !force_) {
    return false;
  }

  bool success = true;
  if(field_->formatFlag() == Field::FormatName) {
    success &= m_peopleFields.removeRef(field_);
  }
  if(field_->type() == Field::Image) {
    success &= m_imageFields.removeRef(field_);
  }
  success &= m_fieldNameDict.remove(field_->name());
  success &= m_fieldTitleDict.remove(field_->title());
  success &= m_fieldNames.remove(field_->name());
  success &= m_fieldTitles.remove(field_->title());

  if(fieldsByCategory(field_->category()).count() == 1) {
    success &= m_fieldCategories.remove(field_->category());
  }

  for(EntryListIterator it(m_entryList); it.current(); ++it) {
    // setting the fields to an empty string removes the value from the entry's list
    it.current()->setField(field_->name(), QString::null);
  }

  if(field_->flags() & Field::AllowGrouped) {
    success &= m_entryGroupDicts.remove(field_->name());
    success &= m_entryGroups.remove(field_->name());
    if(field_->name() == m_defaultGroupField) {
      setDefaultGroupField(m_entryGroups[0]);
    }
  }

  m_fieldList.setAutoDelete(false);
  success &= m_fieldList.removeRef(field_);
  m_fieldList.setAutoDelete(true);

  emit signalFieldDeleted(this, field_);
  delete field_; // don't delete until after signal
  return success;
}

void Collection::reorderFields(const FieldList& list_) {
// assume the lists have the same pointers!
  m_fieldList.setAutoDelete(false);
  m_fieldList = list_;
  m_fieldList.setAutoDelete(true);

  // also reset category list, since the order may have changed
  m_fieldCategories.clear();
  for(FieldListIterator it(m_fieldList); it.current(); ++it) {
    if(m_fieldCategories.findIndex(it.current()->category()) == -1) {
      m_fieldCategories << it.current()->category();
    }
  }

  emit signalFieldsReordered(this);
}

void Collection::addEntry(Entry* entry_) {
  if(!entry_) {
    return;
  }

#ifndef NDEBUG
  if(entry_->collection() != this) {
    // should the addEntry() call be done in the Entry constructor?
    kdWarning() << "Collection::addEntry() - entry is not being added to its parent collection" << endl;
  }
#endif

  m_entryList.append(entry_);
//  kdDebug() << "Collection::addEntry() - added entry (" << entry_->title() << ")" << endl;

  populateDicts(entry_);
}

void Collection::removeEntryFromDicts(Entry* entry_) {
  QPtrListIterator<EntryGroup> it(entry_->groups());
  while(it.current()) {
    EntryGroup* group = it.current();
    EntryGroupDict* dict = m_entryGroupDicts.find(group->fieldName());
    // removeFromGroup will delete the group if it becomes empty
    // so just see if there's only one entry in the group, because that
    // implies that it will be empty when the entry is removed
    // emit signalGroupModified(this, group);
    if(dict && group->count() == 1) {
      dict->remove(group->groupName());
    }
    // increment the iterator before calling removeFromGroup since the
    // group might be deleted
    ++it;
    entry_->removeFromGroup(group);
  }
}

// this function gets called whenever a entry is modified. Its purpose is to keep the
// groupDicts current. It first removes the entry from every group to which it belongs,
// then it repopulates the dicts with the entry's attributes
void Collection::updateDicts(Entry* entry_) {
//  kdDebug() << "Collection::updateDicts" << endl;
  if(!entry_) {
    return;
  }

  removeEntryFromDicts(entry_);
  populateDicts(entry_);
}

bool Collection::deleteEntry(Entry* entry_) {
  if(!entry_) {
    return false;
  }

//  kdDebug() << "Collection::deleteEntry() - deleted entry - " << entry_->title() << endl;
  removeEntryFromDicts(entry_);
  return m_entryList.remove(entry_);
}

Bookcase::Data::FieldList Collection::fieldsByCategory(const QString& cat_) const {
#ifndef NDEBUG
  if(m_fieldCategories.findIndex(cat_) == -1) {
    kdDebug() << "Collection::fieldsByCategory() - '" << cat_ << "' is not in category list" << endl;
  }
#endif
  if(cat_.isEmpty()) {
    kdDebug() << "Collection::fieldsByCategory() - empty category!" << endl;
    return FieldList();
  }

  FieldList list;
  for(FieldListIterator it(m_fieldList); it.current(); ++it) {
    if(it.current()->category() == cat_) {
      list.append(it.current());
    }
  }
  return list;
}

const QString& Collection::fieldNameByTitle(const QString& title_) const {
  if(title_.isEmpty()) {
    return QString::null;
  }
  Field* f = fieldByTitle(title_);
  if(!f) {
    kdWarning() << "Collection::fieldNameByTitle() - no field titled " << title_ << endl;
    return QString::null;
  }
  return f->name();
}

const QString& Collection::fieldTitleByName(const QString& name_) const {
  if(name_.isEmpty()) {
    return QString::null;
  }
  Field* f = fieldByName(name_);
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
  QStringList strlist;
  for(EntryListIterator it(m_entryList); it.current(); ++it) {
    QStringList values;
    if(multiple) {
      values = it.current()->fields(name_, false);
    } else {
      values = it.current()->field(name_);
    }
    for(QStringList::ConstIterator it = values.begin(); it != values.end(); ++it) {
      if(strlist.findIndex(*it) == -1) { // haven't inserted it yet
        // no need to call value.simplifyWhiteSpace()
        strlist += *it;
      }
    }

  } // end entry loop
  return strlist;
}

Bookcase::Data::Field* const Collection::fieldByName(const QString& name_) const {
  return m_fieldNameDict.isEmpty() ? 0 : m_fieldNameDict.find(name_);
}

Bookcase::Data::Field* const Collection::fieldByTitle(const QString& title_) const {
  return m_fieldTitleDict.isEmpty() ? 0 : m_fieldTitleDict.find(title_);
}

bool Collection::isAllowed(const QString& key_, const QString& value_) const {
  // empty string is always allowed
  if(value_.isEmpty()) {
    return true;
  }

  // find the field with a name of 'key_'
  Field* field = fieldByName(key_);

  // if the type is not multiple choice or if value_ is allowed, return true
  if(field && (field->type() != Field::Choice || field->allowed().findIndex(value_) > -1)) {
    return true;
  }

  return false;
}

Bookcase::Data::EntryGroupDict* const Collection::entryGroupDictByName(const QString& name_) const {
  return m_entryGroupDicts.isEmpty() ? 0 : m_entryGroupDicts.find(name_);
}

void Collection::populateDicts(Entry* entry_) {
//  kdDebug() << "Collection::populateDicts" << endl;
  if(m_entryGroupDicts.isEmpty()) {
    return;
  }

  // iterate over all the possible groupDicts
  // for each dict, get the value of that field for the entry
  // if multiple values are allowed, split the value and then insert the
  // entry pointer into the dict for each value
  QDictIterator<EntryGroupDict> dictIt(m_entryGroupDicts);
  for( ; dictIt.current(); ++dictIt) {
    EntryGroupDict* dict = dictIt.current();
    QString fieldName = dictIt.currentKey();

    QStringList groups = entryGroupNamesByField(entry_, fieldName);
    for(QStringList::ConstIterator groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {
      // find the group for this group name
      EntryGroup* group = dict->find(*groupIt);
      // if the group doesn't exist, create it
      if(!group) {
        // if it's a bool, rather than showing "true", use field title instead of "true"
        // as long as it's not the empty group name
        // be careful that the group might be the people group
        if(fieldName != s_peopleGroupName
           && fieldByName(fieldName)->type() == Field::Bool
           && (*groupIt) != s_emptyGroupTitle) {
          group = new EntryGroup(fieldTitleByName(fieldName), fieldName);
          dict->insert(fieldTitleByName(fieldName), group);
        } else {
          group = new EntryGroup(*groupIt, fieldName);
          dict->insert(*groupIt, group);
        }
      }
      entry_->addToGroup(group);
    } // end group loop
  } // end dict loop
}

void Collection::groupModified(EntryGroup* group_) {
  emit signalGroupModified(this, group_);
}

// return a string list for all the groups that the entry belongs to
// for a given field. Normally, this would just be splitting the entry's value
// for the field, but if the field name is the people pseudo-group, then it gets
// a bit more complicated
QStringList Collection::entryGroupNamesByField(Entry* entry_, const QString& fieldName_) {
  if(fieldName_ != s_peopleGroupName) {
    return entry_->groupNamesByFieldName(fieldName_);
  }

  QStringList groups;
  for(FieldListIterator it(m_peopleFields); it.current(); ++it) {
    groups += entry_->groupNamesByFieldName(it.current()->name());
  }

  // just want unique values
  // so if there's more than one people field, need to remove any duplicates
  if(m_peopleFields.count() > 1) {
    groups.sort();
    unsigned i = 1;
    while(i < groups.count()) {
      if(groups[i] == groups[i-1]) {
        groups.remove(groups.at(i));
      } else {
        ++i;
      }
    }
  }

  // don't want the empty group
  // effectively, this means that if the people group is used, entries with no
  // people are not shown in the group view
  groups.remove(s_emptyGroupTitle);
  return groups;
}

void Collection::invalidateGroups() {
  QDictIterator<EntryGroupDict> dictIt(m_entryGroupDicts);
  for( ; dictIt.current(); ++dictIt) {
    dictIt.current()->clear();
  }

  for(EntryListIterator it(m_entryList); it.current(); ++it) {
    it.current()->invalidateFormattedFieldValue();
    // populateDicts() will make signals that the group view is connected to, block those
    blockSignals(true);
    populateDicts(it.current());
    blockSignals(false);
  }
}

// static
int Collection::getID() {
  static int id = 0;
  return ++id;
}
