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

const QString Collection::s_emptyGroupName = i18n("(Empty)");

Collection::Collection(const QString& title_, const QString& entryName_, const QString& entryTitle_)
    : QObject(), m_title(title_), m_entryName(entryName_), m_entryTitle(entryTitle_), m_numImageFields(0) {
  m_entryList.setAutoDelete(true);
  m_fieldList.setAutoDelete(true);
  m_entryGroupDicts.setAutoDelete(true);

  m_id = getID();
  m_iconName = entryName_ + QString::fromLatin1("s");
}

Collection::~Collection() {
}

bool Collection::addFields(const Data::FieldList& list_) {
  bool success = true;
  for(Data::FieldListIterator it(list_); it.current(); ++it) {
    success &= addField(it.current());
  }
  return success;
}

bool Collection::addField(Data::Field* field_) {
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
  if(field_->formatFlag() == Data::Field::FormatName) {
    m_peopleFields.append(field_); // list of people attributes
    if(m_peopleFields.count() > 1) {
      // the first time that a person field is added, add a "pseudo-group" for people
      QString people = QString::fromLatin1("_people");
      if(m_entryGroupDicts.find(people) == 0) {
        m_entryGroupDicts.insert(people, new Data::EntryGroupDict());
        m_entryGroups.prepend(people);
      }
    }
  }
  m_fieldNameDict.insert(field_->name(), field_);
  m_fieldTitleDict.insert(field_->title(), field_);
  m_fieldNames << field_->name();
  m_fieldTitles << field_->title();
  if(field_->type() == Data::Field::Image) {
    m_imageFields.append(field_);
  }

  //field_->category() will never be empty
  if(m_fieldCategories.findIndex(field_->category()) == -1) {
    m_fieldCategories << field_->category();
  }

  if(field_->flags() & Data::Field::AllowGrouped) {
    // m_entryGroupDicts autoDeletes each QDict when the Collection d'tor is called
    Data::EntryGroupDict* dict = new Data::EntryGroupDict();
    // don't autoDelete, since the group is deleted when it becomes
    // empty in Entry::removeFromGroup()
    m_entryGroupDicts.insert(field_->name(), dict);
    // cache the possible groups of entries
    m_entryGroups << field_->name();
  }

  if(field_->type() == Field::Image) {
    ++m_numImageFields;
  }

  for(Data::EntryListIterator it(m_entryList); it.current(); ++it) {
    populateDicts(it.current());
  }

  if(m_defaultGroupField.isEmpty() && field_->flags() & Data::Field::AllowGrouped) {
    m_defaultGroupField = field_->name();
  }

  emit signalFieldAdded(this, field_);

  return true;
}

