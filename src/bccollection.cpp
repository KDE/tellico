/***************************************************************************
                              bccollection.cpp
                             -------------------
    begin                : Sat Sep 15 2001
    copyright            : (C) 2001, 2002, 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bccollection.h"

#include <klocale.h>
#include <kdebug.h>

#include <qregexp.h>

const QString BCCollection::s_emptyGroupName = i18n("(Empty)");

BCCollection::BCCollection(const QString& title_, const QString& unitName_, const QString& unitTitle_)
    : QObject(), m_title(title_), m_unitName(unitName_), m_unitTitle(unitTitle_),
      m_defaultGroupAttribute(QString::fromLatin1("title")) {
  m_unitList.setAutoDelete(true);
  m_attributeList.setAutoDelete(true);
  m_unitGroupDicts.setAutoDelete(true);
  m_id = BCCollection::getID();
  m_iconName = unitName_ + QString::fromLatin1("s");

  // all collections have a title attribute for their units
//  BCAttribute* att = new BCAttribute(QString::fromLatin1("title"), i18n("Title"));
//  att->setCategory(i18n("General"));
//  att->setFormatFlag(BCAttribute::FormatTitle);
//  addAttribute(att);
  
  setDefaultViewAttributes(QStringList::split(',', QString::fromLatin1("title")));
}

BCCollection::~BCCollection() {
//  m_unitList.clear();
//  m_attributeList.clear();
//  m_attributeCategories.clear();
//  m_unitGroupDicts.clear();
//  m_unitGroups.clear();
}

unsigned BCCollection::unitCount() const {
  return m_unitList.count();
}

bool BCCollection::addAttributes(const BCAttributeList& list_) {
  bool success = true;
  BCAttributeListIterator it(list_);
  for( ; it.current(); ++it) {
    success &= addAttribute(it.current());
  }
  return success;
}

bool BCCollection::addAttribute(BCAttribute* att_) {
  if(!att_) {
    return false;
  }

  // attributeByName() returns 0 if there's no attribute by that name
  // this essentially checks for duplicates
  if(attributeByName(att_->name())) {
//    kdDebug() << "BCCollection::addAttribute() - replacing " << att_->name() << endl;
    deleteAttribute(attributeByName(att_->name()), true);
  }

//  kdDebug() << "BCCollection::addAttribute() - adding " << att_->name() << endl;
  m_attributeList.append(att_);
  if(att_->formatFlag() == BCAttribute::FormatName) {
    m_peopleAttributeList.append(att_); // list of people attributes
    if(m_peopleAttributeList.count() > 1) {
      // the first time that a person attribute is added, add a "pseudo-group" for people
      QString people = QString::fromLatin1("_people");
      if(m_unitGroupDicts.find(people) == 0) {
        m_unitGroupDicts.insert(people, new BCUnitGroupDict());
        m_unitGroups.prepend(people);
      }
    }
  }
  m_attributeNameDict.insert(att_->name(), att_);
  m_attributeTitleDict.insert(att_->title(), att_);
  m_attributeNames << att_->name();
  m_attributeTitles << att_->title();

  //att_->category() will never be empty
  if(m_attributeCategories.contains(att_->category()) == 0) {
    m_attributeCategories << att_->category();
  }

  if(att_->flags() & BCAttribute::AllowGrouped) {
    // m_unitGroupDicts autoDeletes each QDict when the BCCollection d'tor is called
    BCUnitGroupDict* dict = new BCUnitGroupDict();
    // don't autoDelete, since the group is deleted when it becomes
    // empty in BCUnit::removeFromGroup()
    m_unitGroupDicts.insert(att_->name(), dict);
    // cache the possible groups of units
    m_unitGroups << att_->name();
  }

  BCUnitListIterator it(m_unitList);
  for( ; it.current(); ++it) {
    populateDicts(it.current());
  }

  emit signalAttributeAdded(this, att_);
  return true;
}

// att_ is the new variable
// find the old variable with the same name, and modify it
bool BCCollection::modifyAttribute(BCAttribute* newAtt_) {
  if(!newAtt_) {
    return false;
  }
//  kdDebug() << "BCCollection::modifyAttribute()" << endl;
// the attribute name never changes
  BCAttribute* oldAtt = attributeByName(newAtt_->name());
  if(!oldAtt) {
    kdDebug() << "BCCollection::modifyAttribute() - no attribute named " << newAtt_->name() << endl;
    return false;
  }

  // check to see if the people "pseudo-group" needs to be updated
  // only if only one of the two is a name
  bool wasPeople = oldAtt->formatFlag() == BCAttribute::FormatName;
  bool isPeople = newAtt_->formatFlag() == BCAttribute::FormatName;
  bool updatePeople = wasPeople ^ isPeople;
  if(wasPeople && !isPeople) {
    m_peopleAttributeList.removeRef(oldAtt);
    if(m_peopleAttributeList.isEmpty()) {
      m_unitGroups.remove(QString::fromLatin1("_people"));
    }
  }
  if(isPeople) {
    if(m_peopleAttributeList.isEmpty()) {
      m_unitGroups.prepend(QString::fromLatin1("_people"));
    }
    m_peopleAttributeList.append(oldAtt);
  }

  bool wasGrouped = oldAtt->flags() & BCAttribute::AllowGrouped;
  bool isGrouped = newAtt_->flags() & BCAttribute::AllowGrouped;
  // do this before emitting signal because unitGroup() gets called in
  // Bookcase::slotUpdateCollectionToolBar() and it might have changed
  if(wasGrouped && !isGrouped) {
    m_unitGroups.remove(newAtt_->name());
  } else if(!wasGrouped && isGrouped) {
    if(updatePeople || !m_unitGroupDicts.find(newAtt_->name())) {
      BCUnitGroupDict* dict = 0;
      if(!m_unitGroupDicts.find(newAtt_->name())) {
        dict = new BCUnitGroupDict();
        m_unitGroupDicts.insert(newAtt_->name(), dict);
      }

      BCUnitGroup* group = 0;
      BCUnitListIterator it(m_unitList);
      for( ; it.current(); ++it) {
        QStringList groups;

        if(updatePeople) {
          BCAttributeListIterator attIt(m_peopleAttributeList);
          for( ; attIt.current(); ++attIt) {
            groups += it.current()->groupNamesByAttributeName(attIt.current()->name());
          }
          // now need to remove unit from all old groups for people
          QPtrListIterator<BCUnitGroup> groupIt(it.current()->groups());
          for( ; groupIt.current(); ++groupIt) {
            if(groupIt.current()->attributeName() == QString::fromLatin1("_people")) {
              it.current()->removeFromGroup(groupIt.current());
            }
          }
          // if more than one, need just unique values
          if(m_peopleAttributeList.count() > 1) {
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
          groups = it.current()->groupNamesByAttributeName(newAtt_->name());
        }

        QStringList::ConstIterator groupIt;
        for(groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {
          if(dict && (dict->isEmpty() || !(group = dict->find(*groupIt)))) {
            if(newAtt_->type() == BCAttribute::Bool && (*groupIt) != s_emptyGroupName) {
              group = new BCUnitGroup(attributeTitleByName(newAtt_->name()), newAtt_->name());
            } else {
              group = new BCUnitGroup(*groupIt, newAtt_->name());
            }
            dict->insert(*groupIt, group);
          }
          it.current()->addToGroup(group);
        }
      }
    }
    m_unitGroups << newAtt_->name();
  }

  // update category list. Need to do this before emitting signal since the
  // edit widget layout may have to change. Nobody else cares about the category.
  if(oldAtt->category() != newAtt_->category()) {
    if(attributesByCategory(oldAtt->category()).count() == 1) {
      m_attributeCategories.remove(oldAtt->category());
    }
    oldAtt->setCategory(newAtt_->category());
    if(m_attributeCategories.contains(oldAtt->category()) == 0) {
      m_attributeCategories << oldAtt->category();
    }
  }

  // need to emit this before oldAtt gets updated
  emit signalAttributeModified(this, newAtt_, oldAtt);

  // I don't actually keep the new pointer, I just copy the different properties
  // TODO: revisit this decision to improve speed?
  // BCCollectionPropDialog or whatever called this function should delete the newAtt pointer

  // change title
  if(oldAtt->title() != newAtt_->title()) {
    m_attributeTitleDict.remove(oldAtt->title());
    m_attributeTitles.remove(oldAtt->title());
    oldAtt->setTitle(newAtt_->title());
    m_attributeTitleDict.insert(oldAtt->title(), oldAtt);
    m_attributeTitles.append(oldAtt->title());
  }

  // type should hardly ever be different.
  // only possible case is if default type changed and user clicked Default in the field editor
  oldAtt->setType(newAtt_->type());

  // allowed can change for Choice
  if(oldAtt->type() == BCAttribute::Choice) {
    oldAtt->setAllowed(newAtt_->allowed());
  }

  // change description, faster to just set without checking if different
  oldAtt->setDescription(newAtt_->description());

  // change flags, faster to just set without checking if different
  oldAtt->setFlags(newAtt_->flags());

  // change format flag
  if(oldAtt->formatFlag() != newAtt_->formatFlag()) {
    oldAtt->setFormatFlag(newAtt_->formatFlag());
    // invalidate cached format strings of all unit attributes of this name
    BCUnitListIterator it(m_unitList);
    for( ; it.current(); ++it) {
      it.current()->invalidateFormattedAttributeValue(oldAtt->name());
    }
  }

  return true;
}

// force allows me to force the deleting of the title attribute if I need to
bool BCCollection::deleteAttribute(BCAttribute* att_, bool force_/*=false*/) {
  if(!att_ || !m_attributeList.containsRef(att_)) {
    return false;
  }

  // can't delete the title attribute
  if(att_->name() == QString::fromLatin1("title") && !force_) {
    return false;
  }
  
  bool success = true;
  if(att_->formatFlag() == BCAttribute::FormatName) {
    success &= m_peopleAttributeList.remove(att_);
  }
  success &= m_attributeNameDict.remove(att_->name());
  success &= m_attributeTitleDict.remove(att_->title());
  success &= m_attributeNames.remove(att_->name());
  success &= m_attributeTitles.remove(att_->title());
  
  if(attributesByCategory(att_->category()).count() == 1) {
    success &= m_attributeCategories.remove(att_->category());
  }
  
  if(att_->flags() & BCAttribute::AllowGrouped) {
    BCUnitListIterator it(m_unitList);
    for( ; it.current(); ++it) {
      // setting the attribute to an empty string remove the value from the unit's list
      // and also removes the unit from any groups of that attribute
      it.current()->setAttribute(att_->name(), QString::null);
    }
    // if the attribute is not a groupable one, then all the units retain that value in
    // their maps, but not big deal, it's never read. Only might be a problem later if the user
    // adds an attribute with the same name as the deleted one. TODO: fix this some day
    // I could just set an empty string on every unit in the collection, but that would mean the
    // group iterator loops for every unit over every group the unit belongs to

    success &= m_unitGroupDicts.remove(att_->name());
    success &= m_unitGroups.remove(att_->name());
    if(att_->name() == m_defaultGroupAttribute) {
      setDefaultGroupAttribute(m_unitGroups[0]);
    }
  }
  
  emit signalAttributeDeleted(this, att_); // emit before actually deleting
  success &= m_attributeList.removeRef(att_); // auto deleted
  return success;
}

