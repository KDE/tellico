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
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "bccollection.h"

#include <klocale.h>
#include <kdebug.h>

#include <qregexp.h>

//using the c'tor makes this a custom collection
BCCollection::BCCollection(int id_, const QString& title_, const QString& unitName_,
                           const QString& unitTitle_, const QString& iconName_/*="unknown"*/)
    : QObject(), m_id(id_), m_title(title_), m_unitName(unitName_),
      m_unitTitle(unitTitle_), m_iconName(iconName_), m_isCustom(true) {
  m_unitList.setAutoDelete(true);
  m_attributeList.setAutoDelete(true);
  m_unitGroupDicts.setAutoDelete(true);

  // all collections have a title attribute for their units
  BCAttribute* att = new BCAttribute("title", i18n("Title"));
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::FormatTitle | BCAttribute::DontComplete);
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
  m_unitList.clear();
  m_attributeList.clear();
  m_attributeCategories.clear();
  m_unitGroupDicts.clear();
  m_unitGroups.clear();
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
    return false;
  }

  m_attributeList.append(att_);
  m_attributeDict.insert(att_->name(), att_);

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
//  kdDebug() << "BCCollection::addUnit() - added unit (" << unit_->attribute("title") << ")\n";

  populateDicts(unit_);
}

void BCCollection::removeUnitFromDicts(BCUnit* unit_) {
  QPtrListIterator<BCUnitGroup> it(unit_->groups());
  BCUnitGroup* group;
  while(it.current()) {
    group = static_cast<BCUnitGroup*>(it.current());
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

  kdDebug() << "BCCollection::deleteUnit() - deleted unit (" << unit_->attribute("title") << ")\n";
  removeUnitFromDicts(unit_);
  return m_unitList.remove(unit_);
}

const QPtrList<BCAttribute>& BCCollection::attributeList() const {
  return m_attributeList;
}

const QStringList& BCCollection::attributeCategories() const {
  return m_attributeCategories;
}

QPtrList<BCAttribute> BCCollection::attributesByCategory(const QString& cat_) const {
  QPtrList<BCAttribute> list;
  QPtrListIterator<BCAttribute> it(m_attributeList);
  for( ; it.current(); ++it) {
    if(it.current()->category() == cat_) {
      list.append(it.current());
    }
  }
  return list;
}

QStringList BCCollection::attributeNames(bool all_/*=true*/) const {
//  kdDebug() << "BCCollection::attributeNames()" << endl;
  QStringList strlist;
  QPtrListIterator<BCAttribute> it(m_attributeList);
  for ( ; it.current(); ++it) {
    if(all_ || !(it.current()->flags() & BCAttribute::DontShow)) {
      strlist << it.current()->name();
    }
  }
  return strlist;
}

QStringList BCCollection::attributeTitles(bool all_/*=true*/) const {
//  kdDebug() << "BCCollection::attributeTitles()" << endl;
  QStringList strlist;
  QPtrListIterator<BCAttribute> it(m_attributeList);
  for( ; it.current(); ++it) {
    if(all_ || !(it.current()->flags() & BCAttribute::DontShow)) {
      strlist << it.current()->title();
    }
  }
  return strlist;
}

QStringList BCCollection::valuesByAttributeName(const QString& name_) const {
  QStringList strlist;
  QPtrListIterator<BCUnit> it(m_unitList);
  for( ; it.current(); ++it) {
    QString value = it.current()->attribute(name_);

    if(strlist.contains(value) == 0) { // haven't inserted it yet
      if(attributeByName(name_)->flags() & BCAttribute::AllowMultiple) {
        // the space after the semi-colon is enforced elsewhere
        strlist += QStringList::split("; ", value);
      } else { // multiple values not allowed
        // no need to call value.simplifyWhiteSpace()
        strlist += value;
      }
    }

  } // end unit loop
  return strlist;
}

BCAttribute* BCCollection::attributeByName(const QString& name_) const {
  return m_attributeDict.isEmpty() ? 0 : m_attributeDict.find(name_);
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

BCUnitGroupDict* BCCollection::unitGroupDictByName(const QString& name_) const {
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

    QStringList groups = unit_->groupsByAttributeName(attName);

    QStringList::Iterator groupIt;
    for(groupIt = groups.begin() ; groupIt != groups.end(); ++groupIt) {
      QString groupName = static_cast<QString>(*groupIt);
//      kdDebug() << "\tnew group - " << groupName << endl;
      BCUnitGroup* group;

      // surely there's a more efficient way of doing this
      // app crashes if find() is called on an empty dict
      if(dict->isEmpty()) {
        group = new BCUnitGroup(groupName, attName);
        dict->insert(groupName, group);
      } else {
        group = dict->find(groupName);
        if(!group) {
          group = new BCUnitGroup(groupName, attName);
          dict->insert(groupName, group);
        }
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

BCCollection* BCCollection::Books(int id_) {
  BCCollection* coll = new BCCollection(id_, i18n("My Books"), "book",
                                        i18n("Book"), "book");

  BCAttribute* att;

  att = new BCAttribute("subtitle", i18n("Subtitle"));
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::FormatName);
  coll->addAttribute(att);

  att = new BCAttribute("author", i18n("Author"));
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::AllowMultiple | BCAttribute::FormatName | BCAttribute::AllowGrouped);
  coll->addAttribute(att);
  coll->setDefaultGroupAttribute(att->name());

  QStringList binding;
  binding << i18n("Hardback") << i18n("Paperback") << i18n("Trade Paperback")
     << i18n("E-Book") << i18n("Magazine") << i18n("Journal");
  att = new BCAttribute("binding", i18n("Binding"), binding);
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("pur_date", i18n("Purchase Date"));
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::DontComplete | BCAttribute::FormatDate);
  coll->addAttribute(att);

  att = new BCAttribute("pur_price", i18n("Purchase Price"));
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::DontComplete);
  coll->addAttribute(att);

  att = new BCAttribute("publisher", i18n("Publisher"));
  att->setCategory(i18n("Publishing"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("edition", i18n("Edition"));
  att->setCategory(i18n("Publishing"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::DontComplete);
  coll->addAttribute(att);

  att = new BCAttribute("cr_year", i18n("Copyright Year"));
  att->setCategory(i18n("Publishing"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::DontComplete | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("pub_year", i18n("Publication Year"));
  att->setCategory(i18n("Publishing"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::DontComplete | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("isbn", i18n("ISBN#"));
  att->setCategory(i18n("Publishing"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::DontComplete);
  att->setDescription(i18n("International Standard Book Number"));
  coll->addAttribute(att);

  att = new BCAttribute("lccn", i18n("LCCN#"));
  att->setCategory(i18n("Publishing"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::DontComplete);
  att->setDescription(i18n("Library of Congress Control Number"));
  coll->addAttribute(att);

  att = new BCAttribute("pages", i18n("Pages"));
  att->setCategory(i18n("Publishing"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::DontComplete);
  coll->addAttribute(att);

  att = new BCAttribute("language", i18n("Language"));
  att->setCategory(i18n("Publishing"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("genre", i18n("Genre"));
  att->setCategory(i18n("Classification"));
  att->setFlags(BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("keywords", i18n("Keywords"));
  att->setCategory(i18n("Classification"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("series", i18n("Series"));
  att->setCategory(i18n("Classification"));
  att->setFlags(BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("series_num", i18n("Series Number"));
  att->setCategory(i18n("Classification"));
  att->setFlags(BCAttribute::DontComplete | BCAttribute::DontShow);
  coll->addAttribute(att);

  QStringList cond;
  cond << i18n("New") << i18n("Used");
  att = new BCAttribute("condition", i18n("Condition"), cond);
  att->setCategory(i18n("Classification"));
  att->setFlags(BCAttribute::DontShow);
  coll->addAttribute(att);

  att = new BCAttribute("signed", i18n("Signed"), BCAttribute::Bool);
  att->setCategory(i18n("Personal"));
  att->setFlags(BCAttribute::DontShow);
  coll->addAttribute(att);

  att = new BCAttribute("read", i18n("Read"), BCAttribute::Bool);
  att->setCategory(i18n("Personal"));
  att->setFlags(BCAttribute::DontShow);
  coll->addAttribute(att);

  att = new BCAttribute("gift", i18n("Gift"), BCAttribute::Bool);
  att->setCategory(i18n("Personal"));
  att->setFlags(BCAttribute::DontShow);
  coll->addAttribute(att);

  att = new BCAttribute("loaned", i18n("Loaned"), BCAttribute::Bool);
  att->setCategory(i18n("Personal"));
  att->setFlags(BCAttribute::DontShow);
  coll->addAttribute(att);

  QStringList rating;
  rating << i18n("5 - Best") << i18n("4 - Good") << i18n("3 - Neutral") << i18n("2 - Bad") << i18n("1 - Worst");
  att = new BCAttribute("rating", i18n("Rating"), rating);
  att->setCategory(i18n("Personal"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("comments", i18n("Comments"));
  att->setCategory(i18n("Personal"));
  att->setFlags(BCAttribute::DontComplete);
  coll->addAttribute(att);

  coll->m_isCustom = false;
  return coll;
}

#if 0
BCCollection* BCCollection::CDs(int id_) {
  BCCollection* coll = new BCCollection(id_, i18n("My CDs"), "cd", i18n("CD"), "cd");

  BCAttribute* att;

  att = new BCAttribute("artist", i18n("Artist"));
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::AllowGrouped);
  coll->addAttribute(att);
  coll->setDefaultGroupAttribute(att->name());

  att = new BCAttribute("year", i18n("Year"));
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::DontComplete | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("genre", i18n("Genre"));
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  QStringList list;
  list << i18n("CD") << i18n("Cassette") << i18n("8-track");
  att = new BCAttribute("medium", i18n("Medium"), list);
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("comments", i18n("Comments"));
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::DontComplete);
  coll->addAttribute(att);

  coll->m_isCustom = false;
  return coll;
}

BCCollection* BCCollection::Videos(int id_) {
  BCCollection* coll = new BCCollection(id_, i18n("My Videos"), "video", i18n("Video"), "video");

  BCAttribute* att;

  att = new BCAttribute("date", i18n("Date"));
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::DontComplete);
  coll->addAttribute(att);

  att = new BCAttribute("genre", i18n("Genre"));
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  coll->addAttribute(att);
  coll->setDefaultGroupAttribute(att->name());

  QStringList list;
  list << i18n("DVD") << i18n("VHS") << i18n("DivX") << i18n("8mm") << i18n("VCD") << i18n("LaserDisc") << i18n("Betamax");
  att = new BCAttribute("medium", i18n("Medium"), list);
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("comments", i18n("Comments"));
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::DontComplete);
  coll->addAttribute(att);

  coll->m_isCustom = false;
  return coll;
}
#endif
