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

BCCollection::BCCollection(int id_, const QString& title_, const QString& unitName_,
                           const QString& unitTitle_, const QString& iconName_/*="unknown"*/)
  : m_id(id_), m_title(title_), m_unitName(unitName_), m_unitTitle(unitTitle_), m_iconName(iconName_) {
  m_unitList.setAutoDelete(true);
  m_attributeList.setAutoDelete(true);
  m_groupAttribute = 0;
  m_isCustom = true;

  // all collections have a title attribute for their units
  BCAttribute* att = new BCAttribute("title", i18n("Title"));
  att->setGroup(i18n("General"));
  att->setFlags(BCAttribute::FormatTitle | BCAttribute::DontComplete);
  addAttribute(att);
}

BCCollection::BCCollection(const BCCollection&) {
  kdWarning() << "BCCollection copy constructor - should not be used!!!" << endl;
}

BCCollection BCCollection::operator=(const BCCollection& coll_) {
  kdWarning() << "BCCollection assignment operator - should not be used!!!" << endl;
  return BCCollection(coll_);
}

BCCollection::~BCCollection() {
  m_unitList.clear();
  m_attributeList.clear();
  m_attributeGroups.clear();
}

unsigned BCCollection::unitCount() const {
  return m_unitList.count();
}

bool BCCollection::addAttribute(BCAttribute* att_) {
  if(!att_) {
    return false;
  }

  // a QList does not have a function for comparing the contents of the pointers
  QListIterator<BCAttribute> it(m_attributeList);
  for( ; it.current(); ++it) {
    // only allow unique names
    if(it.current()->name() == att_->name()) {
      return false;
    }
  }
  m_attributeList.append(att_);

  if(!att_->group().isEmpty() && m_attributeGroups.contains(att_->group()) == 0) {
    m_attributeGroups << att_->group();
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
    kdDebug() << "BCCollection::addUnit() - unit is not being added to its proper collection" << endl;
  }

  m_unitList.append(unit_);
//  kdDebug() << "BCCollection::addUnit() - added unit (" << unit_->attribute("title") << ")\n";
}

bool BCCollection::deleteUnit(BCUnit* unit_) {
  if(!unit_) {
    return false;
  }

  kdDebug() << "BCCollection::deleteUnit() - deleted unit (" << unit_->attribute("title") << ")\n";
  return m_unitList.remove(unit_);
}

unsigned BCCollection::attributeCount() const {
  return m_attributeList.count();
}

const QList<BCAttribute>& BCCollection::attributeList() const {
  return m_attributeList;
}

const QStringList& BCCollection::attributeGroups() const {
  return m_attributeGroups;
}

QList<BCAttribute> BCCollection::attributeListByGroup(const QString& group_) const {
  QList<BCAttribute> list;
  QListIterator<BCAttribute> it(m_attributeList);
  for( ; it.current(); ++it) {
    if(it.current()->group() == group_) {
      list.append(it.current());
    }
  }
  return list;
}

QStringList BCCollection::attributeNames(bool all_/*=true*/) const {
//  kdDebug() << "BCCollection::attributeNames()" << endl;
  QStringList strlist;
  QListIterator<BCAttribute> it(m_attributeList);
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
  QListIterator<BCAttribute> it(m_attributeList);
  for( ; it.current(); ++it) {
    if(all_ || !(it.current()->flags() & BCAttribute::DontShow)) {
      strlist << it.current()->title();
    }
  }
  return strlist;
}

// can't be const since attributeByName() isn't
QStringList BCCollection::valuesByAttributeName(const QString& name_) {
  QStringList strlist;
  QListIterator<BCUnit> it(m_unitList);
  for( ; it.current(); ++it) {
    QString value = it.current()->attribute(name_);

    if(strlist.contains(value) == 0) { // haven't inserted it yet
      if(attributeByName(name_)->flags() & BCAttribute::AllowMultiple) {
        // careful, the semi-colon is used as a separator for things like multiple authors
        QStringList values = QStringList::split(";", value);
        // possible to have "; " or ";" so need to strip white space for each value
        QStringList::Iterator it2;
        for(it2 = values.begin(); it2 != values.end(); ++it2) {
          strlist << static_cast<QString>(*it2).simplifyWhiteSpace();
        }
      } else { // multiple values not allowed
        // no need to call value.simplifyWhiteSpace()
        strlist << value;
      }
    }

  } // end unit loop
  return strlist;
}

// can't be const since a pointer is returned
BCAttribute* BCCollection::attributeByName(const QString& name_) {
  QListIterator<BCAttribute> it(m_attributeList);
  for( ; it.current(); ++it) {
    if(it.current()->name() == name_) {
      return it.current();
    }
  }
  return 0;
}

