/***************************************************************************
                              bccollection.cpp
                             -------------------
    begin                : Sat Sep 15 2001
    copyright            : (C) 2001 by Robby Stephenson
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

//using the c'tor makes this a custom collection
BCCollection::BCCollection(int id_, const QString& title_, const QString& unitName_,
                           const QString& unitTitle_)
    : QObject(), m_id(id_), m_title(title_), m_unitName(unitName_),
      m_unitTitle(unitTitle_), m_iconName(unitName_) {
  m_unitList.setAutoDelete(true);
  m_attributeList.setAutoDelete(true);
  m_unitGroupDicts.setAutoDelete(true);

  // all collections have a title attribute for their units
  BCAttribute* att = new BCAttribute(QString::fromLatin1("title"), i18n("Title"));
  att->setCategory(i18n("&General"));
  att->setFormatFlag(BCAttribute::FormatTitle);
  addAttribute(att);
}

BCCollection::BCCollection(const BCCollection&) : QObject() {
  kdWarning() << "BCCollection copy constructor - should not be used!!!" << endl;
}

BCCollection BCCollection::operator=(const BCCollection& coll_) {
  kdWarning() << "BCCollection assignment operator - should not be used!!!" << endl;
  return BCCollection(coll_);
}

BCCollection::~BCCollection() {
//  m_unitList.clear();
//  m_attributeList.clear();
//  m_attributeCategories.clear();
//  m_unitGroupDicts.clear();
//  m_unitGroups.clear();
}

BCCollection::CollectionType BCCollection::collectionType() const {
  return BCCollection::Base;
}

bool BCCollection::isBook() const {
  return false;
}

bool BCCollection::isSong() const {
  return false;
}

bool BCCollection::isVideo() const {
  return false;
}

unsigned BCCollection::unitCount() const {
  return m_unitList.count();
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

  m_attributeList.append(att_);
  m_attributeNameDict.insert(att_->name(), att_);
  m_attributeTitleDict.insert(att_->title(), att_);

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
  m_isCustom = true;
  return true;
}

// att_ is the new variable
// find the old variable with the same name, and modify it
bool BCCollection::modifyAttribute(BCAttribute* newAtt) {
  if(!newAtt) {
    return false;
  }
//  kdDebug() << "BCCollection::modifyAttribute()" << endl;
  BCAttribute* oldAtt = attributeByName(newAtt->name());
  if(!oldAtt) {
    kdDebug() << "BCCollection::modifyAttribute() - no attribute named " << newAtt->title() << endl;
    return false;
  }

  // do this before emitting signal because unitGroup() gets called in
  // Bookcase::slotUpdateCollectionToolBar() and it might have changed
  m_unitGroups.remove(newAtt->name());
  if(newAtt->flags() & BCAttribute::AllowGrouped) {
    if(!m_unitGroupDicts.find(newAtt->name())) {
      BCUnitGroupDict* dict = new BCUnitGroupDict();
      m_unitGroupDicts.insert(newAtt->name(), dict);

      BCUnitGroup* group;
      BCUnitListIterator it(m_unitList);
      for( ; it.current(); ++it) {
        QStringList groups = it.current()->groupNamesByAttributeName(newAtt->name());
        QStringList::ConstIterator groupIt;
        for(groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {
          if(dict->isEmpty() || !(group = dict->find(*groupIt))) {
            group = new BCUnitGroup(*groupIt, newAtt->name());
            dict->insert(*groupIt, group);
          }
          it.current()->addToGroup(group);
        }
      }
    }
    m_unitGroups << newAtt->name();
  }

  // need to emit this before oldAtt gets updated
  emit signalAttributeModified(this, newAtt, oldAtt);

  // I don't actually keep the new pointer, I just copy the different properties
  // TODO: revisit this decision to improve speed?
  // BCCollectionPropDialog or whatever called this function should delete the newAtt pointer

  // change title
  if(oldAtt->title() != newAtt->title()) {
    m_attributeTitleDict.remove(oldAtt->title());
    oldAtt->setTitle(newAtt->title());
    m_attributeTitleDict.insert(oldAtt->title(), oldAtt);
  }

  // change category
  if(oldAtt->category() != newAtt->category()) {
    if(attributesByCategory(oldAtt->category()).count() == 1) {
      m_attributeCategories.remove(newAtt->category());
    }
    oldAtt->setCategory(newAtt->category());
    if(m_attributeCategories.contains(oldAtt->category()) == 0) {
      m_attributeCategories << oldAtt->category();
    }
  }

  // type won't can't be changed, but allowed can for Choice
  if(oldAtt->type() == BCAttribute::Choice && oldAtt->allowed() != newAtt->allowed()) {
    oldAtt->setAllowed(newAtt->allowed());
  }

  // change description
  if(oldAtt->description() != newAtt->description()) {
    oldAtt->setDescription(newAtt->description());
  }

  // change flags
  if(oldAtt->flags() != newAtt->flags()) {
    oldAtt->setFlags(newAtt->flags());
  }

  // change format flag
  if(oldAtt->formatFlag() != newAtt->formatFlag()) {
    oldAtt->setFormatFlag(newAtt->formatFlag());
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
  
  m_attributeNameDict.remove(att_->name());
  m_attributeTitleDict.remove(att_->title());
  
  if(attributesByCategory(att_->category()).count() == 1) {
    m_attributeCategories.remove(att_->category());
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

    m_unitGroupDicts.remove(att_->name());
    m_unitGroups.remove(att_->name());
    if(att_->name() == m_defaultGroupAttribute) {
      setDefaultGroupAttribute(m_unitGroups[0]);
    }
  }
  
  emit signalAttributeDeleted(this, att_); // emit before actually deleting
  m_attributeList.removeRef(att_); // auto deleted
  m_isCustom = true;
  return true;
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

QStringList BCCollection::attributeNames() const {
//  kdDebug() << "BCCollection::attributeNames()" << endl;
  QStringList strlist;
  BCAttributeListIterator it(m_attributeList);
  for ( ; it.current(); ++it) {
    strlist << it.current()->name();
  }
  return strlist;
}

QStringList BCCollection::attributeTitles() const {
//  kdDebug() << "BCCollection::attributeTitles()" << endl;
  QStringList strlist;
  BCAttributeListIterator it(m_attributeList);
  for( ; it.current(); ++it) {
    strlist << it.current()->title();
  }
  return strlist;
}

const QString& BCCollection::attributeNameByTitle(const QString& title_) const {
  BCAttribute* att = attributeByTitle(title_);
  if(!att) {
    kdWarning() << "BCCollection::attributeNameByTitle() - no attribute titled " << title_ << endl;
    return QString::null;
  }
  return att->name();
}

const QString& BCCollection::attributeTitleByName(const QString& name_) const {
  BCAttribute* att = attributeByName(name_);
  if(!att) {
    kdWarning() << "BCCollection::attributeTitleByName() - no attribute named " << name_ << endl;
    return QString::null;
  }
  return att->title();
}

QStringList BCCollection::valuesByAttributeName(const QString& name_) const {
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

bool BCCollection::isCustom() const {
  return m_isCustom;
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
  if(name_ != iconName()) {
    m_isCustom = true;
  }
  m_iconName = name_;
}

const QString& BCCollection::defaultGroupAttribute() const {
  return m_defaultGroupAttribute;
}

void BCCollection::setDefaultGroupAttribute(const QString& name_) {
  // the collection is now custom if the default UnitGroup was already set and
  // this is a new value
  if(!m_defaultGroupAttribute.isEmpty() && name_ != m_defaultGroupAttribute) {
    m_isCustom = true;
  }
  m_defaultGroupAttribute = name_;
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

    BCUnitGroup* group = 0;

    QStringList groups = unit_->groupNamesByAttributeName(attName);
    QStringList::ConstIterator groupIt;
    for(groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {
      // if the dict is empty, or doesn't contain this particular group, create it
      if(dict->isEmpty() || !(group = dict->find(*groupIt))) {
        group = new BCUnitGroup(*groupIt, attName);
        dict->insert(*groupIt, group);
      }
      unit_->addToGroup(group);
    } // end group loop
  } // end dict loop
}

QString BCCollection::emptyGroupName() {
  return i18n("(Empty)");
}

void BCCollection::groupModified(BCUnitGroup* group_) {
  emit signalGroupModified(this, group_);
}
