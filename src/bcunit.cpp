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
#include "bcattribute.h"

#include <kdebug.h>

#include <qregexp.h>

BCUnit::BCUnit(BCCollection* coll_) : m_id(coll_->unitCount()), m_coll(coll_) {
  // keep the title in the attributes, too.
  setAttribute(QString::fromLatin1("title"), m_title);
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
      return m_formattedAttributes.find(titleName).data();
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

QString BCUnit::attribute(const QString& attName_) const {
  if(attName_ == QString::fromLatin1("title")) {
    return m_title;
  }
  
  QString value;
  if(!m_attributes.isEmpty() && m_attributes.contains(attName_)) {
    value = m_attributes.find(attName_).data();
  }
  return value;
}

QString BCUnit::attributeFormatted(const QString& attName_, int flags_/*=0*/) const {
  // I should really have separated format flags
  // if none of the format flags are set, just return the attribute, maybe capitalized
  if( !(flags_ & BCAttribute::FormatTitle
         || flags_ & BCAttribute::FormatName
         || flags_ & BCAttribute::FormatDate) ) {
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
      value = BCAttribute::format(value, flags_);
      m_formattedAttributes.insert(attName_, value);
    }
  } else {
    value = m_formattedAttributes.find(attName_).data();
  }
  return value;
}

bool BCUnit::setAttribute(const QString& attName_, const QString& attValue_) {
  QString attValue = attValue_;
  // enforce rule to have a space after a semi-colon and a comma
  attValue.replace(QRegExp(QString::fromLatin1(";")), QString::fromLatin1("; "));
  attValue.replace(QRegExp(QString::fromLatin1(",")), QString::fromLatin1(", "));
  attValue = attValue.simplifyWhiteSpace();
  
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

  if(!m_coll->isAllowed(attName_, attValue)) {
    kdDebug() << "BCUnit::setAttribute() - value is not allowed - " << attValue << endl;
    return false;
  }

  m_attributes.insert(attName_, attValue);
  if(m_formattedAttributes.contains(attName_)) {
    m_formattedAttributes.remove(attName_);
  }
  return true;
}

BCCollection* const BCUnit::collection() const {
  return m_coll;
}

int BCUnit::id() const {
  return m_id;
}

bool BCUnit::addToGroup(BCUnitGroup* group_) {
  if(m_groups.containsRef(group_)) {
    return false;
  } else {
    m_groups.append(group_);
    group_->append(this);
//    kdDebug() << "BCUnit::addToGroup() - adding group (" << group_->groupName() << ")" << endl;
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
QStringList BCUnit::groupsByAttributeName(const QString& attName_) const {
//  kdDebug() << "BCUnit::groupsByAttributeName() - " << attName_ << endl;
  QStringList groups;
  BCAttribute* att = m_coll->attributeByName(attName_);
  int flags = att->flags();
  QString attValue = attributeFormatted(attName_, flags);
  if(attValue.isEmpty()) {
    groups += BCCollection::emptyGroupName();
  } else if(flags & BCAttribute::AllowMultiple) {
    // the space after the semi-colon is enforced elsewhere
    groups += QStringList::split(QString::fromLatin1("; "), attValue);
  } else {
    groups += attValue;
  }
  return groups;
}