// can't be const since attributeByName() is not const
bool BCCollection::allowed(const QString& key_, const QString& value_) {
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

BCAttribute* BCCollection::groupAttribute() {
  return m_groupAttribute;
}

void BCCollection::setGroupAttribute(BCAttribute* att_) {
  if(att_ != groupAttribute()) {
    m_isCustom = true;
  }
  m_groupAttribute = att_;
}

const QList<BCUnit>& BCCollection::unitList() const {
  return m_unitList;
}

BCCollection* BCCollection::Books(int id_) {
  BCCollection* coll = new BCCollection(id_, i18n("My Books"), "book", i18n("Book"), "book");

  BCAttribute* att;

  att = new BCAttribute("subtitle", i18n("Subtitle"));
  att->setGroup(i18n("General"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::FormatName);
  coll->addAttribute(att);

  att = new BCAttribute("author", i18n("Author"));
  att->setGroup(i18n("General"));
  att->setFlags(BCAttribute::AllowMultiple | BCAttribute::FormatName | BCAttribute::AllowGrouped);
  coll->addAttribute(att);
  coll->setGroupAttribute(att);

  QStringList binding;
  binding << i18n("Hardback") << i18n("Paperback") << i18n("Trade Paperback") << i18n("E-Book");
  att = new BCAttribute("binding", i18n("Binding"), binding);
  att->setGroup(i18n("General"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("pur_date", i18n("Purchase Date"));
  att->setGroup(i18n("General"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::DontComplete | BCAttribute::FormatDate);
  coll->addAttribute(att);

  att = new BCAttribute("pur_price", i18n("Purchase Price"));
  att->setGroup(i18n("General"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::DontComplete);
  coll->addAttribute(att);

  att = new BCAttribute("publisher", i18n("Publisher"));
  att->setGroup(i18n("Publishing"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("edition", i18n("Edition"));
  att->setGroup(i18n("Publishing"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::DontComplete | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("cr_year", i18n("Copyright Year"));
  att->setGroup(i18n("Publishing"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::DontComplete | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("pub_year", i18n("Publication Year"));
  att->setGroup(i18n("Publishing"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::DontComplete | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("isbn", i18n("ISBN#"));
  att->setGroup(i18n("Publishing"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::DontComplete);
  att->setDescription(i18n("International Standard Book Number"));
  coll->addAttribute(att);

  att = new BCAttribute("lccn", i18n("LCCN#"));
  att->setGroup(i18n("Publishing"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::DontComplete);
  att->setDescription(i18n("Library of Congress Control Number"));
  coll->addAttribute(att);

  att = new BCAttribute("pages", i18n("Pages"));
  att->setGroup(i18n("Publishing"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::DontComplete);
  coll->addAttribute(att);

  att = new BCAttribute("language", i18n("Language"));
  att->setGroup(i18n("Publishing"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("genre", i18n("Genre"));
  att->setGroup(i18n("Classification"));
  att->setFlags(BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("keywords", i18n("Keywords"));
  att->setGroup(i18n("Classification"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("series", i18n("Series"));
  att->setGroup(i18n("Classification"));
  att->setFlags(BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("series_num", i18n("Series Number"));
  att->setGroup(i18n("Classification"));
  att->setFlags(BCAttribute::DontComplete | BCAttribute::DontShow);
  coll->addAttribute(att);

  QStringList cond;
  cond << i18n("New") << i18n("Used");
  att = new BCAttribute("condition", i18n("Condition"), cond);
  att->setGroup(i18n("Classification"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("signed", i18n("Signed"), BCAttribute::Bool);
  att->setGroup(i18n("Personal"));
  att->setFlags(BCAttribute::DontShow);
  coll->addAttribute(att);

  att = new BCAttribute("read", i18n("Read"), BCAttribute::Bool);
  att->setGroup(i18n("Personal"));
  att->setFlags(BCAttribute::DontShow);
  coll->addAttribute(att);

  att = new BCAttribute("gift", i18n("Gift"), BCAttribute::Bool);
  att->setGroup(i18n("Personal"));
  att->setFlags(BCAttribute::DontShow);
  coll->addAttribute(att);

  att = new BCAttribute("loaned", i18n("Loaned"), BCAttribute::Bool);
  att->setGroup(i18n("Personal"));
  att->setFlags(BCAttribute::DontShow);
  coll->addAttribute(att);

  QStringList rating;
  rating << i18n("5 - Best") << i18n("4 - Good") << i18n("3 - Neutral") << i18n("2 - Bad") << i18n("1 - Worst");
  att = new BCAttribute("rating", i18n("Rating"), rating);
  att->setGroup(i18n("Personal"));
  att->setFlags(BCAttribute::DontShow | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("comments", i18n("Comments"));
  att->setGroup(i18n("Personal"));
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
  att->setGroup(i18n("General"));
  att->setFlags(BCAttribute::AllowGrouped);
  coll->addAttribute(att);
  coll->setGroupAttribute(att);

  att = new BCAttribute("year", i18n("Year"));
  att->setGroup(i18n("General"));
  att->setFlags(BCAttribute::DontComplete | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("genre", i18n("Genre"));
  att->setGroup(i18n("General"));
  att->setFlags(BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  QStringList list;
  list << i18n("CD") << i18n("Cassette") << i18n("8-track");
  att = new BCAttribute("medium", i18n("Medium"), list);
  att->setGroup(i18n("General"));
  att->setFlags(BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("comments", i18n("Comments"));
  att->setGroup(i18n("General"));
  att->setFlags(BCAttribute::DontComplete);
  coll->addAttribute(att);

  coll->m_isCustom = false;
  return coll;
}

BCCollection* BCCollection::Videos(int id_) {
  BCCollection* coll = new BCCollection(id_, i18n("My Videos"), "video", i18n("Video"), "video");

  BCAttribute* att;

  att = new BCAttribute("date", i18n("Date"));
  att->setGroup(i18n("General"));
  att->setFlags(BCAttribute::DontComplete);
  coll->addAttribute(att);

  att = new BCAttribute("genre", i18n("Genre"));
  att->setGroup(i18n("General"));
  att->setFlags(BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  coll->addAttribute(att);
  coll->setGroupAttribute(att);

  QStringList list;
  list << i18n("DVD") << i18n("VHS") << i18n("8mm") << i18n("VCD") << i18n("LaserDisc") << i18n("Beta");
  att = new BCAttribute("medium", i18n("Medium"), list);
  att->setGroup(i18n("General"));
  att->setFlags(BCAttribute::AllowGrouped);
  coll->addAttribute(att);

  att = new BCAttribute("comments", i18n("Comments"));
  att->setGroup(i18n("General"));
  att->setFlags(BCAttribute::DontComplete);
  coll->addAttribute(att);

  coll->m_isCustom = false;
  return coll;
}
#endif
