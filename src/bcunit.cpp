/* *************************************************************************
                          bcunit.cpp  -  description
                             -------------------
    begin                : Sat Sep 15 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@radiojodi.com
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
#include <kdebug.h>
#include <qstringlist.h>
#include <qstring.h>

BCUnit::BCUnit(BCCollection* coll_) : m_coll(coll_) {
  m_id = m_coll->unitCount();
}

BCUnit::BCUnit(const BCUnit& unit_) {
  m_coll = unit_.m_coll;
  m_id = m_coll->unitCount();
  m_attributes = unit_.m_attributes;
}

BCUnit::~BCUnit() {
  m_attributes.clear();
}

QString BCUnit::attribute(const QString& key_) const {
  QString value = QString::null;
  if(!m_attributes.isEmpty() && m_attributes.contains(key_)) {
    value = m_attributes.find(key_).data();
  }
  return value;
}

bool BCUnit::setAttribute(const QString& key_, const QString& value_) {
  bool successful = false;
  if(m_coll->attributeNames().contains(key_)) {
    if(m_coll->allowed(key_, value_)) {
      m_attributes.insert(key_, value_);
      successful = true;
    } else {
      kdDebug() << QString("BCUnit::setAttribute() -- value (%1) not allowed for attribute (%2)").arg(value_).arg(key_) << endl;
    }
  } else {
    kdDebug() << QString("BCUnit::setAttribute() -- unknown collection unit attribute (%1)").arg(key_) << endl;
  }
  return successful;
}

BCCollection* BCUnit::collection() {
  return m_coll;
}

int BCUnit::id() const {
  return m_id;
}