void BCCollection::reorderAttributes(const BCAttributeList& list_) {
// assume the lists have the same pointers!
  m_attributeList.setAutoDelete(false);
  m_attributeList = list_;
  m_attributeList.setAutoDelete(true);
  emit signalAttributesReordered(this);
}

void BCCollection::addUnit(BCUnit* unit_) {
  if(!unit_) {
    return;
  }

  if(unit_->collection() != this) {
    // should the addUnit() call be done in the BCUnit constructor?
    kdWarning() << "BCCollection::addUnit() - unit is not being added to its parent collection" << endl;
  }

  m_unitList.append(unit_);
//  kdDebug() << "BCCollection::addUnit() - added unit (" << unit_->title() << ")" << endl;
  
  populateDicts(unit_);
}

void BCCollection::removeUnitFromDicts(BCUnit* unit_) {
  BCUnitGroup* group;
  QPtrListIterator<BCUnitGroup> it(unit_->groups());
  while(it.current()) {
    group = it.current();
    BCUnitGroupDict* dict = m_unitGroupDicts.find(group->attributeName());
    // removeFromGroup will delete the group if it becomes empty
    // so just see if there's only one unit in the group, because that
    // implies that it will be empty when the unit is removed
    // emit signalGroupModified(this, group);
    if(group->count() == 1) {
      dict->remove(group->groupName());
    }
    // increment the iterator before calling removeFromGroup since the
    // group might be deleted
    ++it;
    unit_->removeFromGroup(group);
  }
}

