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

#include <klocale.h>
#include <kglobal.h> // for KMAX

#include <qregexp.h>

using Tellico::Data::Collection;

const QString Collection::s_emptyGroupTitle = i18n("(Empty)");
const QString Collection::s_peopleGroupName = QString::fromLatin1("_people");

Collection::Collection(const QString& title_, const QString& entryTitle_)
    : QObject(), KShared(), m_nextEntryId(0), m_title(title_), m_entryTitle(entryTitle_), m_entryIdDict(997) {
  m_entryGroupDicts.setAutoDelete(true);

  m_id = getID();
//  m_iconName = entryName_ + 's';
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

//  kdDebug() << "Collection::addField() - adding " << field_->name() << endl;
  m_fields.append(field_);
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

  if(!field_->category().isEmpty() && m_fieldCategories.findIndex(field_->category()) == -1) {
    m_fieldCategories << field_->category();
  }

  if(field_->flags() & Field::AllowGrouped) {
    // m_entryGroups autoDeletes each QDict when the Collection d'tor is called
    EntryGroupDict* dict = new EntryGroupDict();
    // don't autoDelete, since the group is deleted when it becomes empty
    m_entryGroupDicts.insert(field_->name(), dict);
    // cache the possible groups of entries
    m_entryGroups << field_->name();
  }

  for(EntryVecIt it = m_entries.begin(); it != m_entries.end(); ++it) {
    populateDicts(it);
  }

  if(m_defaultGroupField.isEmpty() && field_->flags() & Field::AllowGrouped) {
    m_defaultGroupField = field_->name();
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
    Data::FieldPtr f = newField_->clone();
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
          myDebug() << "Collection::mergeField() - changing " << propName << " for " << currField->name() << endl;
          currField->setProperty(propName, QString::number(newNum));
        }
      } else if(currField->type() == Field::Rating && propName == Latin1Literal("minimum")) {
        bool ok;
        uint currNum = Tellico::toUInt(currValue, &ok);
        uint newNum = Tellico::toUInt(it.data(), &ok);
        if(newNum < currNum) { // smaller values
          myDebug() << "Collection::mergeField() - changing " << propName << " for " << currField->name() << endl;
          currField->setProperty(propName, QString::number(newNum));
        }
      }
    }
  }

  // combine flags
  currField->setFlags(currField->flags() | newField_->flags());
  return true;
}

