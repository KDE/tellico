/***************************************************************************
                                 bcunit.cpp
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

#include "bcunit.h"
#include "bccollection.h"
#include "bcunitgroup.h"

#include <kdebug.h>

#include <qregexp.h>

BCUnit::BCUnit(BCCollection* coll_) : m_title(QString::null), m_id(coll_->unitCount()), m_coll(coll_) {
  // keep the title in the attributes, too.
  setAttribute(QString::fromLatin1("title"), m_title);

  if(!coll_) {
    kdWarning() << "BCUnit() - null collection pointer!" << endl;
  }
}

BCUnit::BCUnit(const BCUnit& unit_) : m_title(unit_.m_title),
    m_id(unit_.m_coll->unitCount()), m_coll(unit_.m_coll),
    m_attributes(unit_.m_attributes) {
}

BCUnit BCUnit::operator= (const BCUnit& unit_) {
  return BCUnit(unit_);
}

QString BCUnit::title() const {
  if(BCAttribute::autoFormat()) {
    QString titleName = QString::fromLatin1("title");
    if(m_formattedAttributes.contains(titleName)) {
      return m_formattedAttributes[titleName];
    } else {
      QString title = BCAttribute::formatTitle(m_title);
      m_formattedAttributes.insert(titleName, title);
      return title;
    }
  } else if(BCAttribute::autoCapitalize()) {
    return BCAttribute::capitalize(m_title);
  } else {
    return m_title;
  }
}

const QString& BCUnit::attribute(const QString& attName_) const {
  if(attName_ == QString::fromLatin1("title")) {
    return m_title;
  }
  
 // from Qt help:
 // QMap::operator[] returns the value associated with the key k.
 // If no such key is present, a reference to an empty item is returned.
//  if(!m_attributes.isEmpty() && m_attributes.contains(attName_)) {
  if(!m_attributes.isEmpty()) {
    return m_attributes[attName_];
  }
  return QString::null;
}

QString BCUnit::attributeFormatted(const QString& attName_, BCAttribute::FormatFlag flag_/*=FormatPlain*/) const {
  // if auto format is not set, then just return the value
  if(!BCAttribute::autoFormat()) {
    return attribute(attName_);
  }
  
  // if format is plain, just return the attribute, maybe capitalized
  if(flag_ == BCAttribute::FormatPlain) {
    if(BCAttribute::autoCapitalize()) {
      return BCAttribute::capitalize(attribute(attName_));
    } else {
      return attribute(attName_);
    }
  }
  
  QString value;
  if(m_formattedAttributes.isEmpty() || !m_formattedAttributes.contains(attName_)) {
    if(attName_ == QString::fromLatin1("title")) {
      value = m_title;
    } else {
      value = attribute(attName_);
    }
    if(!value.isEmpty()) {
      value = BCAttribute::format(value, flag_);
      m_formattedAttributes.insert(attName_, value);
    }
  } else {
    value = m_formattedAttributes[attName_];
  }
  return value;
}

bool BCUnit::setAttribute(const QString& attName_, const QString& attValue_) {
  // an empty value means remove the attribute
  if(attValue_.isEmpty()) {
    if(!m_attributes.isEmpty() && m_attributes.contains(attName_)) {
      m_attributes.remove(attName_);
    }
    invalidateFormattedAttributeValue(attName_);
    return true;
  }
    
  // the title is actually stored in two places
  // I started out with treating it as any other attribute
  // then decided it was special and gave it its own variable
  if(attName_ == QString::fromLatin1("title")) {
    m_title = attValue_;
  }

  if(m_coll->attributeList().count() == 0
      || m_coll->attributeNames().contains(attName_) == 0) {
    kdDebug() << "BCUnit::setAttribute() - unknown collection unit attribute - " << attName_ << endl;
    return false;
  }

  if(!m_coll->isAllowed(attName_, attValue_)) {
    kdDebug() << "BCUnit::setAttribute() - value is not allowed - " << attValue_ << endl;
    return false;
  }

  m_attributes.insert(attName_, attValue_);
  invalidateFormattedAttributeValue(attName_);
  return true;
}

BCCollection* const BCUnit::collection() const {
  return m_coll;
}

int BCUnit::id() const {
  return m_id;
}

bool BCUnit::addToGroup(BCUnitGroup* group_) {
  if(!group_ || m_groups.containsRef(group_)) {
    return false;
  } else {
//    kdDebug() << "BCUnit::addToGroup() - adding group (" << group_->groupName() << ")" << endl;
    m_groups.append(group_);
    group_->append(this);
    m_coll->groupModified(group_);
    return true;
  }
}

bool BCUnit::removeFromGroup(BCUnitGroup* group_) {
  // if the removal isn't successful, just return
  bool success = m_groups.removeRef(group_);
  success = success && group_->removeRef(this);
//  kdDebug() << "BCUnit::removeFromGroup() - removing from group - " << group_->groupName() << endl;
  if(success) {
    m_coll->groupModified(group_);
    // don't delete until the signal is emitted
    if(group_->isEmpty()) {
//      kdDebug() << "BCUnit::removeFromGroup() - deleting group (" << group_->groupName() << ")" << endl;
      delete group_;
    }
  } else {
    kdDebug() << "BCUnit::removeFromGroup() failed! " << endl;
  }
  return success;
}

const QPtrList<BCUnitGroup>& BCUnit::groups() const {
  return m_groups;
}

// this function gets called before m_groups is updated. In fact, it is used to
// update that list. This is the function that actually parses the attribute values
// and returns the list of the group names.
QStringList BCUnit::groupNamesByAttributeName(const QString& attName_) const {
//  kdDebug() << "BCUnit::groupsByAttributeName() - " << attName_ << endl;
  QStringList groups;
  BCAttribute* att = m_coll->attributeByName(attName_);
  
  QString attValue = attributeFormatted(attName_, att->formatFlag());
  if(attValue.isEmpty()) {
    groups += BCCollection::emptyGroupName();
    // attributeByName() had better now return 0
  } else if(att->flags() & BCAttribute::AllowMultiple) {
    groups += QStringList::split(QRegExp(QString::fromLatin1(";\\s*")), attValue);
  } else {
    groups += attValue;
  }
  return groups;
}

QStringList BCUnit::attributeValues() const {
  return m_attributes.values();
}

bool BCUnit::isOwned() const {
  return (m_coll && m_coll->unitList().count() > 0 && m_coll->unitList().containsRef(this) > 0);
}

void BCUnit::invalidateFormattedAttributeValue(const QString& name_) {
  if(!m_formattedAttributes.isEmpty() && m_formattedAttributes.contains(name_)) {
    m_formattedAttributes.remove(name_);
  } 
}