// this function gets called whenever a unit is modified. Its purpose is to keep the
// groupDicts current. It first removes the unit from every group to which it belongs,
// then it repopulates the dicts with the unit's attributes
void BCCollection::updateDicts(BCUnit* unit_) {
//  kdDebug() << "BCCollection::updateDicts" << endl;
  if(!unit_) {
    return;
  }

  removeUnitFromDicts(unit_);
  populateDicts(unit_);
}

bool BCCollection::deleteUnit(BCUnit* unit_) {
  if(!unit_) {
    return false;
  }

//  kdDebug() << "BCCollection::deleteUnit() - deleted unit - " << unit_->title() << endl;
  removeUnitFromDicts(unit_);
  return m_unitList.remove(unit_);
}

const BCAttributeList& BCCollection::attributeList() const {
  return m_attributeList;
}

const BCAttributeList& BCCollection::peopleAttributeList() const {
  return m_peopleAttributeList;
}

const QStringList& BCCollection::attributeCategories() const {
  return m_attributeCategories;
}

BCAttributeList BCCollection::attributesByCategory(const QString& cat_) const {
  BCAttributeList list;
  BCAttributeListIterator it(m_attributeList);
  for( ; it.current(); ++it) {
    if(it.current()->category() == cat_) {
      list.append(it.current());
    }
  }
  return list;
}

