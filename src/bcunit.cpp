/* *************************************************************************
                                 bcunit.cpp
                             -------------------
    begin                : Sat Sep 15 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@periapsis.org
 * *************************************************************************/

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 * *************************************************************************/

#include "bcunit.h"
#include "bccollection.h"
#include "bcunitgroup.h"

#include <kdebug.h>

#include <qstringlist.h>
#include <qstring.h>
#include <qregexp.h>

BCUnit::BCUnit(BCCollection* coll_) : m_id(coll_->unitCount()), m_coll(coll_) {
}

BCUnit::BCUnit(const BCUnit& unit_) : m_id(unit_.m_coll->unitCount()),
    m_coll(unit_.m_coll), m_attributes(unit_.m_attributes){
}

BCUnit BCUnit::operator= (const BCUnit& unit_) {
  return BCUnit(unit_);
}

QString BCUnit::attribute(const QString& attName_) const {
  QString value = QString::null;
  if(!m_attributes.isEmpty() && m_attributes.contains(attName_)) {
    value = m_attributes.find(attName_).data();
  }
  return value;
}

// not const since the m_attributesF might be modified
QString BCUnit::attributeFormatted(const QString& attName_, int flags_/*=0*/) {
  if(flags_ == 0) {
    return attribute(attName_);
  }
  QString value = QString::null;
  if(m_attributesF.isEmpty() || !m_attributesF.contains(attName_)) {
    value = attribute(attName_);
    BCAttribute::format(value, flags_);
    m_attributesF.insert(attName_, value);
  } else {
    value = m_attributesF.find(attName_).data();
  }
  return value;
}

bool BCUnit::setAttribute(const QString& attName_, const QString& attValue_) {
  QString attValue = attValue_;
  // enforce rule to have a space after a semi-colon and a comma
  attValue.replace(QRegExp(";"), "; ");
  attValue.replace(QRegExp(","), ", ");
  attValue = attValue.simplifyWhiteSpace();

  if(m_coll->attributeList().count() == 0
      || m_coll->attributeNames().contains(attName_) == 0) {
    kdDebug() << QString("BCUnit::setAttribute() - unknown collection unit attribute (%1)").arg(attName_) << endl;
    return false;
  }

  if(!m_coll->isAllowed(attName_, attValue)) {
    kdDebug() << QString("BCUnit::setAttribute() - value (%1) not allowed for attribute (%2)").arg(attValue).arg(attName_) << endl;
    return false;
  }

  m_attributes.insert(attName_, attValue);
  if(m_attributesF.contains(attName_)) {
    m_attributesF.remove(attName_);
  }
  return true;
}

BCCollection* BCUnit::collection() const {
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
  m_groups.removeRef(group_);
  // if the removal isn't successful, just return
  bool success = group_->removeRef(this);
//  kdDebug() << "BCUnit::removeFromGroup() - removing from group - " << group_->groupName() << endl;
  m_coll->groupModified(group_);
  // don't delete until the signal is emitted
  if(group_->isEmpty()) {
//    kdDebug() << "BCUnit::removeFromGroup() - deleting group (" << group_->groupName() << ")" << endl;
    delete group_;
  }
  return success;
}

const QPtrList<BCUnitGroup>& BCUnit::groups() const {
  return m_groups;
}

// this function gets called before m_groups is updated. In fact, it is used to
// update that list. This is the function that actually parses the attribute values
// and returns the list of the group names.
// Can't be const since attributeFormatted isn't const
QStringList BCUnit::groupsByAttributeName(const QString& attName_) {
//  kdDebug() << "BCUnit::groupsByAttributeName() - " << attName_ << endl;
  QStringList groups;
  BCAttribute* att = m_coll->attributeByName(attName_);
  int flags = att->flags();
  QString attValue = attributeFormatted(attName_, flags);
  if(attValue.isEmpty()) {
    groups += BCCollection::emptyGroupName();
  } else if(flags & BCAttribute::AllowMultiple) {
    // the space after the semi-colon is enforced elsewhere
    groups += QStringList::split("; ", attValue);
  } else {
    groups += attValue;
  }
  return groups;
}