// be really careful with these field pointers, try not to call to many other functions
// which may depend on the field list
bool Collection::modifyField(FieldPtr newField_) {
  if(!newField_) {
    return false;
  }
//  kdDebug() << "Collection::modifyField() - " << newField_->name() << endl;

// the field name never changes
  const QString& fieldName = newField_->name();
  FieldPtr oldField = fieldByName(fieldName);
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
  FieldVec::Iterator it = m_fields.find(oldField);
  if(it != m_fields.end()) {
    m_fields.insert(it, newField_);
    m_fields.remove(oldField);
  } else {
    kdDebug() << "Collection::modifyField() - no index found!" << endl;
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
    if(m_peopleFields.count() > 1 && !m_entryGroupDicts.find(s_peopleGroupName)) {
      m_entryGroupDicts.insert(s_peopleGroupName, new EntryGroupDict());
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
      resetGroups = true;
    } else {
      // don't do this, it wipes out the old groups!
//      m_entryGroupDicts.replace(fieldName, new EntryGroupDict());
    }
  } else if(isGrouped) {
    m_entryGroupDicts.insert(fieldName, new EntryGroupDict());
    // cache the possible groups of entries
    m_entryGroups << fieldName;
    resetGroups = true;
  }

  if(oldField->type() == Field::Image) {
    m_imageFields.remove(oldField);
  }
  if(newField_->type() == Field::Image) {
    m_imageFields.append(newField_);
  }

  if(resetGroups) {
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
    return false;
  }
//  kdDebug() << "Collection::deleteField() - name = " << field_->name() << endl;

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

void Collection::addEntry(EntryPtr entry_) {
  if(!entry_) {
    return;
  }

  if(this != entry_->collection()) {
    entry_->setCollection(this);
  }

  m_entries.append(entry_);
//  kdDebug() << "Collection::addEntry() - added entry (" << entry_->title() << ")" << endl;

  if(entry_->id() >= m_nextEntryId) {
    m_nextEntryId = entry_->id() + 1;
  } else {
    entry_->setId(m_nextEntryId);
    ++m_nextEntryId;
  }
  m_entryIdDict.insert(entry_->id(), entry_);
  populateDicts(entry_);
}

void Collection::removeEntriesFromDicts(EntryVec entries_) {
  PtrVector<EntryGroup> modifiedGroups, deleteGroups;
  for(EntryVecIt entry = entries_.begin(); entry != entries_.end(); ++entry) {
    // need a copy of the vector since it gets changed
    PtrVector<EntryGroup> groups = entry->groups();
    for(PtrVector<EntryGroup>::Iterator group = groups.begin(); group != groups.end(); ++group) {
      if(entry->removeFromGroup(group.ptr()) && !modifiedGroups.contains(group.ptr())) {
        modifiedGroups.push_back(group.ptr());
      }
      if(group->isEmpty()) {
        EntryGroupDict* dict = m_entryGroupDicts.find(group->fieldName());
        if(!dict) {
          continue;
        }
        dict->remove(group->groupName());
        if(!deleteGroups.contains(group.ptr())) {
          deleteGroups.push_back(group.ptr());
        }
      }
    }
  }
  for(PtrVector<EntryGroup>::Iterator group = modifiedGroups.begin(); group != modifiedGroups.end(); ++group) {
    emit signalGroupModified(this, group.ptr());
  }
  for(PtrVector<EntryGroup>::Iterator group = deleteGroups.begin(); group != deleteGroups.end(); ++group) {
    delete group.ptr();
  }
}

// this function gets called whenever an entry is modified. Its purpose is to keep the
// groupDicts current. It first removes the entry from every group to which it belongs,
// then it repopulates the dicts with the entry's fields
void Collection::updateDicts(EntryVec entries_) {
//  myDebug() << "Collection::updateDicts()" << endl;
  if(entries_.isEmpty()) {
    return;
  }

  removeEntriesFromDicts(entries_);
  for(EntryVecIt entry = entries_.begin(); entry != entries_.end(); ++entry) {
    populateDicts(entry);
  }
}

bool Collection::removeEntry(EntryPtr entry_) {
  if(!entry_) {
    return false;
  }

//  kdDebug() << "Collection::deleteEntry() - deleted entry - " << entry_->title() << endl;
  EntryVec vec;
  vec.append(entry_);
  removeEntriesFromDicts(vec);
  bool success = m_entryIdDict.remove(entry_->id());

  success &= m_entries.remove(entry_);

  return success;
}

Tellico::Data::FieldVec Collection::fieldsByCategory(const QString& cat_) {
#ifndef NDEBUG
  if(m_fieldCategories.findIndex(cat_) == -1) {
    kdDebug() << "Collection::fieldsByCategory() - '" << cat_ << "' is not in category list" << endl;
  }
#endif
  if(cat_.isEmpty()) {
    kdDebug() << "Collection::fieldsByCategory() - empty category!" << endl;
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

  QStringList strlist;
  for(EntryVec::ConstIterator it = m_entries.begin(); it != m_entries.end(); ++it) {
    if(multiple) {
      strlist += it->fields(name_, false);
    } else {
      strlist += it->field(name_);
    }
  } // end entry loop
  strlist.sort();

  QStringList::Iterator it = strlist.begin();
  while(it != strlist.end()) {
    const QString& s = *it;
    ++it;
    while(it != strlist.end() && s == *it) {
      it = strlist.remove(it);
    }
  }
  return strlist;
}

Tellico::Data::FieldPtr Collection::fieldByName(const QString& name_) const {
  return m_fieldNameDict.isEmpty() ? 0 : m_fieldNameDict.find(name_);
}

Tellico::Data::FieldPtr  Collection::fieldByTitle(const QString& title_) const {
  return m_fieldTitleDict.isEmpty() ? 0 : m_fieldTitleDict.find(title_);
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

Tellico::Data::EntryGroupDict* const Collection::entryGroupDictByName(const QString& name_) const {
  return m_entryGroupDicts.isEmpty() ? 0 : m_entryGroupDicts.find(name_);
}

void Collection::populateDicts(EntryPtr entry_) {
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
    // the field name might be the people group name
    QString fieldName = dictIt.currentKey();
    bool isBool = hasField(fieldName) && fieldByName(fieldName)->type() == Field::Bool;

    QStringList groups = entryGroupNamesByField(entry_, fieldName);
    for(QStringList::ConstIterator groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {
      // find the group for this group name
      // bool fields used the field title
      EntryGroup* group;
      QString groupTitle = *groupIt;
      if(isBool && groupTitle != s_emptyGroupTitle) {
        groupTitle = fieldTitleByName(fieldName);
      }
      group = dict->find(groupTitle);
      // if the group doesn't exist, create it
      if(!group) {
        group = new EntryGroup(groupTitle, fieldName);
        dict->insert(groupTitle, group);
      }
      if(entry_->addToGroup(group)) {
        emit signalGroupModified(this, group);
      }
    } // end group loop
//    kdDebug() << "Collection::populateDicts - end of group loop" << endl;
  } // end dict loop
//  kdDebug() << "Collection::populateDicts - end of full loop" << endl;
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
  values.remove(s_emptyGroupTitle);
  return values.toList();
}

void Collection::invalidateGroups() {
  QDictIterator<EntryGroupDict> dictIt(m_entryGroupDicts);
  for( ; dictIt.current(); ++dictIt) {
    dictIt.current()->clear();
  }

  for(EntryVecIt it = m_entries.begin(); it != m_entries.end(); ++it) {
    it->invalidateFormattedFieldValue();
    // populateDicts() will make signals that the group view is connected to, block those
    blockSignals(true);
    populateDicts(it);
    blockSignals(false);
  }
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

// static
bool Collection::mergeEntry(EntryPtr e1, EntryPtr e2) {
  if(!e1 || !e2) {
    return false;
  }
  bool ret1 = false;
  FieldVec fields = e1->collection()->fields();
  for(FieldVec::Iterator field = fields.begin(); field != fields.end(); ++field) {
    if(e1->field(field).isEmpty() && !e2->field(field).isEmpty()) {
//      myDebug() << e1->title() << ": updating field(" << field->name() << ") to " << e2->field(field->name()) << endl;
      e1->setField(field, e2->field(field));
      ret1 = true;
    }
  }
  // special case for tracks in albums
  bool ret2 = false;
  const QString sep = QString::fromLatin1("::");
  if(e1->collection()->type() == Collection::Album) {
    const QString track = QString::fromLatin1("track");
    QStringList tracks1 = e1->fields(track, false);
    QStringList tracks2 = e2->fields(track, false);
    while(tracks1.count() < tracks2.count()) {
      tracks1 << QString();
    }
    for(uint i = 0; i < tracks2.count(); ++i) {
      if(tracks2[i].isEmpty()) {
        continue;
      }
      if(tracks1[i].isEmpty()) {
        tracks1[i] = tracks2[i];
        ret2 = true;
      } else {
        QStringList parts1 = QStringList::split(sep, tracks1[i], true);
        QStringList parts2 = QStringList::split(sep, tracks2[i], true);
        bool addedPart = false;
        while(parts1.count() < parts2.count()) {
          parts1 += QString();
        }
        for(uint i = 1; i < parts2.count(); ++i) {
          if(parts1[i].isEmpty()) {
            parts1[i] = parts2[i];
            addedPart = true;
          }
        }
        if(addedPart) {
          tracks1[i] = parts1.join(sep);
          ret2 = true;
        }
      }
    }
    if(ret2) {
      e1->setField(track, tracks1.join(QString::fromLatin1("; ")));
    }
  }
  return ret1 || ret2;
}

long Collection::getID() {
  static long id = 0;
  return ++id;
}

#include "collection.moc"