const QStringList& BCCollection::attributeNames() const {
  return m_attributeNames;
}

const QStringList& BCCollection::attributeTitles() const {
  return m_attributeTitles;
}

const QString& BCCollection::attributeNameByTitle(const QString& title_) const {
  if(title_.isEmpty()) {
    return QString::null;
  }
  BCAttribute* att = attributeByTitle(title_);
  if(!att) {
    kdWarning() << "BCCollection::attributeNameByTitle() - no attribute titled " << title_ << endl;
    return QString::null;
  }
  return att->name();
}

const QString& BCCollection::attributeTitleByName(const QString& name_) const {
  if(name_.isEmpty()) {
    return QString::null;
  }
  BCAttribute* att = attributeByName(name_);
  if(!att) {
    kdWarning() << "BCCollection::attributeTitleByName() - no attribute named " << name_ << endl;
    return QString::null;
  }
  return att->title();
}

QStringList BCCollection::valuesByAttributeName(const QString& name_) const {
  if(name_.isEmpty()) {
    return QStringList();
  }
  QStringList strlist;
  QPtrListIterator<BCUnit> it(m_unitList);
  for( ; it.current(); ++it) {
    QString value = it.current()->attribute(name_);

    if(strlist.contains(value) == 0) { // haven't inserted it yet
      if(attributeByName(name_)->flags() & BCAttribute::AllowMultiple) {
        // the space after the semi-colon is enforced elsewhere
        strlist += QStringList::split(QString::fromLatin1("; "), value);
      } else { // multiple values not allowed
        // no need to call value.simplifyWhiteSpace()
        strlist += value;
      }
    }

  } // end unit loop
  return strlist;
}

BCAttribute* const BCCollection::attributeByName(const QString& name_) const {
  return m_attributeNameDict.isEmpty() ? 0 : m_attributeNameDict.find(name_);
}

BCAttribute* const BCCollection::attributeByTitle(const QString& title_) const {
  return m_attributeTitleDict.isEmpty() ? 0 : m_attributeTitleDict.find(title_);
}