// field_ is the new variable
// find the old variable with the same name, and modify it
bool Collection::modifyField(Data::Field* newField_) {
  if(!newField_) {
    return false;
  }
//  kdDebug() << "Collection::modifyField()" << endl;
// the field name never changes
  Data::Field* oldField = fieldByName(newField_->name());
  if(!oldField) {
    kdDebug() << "Collection::modifyField() - no field named " << newField_->name() << endl;
    return false;
  }

  // check to see if the people "pseudo-group" needs to be updated
  // only if only one of the two is a name
  bool wasPeople = oldField->formatFlag() == Data::Field::FormatName;
  bool isPeople = newField_->formatFlag() == Data::Field::FormatName;
  bool updatePeople = wasPeople ^ isPeople;
  if(wasPeople && !isPeople) {
    m_peopleFields.removeRef(oldField);
    if(m_peopleFields.isEmpty()) {
      m_entryGroups.remove(QString::fromLatin1("_people"));
    }
  }
  if(isPeople) {
    if(m_peopleFields.isEmpty()) {
      m_entryGroups.prepend(QString::fromLatin1("_people"));
    }
    m_peopleFields.append(oldField);
  }

  bool wasGrouped = oldField->flags() & Data::Field::AllowGrouped;
  bool isGrouped = newField_->flags() & Data::Field::AllowGrouped;
  // do this before emitting signal because entryGroup() gets called in
  // Bookcase::slotUpdateCollectionToolBar() and it might have changed
  if(wasGrouped && !isGrouped) {
    m_entryGroups.remove(newField_->name());
  } else if(!wasGrouped && isGrouped) {
    if(updatePeople || !m_entryGroupDicts.find(newField_->name())) {
      Data::EntryGroupDict* dict = 0;
      if(!m_entryGroupDicts.find(newField_->name())) {
        dict = new Data::EntryGroupDict();
        m_entryGroupDicts.insert(newField_->name(), dict);
      }

      Data::EntryGroup* group = 0;
      for(Data::EntryListIterator it(m_entryList); it.current(); ++it) {
        QStringList groups;

        if(updatePeople) {
          for(Data::FieldListIterator fIt(m_peopleFields); fIt.current(); ++fIt) {
            groups += it.current()->groupNamesByFieldName(fIt.current()->name());
          }
          // now need to remove entry from all old groups for people
          QPtrListIterator<Data::EntryGroup> groupIt(it.current()->groups());
          for( ; groupIt.current(); ++groupIt) {
            if(groupIt.current()->fieldName() == QString::fromLatin1("_people")) {
              it.current()->removeFromGroup(groupIt.current());
            }
          }
          // if more than one, need just unique values
          if(m_peopleFields.count() > 1) {
            groups.sort();
            for(unsigned i = 1; i < groups.count(); ++i) {
              if(groups[i] == groups[i-1]) {
                groups.remove(groups.at(i));
                --i; // decrement since the for loop will increment it
              }
            }
          }
          // also remove empty name
          groups.remove(s_emptyGroupName);
        } else {
          groups = it.current()->groupNamesByFieldName(newField_->name());
        }

        QStringList::ConstIterator groupIt;
        for(groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {
          if(dict && (dict->isEmpty() || !(group = dict->find(*groupIt)))) {
            if(newField_->type() == Data::Field::Bool && (*groupIt) != s_emptyGroupName) {
              group = new Data::EntryGroup(fieldTitleByName(newField_->name()), newField_->name());
            } else {
              group = new Data::EntryGroup(*groupIt, newField_->name());
            }
            dict->insert(*groupIt, group);
          }
          it.current()->addToGroup(group);
        }
      }
    }
    m_entryGroups << newField_->name();
  }

  // update category list. Need to do this before emitting signal since the
  // edit widget layout may have to change. Nobody else cares about the category.
  if(oldField->category() != newField_->category()) {
    if(fieldsByCategory(oldField->category()).count() == 1) {
      m_fieldCategories.remove(oldField->category());
    }
    oldField->setCategory(newField_->category());
    if(m_fieldCategories.findIndex(oldField->category()) == -1) {
      m_fieldCategories << oldField->category();
    }
  }

  // need to emit this before oldField gets updated
  emit signalFieldModified(this, newField_, oldField);

  // I don't actually keep the new pointer, I just copy the different properties
  // TODO: revisit this decision to improve speed?
  // BCCollectionPropDialog or whatever called this function should delete the newField pointer

  // change title
  if(oldField->title() != newField_->title()) {
    m_fieldTitleDict.remove(oldField->title());
    m_fieldTitles.remove(oldField->title());
    oldField->setTitle(newField_->title());
    m_fieldTitleDict.insert(oldField->title(), oldField);
    m_fieldTitles.append(oldField->title());
  }

  // type should hardly ever be different.
  // only possible case is if default type changed and user clicked Default in the field editor
  oldField->setType(newField_->type());

  // allowed can change for Choice
  if(oldField->type() == Data::Field::Choice) {
    oldField->setAllowed(newField_->allowed());
  }

  // change flags, faster to just set without checking if different
  oldField->setFlags(newField_->flags());

  // change format flag
  if(oldField->formatFlag() != newField_->formatFlag()) {
    oldField->setFormatFlag(newField_->formatFlag());
    // invalidate cached format strings of all entry attributes of this name
    for(Data::EntryListIterator it(m_entryList); it.current(); ++it) {
      it.current()->invalidateFormattedFieldValue(oldField->name());
    }
  }

  // change description
  if(oldField->description() != newField_->description()) {
    oldField->setDescription(newField_->description());
    // if derived, all entries need to be refreshed
    if(oldField->type() == Data::Field::Dependent) {
      emit signalRefreshAttribute(oldField);
    }
  }

  if(oldField->type() == Field::Image && newField_->type() != Field::Image) {
    --m_numImageFields;
  } else if(oldField->type() != Field::Image && newField_->type() == Field::Image) {
    ++m_numImageFields;
  }
  return true;
}

// force allows me to force the deleting of the title field if I need to
bool Collection::deleteField(Data::Field* field_, bool force_/*=false*/) {
  if(!field_ || !m_fieldList.containsRef(field_)) {
    return false;
  }
//  kdDebug() << "Collection::deleteField() - name = " << field_->name() << endl;

  // can't delete the title field
  if(field_->flags() & Data::Field::NoDelete && !force_) {
    return false;
  }
  
  bool success = true;
  if(field_->formatFlag() == Data::Field::FormatName) {
    success &= m_peopleFields.remove(field_);
  }
  if(field_->type() == Data::Field::Image) {
    success &= m_imageFields.remove(field_);
  }
  success &= m_fieldNameDict.remove(field_->name());
  success &= m_fieldTitleDict.remove(field_->title());
  success &= m_fieldNames.remove(field_->name());
  success &= m_fieldTitles.remove(field_->title());
  
  if(fieldsByCategory(field_->category()).count() == 1) {
    success &= m_fieldCategories.remove(field_->category());
  }
  
  for(Data::EntryListIterator it(m_entryList); it.current(); ++it) {
    // setting the fields to an empty string removes the value from the entry's list
    it.current()->setField(field_->name(), QString::null);
  }

  if(field_->flags() & Data::Field::AllowGrouped) {
    success &= m_entryGroupDicts.remove(field_->name());
    success &= m_entryGroups.remove(field_->name());
    if(field_->name() == m_defaultGroupField) {
      setDefaultGroupField(m_entryGroups[0]);
    }
  }

  if(field_->type() == Field::Image) {
    --m_numImageFields;
  }

  m_fieldList.setAutoDelete(false);
  success &= m_fieldList.removeRef(field_);
  m_fieldList.setAutoDelete(true);

  emit signalFieldDeleted(this, field_);
  delete field_; // don't delete until after signal
  return success;
}

void Collection::reorderFields(const Data::FieldList& list_) {
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

void Collection::addEntry(Data::Entry* entry_) {
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

void Collection::removeEntryFromDicts(Data::Entry* entry_) {
  Data::EntryGroup* group;
  QPtrListIterator<Data::EntryGroup> it(entry_->groups());
  while(it.current()) {
    group = it.current();
    Data::EntryGroupDict* dict = m_entryGroupDicts.find(group->fieldName());
    // removeFromGroup will delete the group if it becomes empty
    // so just see if there's only one entry in the group, because that
    // implies that it will be empty when the entry is removed
    // emit signalGroupModified(this, group);
    if(group->count() == 1) {
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
void Collection::updateDicts(Data::Entry* entry_) {
//  kdDebug() << "Collection::updateDicts" << endl;
  if(!entry_) {
    return;
  }

  removeEntryFromDicts(entry_);
  populateDicts(entry_);
}

bool Collection::deleteEntry(Data::Entry* entry_) {
  if(!entry_) {
    return false;
  }

//  kdDebug() << "Collection::deleteEntry() - deleted entry - " << entry_->title() << endl;
  removeEntryFromDicts(entry_);
  return m_entryList.remove(entry_);
}

Bookcase::Data::FieldList Collection::fieldsByCategory(const QString& cat_) const {
  Data::FieldList list;
  for(Data::FieldListIterator it(m_fieldList); it.current(); ++it) {
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
  Data::Field* f = fieldByTitle(title_);
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
  Data::Field* f = fieldByName(name_);
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
  for(Data::EntryListIterator it(m_entryList); it.current(); ++it) {
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
  Data::Field* field = fieldByName(key_);

  // if the type is not multiple choice or if value_ is allowed, return true
  if(field && (field->type() != Data::Field::Choice || field->allowed().findIndex(value_) > -1)) {
    return true;
  }

  return false;
}

Bookcase::Data::EntryGroupDict* const Collection::entryGroupDictByName(const QString& name_) const {
  return m_entryGroupDicts.isEmpty() ? 0 : m_entryGroupDicts.find(name_);
}

void Collection::populateDicts(Data::Entry* entry_) {
//  kdDebug() << "Collection::populateDicts" << endl;
  if(m_entryGroupDicts.isEmpty()) {
    return;
  }

  // iterate over all the possible groupDicts
  // for each dict, get the value of that field for the entry
  // if multiple values are allowed, split the value and then insert the
  // entry pointer into the dict for each value
  QDictIterator<Data::EntryGroupDict> dictIt(m_entryGroupDicts);
  for( ; dictIt.current(); ++dictIt) {
    Data::EntryGroupDict* dict = dictIt.current();
    QString attName = dictIt.currentKey();

    Data::Field* field = 0;
    QStringList groups;
    if(attName == QString::fromLatin1("_people")) {
      Data::FieldListIterator it(m_peopleFields);
      for( ; it.current(); ++it) {
        groups += entry_->groupNamesByFieldName(it.current()->name());
      }
      // if more than one, need just unique values
      if(m_peopleFields.count() > 1) {
        groups.sort();
        for(unsigned i = 1; i < groups.count(); ++i) {
          if(groups[i] == groups[i-1]) {
            groups.remove(groups.at(i));
            --i; // decrement since the for loop will increment it
          }
        }
      }
      // also remove empty name
      groups.remove(s_emptyGroupName);
    } else {
      field = fieldByName(attName);
      groups = entry_->groupNamesByFieldName(attName);
    }

    Data::EntryGroup* group = 0;

    QStringList::ConstIterator groupIt;
    for(groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {
      // if the dict is empty, or doesn't contain this particular group, create it
      if(dict->isEmpty() || !(group = dict->find(*groupIt))) {
        // if it's a bool, rather than showing "true", show field title
        // as long as it's not the empty group name
        if(field && field->type() == Data::Field::Bool && (*groupIt) != s_emptyGroupName) {
          group = new Data::EntryGroup(fieldTitleByName(attName), attName);
        } else {
          group = new Data::EntryGroup(*groupIt, attName);
        }
        dict->insert(*groupIt, group);
      }
      entry_->addToGroup(group);
    } // end group loop
  } // end dict loop
}

void Collection::groupModified(Data::EntryGroup* group_) {
  emit signalGroupModified(this, group_);
}

// static
int Collection::getID() {
  static int id = 0;
  return ++id;
}
