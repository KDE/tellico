/***************************************************************************
                              bibtexattribute.h
                             -------------------
    begin                : Sun Sep 21 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BIBTEXATTRIBUTE_H
#define BIBTEXATTRIBUTE_H

#include "../bcattribute.h"

class BibtexAttribute;
typedef QPtrList<BibtexAttribute> BibtexAttributeList;
typedef QPtrListIterator<BibtexAttribute> BibtexAttributeListIterator;

/**
 * The BibtexAttribute extends @ref BCAttribute to add a bibtexFieldName property. This
 * class will be used primarily in a @ref BibtexCollection.
 *
 * @author Robby Stephenson
 * @version $Id: bibtexattribute.h 219 2003-10-24 03:04:04Z robby $
 */
class BibtexAttribute : public BCAttribute {
public: 
  BibtexAttribute(const QString& name, const QString& title, AttributeType type = Line)
    : BCAttribute(name, title, type) {}
  BibtexAttribute(const QString& name, const QString& title, const QStringList& allowed)
    : BCAttribute(name, title, allowed) {}
  BibtexAttribute(const BCAttribute& att)
    : BCAttribute(att) {}

  virtual ~BibtexAttribute() {}
  // default copy constructor is ok, right?
  virtual BCAttribute* clone() { return new BibtexAttribute(*this); }

  virtual bool isBibtexAttribute() const { return true; }
  void setBibtexFieldName(const QString& fieldName) { m_bibtexFieldName = fieldName; }
  const QString& bibtexFieldName() const { return m_bibtexFieldName; }

private:
  QString m_bibtexFieldName;
};

#endif