bool BCCollection::isAllowed(const QString& key_, const QString& value_) const {
  // empty string is always allowed
  if(value_.isEmpty()) {
    return true;
  }

  // find the attribute with a name of 'key_'
  BCAttribute* att = attributeByName(key_);

  // if the type is not multiple choice or if value_ is allowed, return true
  if(att && (att->type() != BCAttribute::Choice || att->allowed().contains(value_))) {
    return true;
  }

  return false;
}

int BCCollection::id() const {
  return m_id;
}

const QString& BCCollection::title() const {
  return m_title;
}

void BCCollection::setTitle(const QString& title_) {
  m_title = title_;
}

const QString& BCCollection::unitName() const {
  return m_unitName;
}

const QString& BCCollection::unitTitle() const {
  return m_unitTitle;
}

const QString& BCCollection::iconName() const {
  return m_iconName;
}

void BCCollection::setIconName(const QString& name_) {
  m_iconName = name_;
}

const QString& BCCollection::defaultGroupAttribute() const {
  return m_defaultGroupAttribute;
}

void BCCollection::setDefaultGroupAttribute(const QString& name_) {
  m_defaultGroupAttribute = name_;
#ifdef KDEBUG
  if(attributeByName(name_) == 0) {
    kdDebug() << "BCCollection::setDefaultGroupAttribute() - no attribute named " << name_ << endl;
  }
#endif
}

const QStringList& BCCollection::defaultViewAttributes() const {
  return m_defaultViewAttributes;
}

void BCCollection::setDefaultViewAttributes(const QStringList& list_) {
  m_defaultViewAttributes = list_;
}

const BCUnitList& BCCollection::unitList() const {
  return m_unitList;
}

const QStringList& BCCollection::unitGroups() const {
  return m_unitGroups;
}

BCUnitGroupDict* const BCCollection::unitGroupDictByName(const QString& name_) const {
  return m_unitGroupDicts.isEmpty() ? 0 : m_unitGroupDicts.find(name_);
}

void BCCollection::populateDicts(BCUnit* unit_) {
//  kdDebug() << "BCCollection::populateDicts" << endl;
  if(m_unitGroupDicts.isEmpty()) {
    return;
  }

  // iterate over all the possible groupDicts
  // for each dict, get the value of that attribute for the unit
  // if multiple values are allowed, split the value and then insert the
  // unit pointer into the dict for each value
  QDictIterator<BCUnitGroupDict> dictIt(m_unitGroupDicts);
  for( ; dictIt.current(); ++dictIt) {
    BCUnitGroupDict* dict = dictIt.current();
    QString attName = dictIt.currentKey();

    BCAttribute* att = 0;
    QStringList groups;
    if(attName == QString::fromLatin1("_people")) {
      BCAttributeListIterator it(m_peopleAttributeList);
      for( ; it.current(); ++it) {
        groups += unit_->groupNamesByAttributeName(it.current()->name());
      }
      // if more than one, need just unique values
      if(m_peopleAttributeList.count() > 1) {
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
      att = attributeByName(attName);
      groups = unit_->groupNamesByAttributeName(attName);
    }

    BCUnitGroup* group = 0;

    QStringList::ConstIterator groupIt;
    for(groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {
      // if the dict is empty, or doesn't contain this particular group, create it
      if(dict->isEmpty() || !(group = dict->find(*groupIt))) {
        // if it's a bool, rather than showing "true", show attribute title
        // as long as it's not the empty group name
        if(att && att->type() == BCAttribute::Bool && (*groupIt) != s_emptyGroupName) {
          group = new BCUnitGroup(attributeTitleByName(attName), attName);
        } else {
          group = new BCUnitGroup(*groupIt, attName);
        }
        dict->insert(*groupIt, group);
      }
      unit_->addToGroup(group);
    } // end group loop
  } // end dict loop
}

void BCCollection::groupModified(BCUnitGroup* group_) {
  emit signalGroupModified(this, group_);
}

// static
int BCCollection::getID() {
  static int id = 0;
  return ++id;
}
