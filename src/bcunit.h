/* *************************************************************************
                          bcunit.h  -  description
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

#ifndef BCUNIT_H
#define BCUNIT_H

#include <qmap.h>
#include <qstring.h>

class BCCollection;

/**
 * The BCUnit class represents a book, a CD, or whatever is the basic entity in the collection.
 *
 * Each BCUnit object has a set of attribute values, such as title, artist, or format,
 * and must belong to a collection. A unique id number identifies each unit.
 *
 * @see BCAttribute
 *
 * @author Robby Stephenson
 * @version $Id: bcunit.h,v 1.7 2001/11/03 19:46:34 robby Exp $
 */
class BCUnit {
public:
  /**
   * The constructor, which automatically sets the id to the current number
   * of units in the collection.
   *
   * @param coll A pointer to the parent BCCollection object
   */
  BCUnit(BCCollection* coll);
  /**
   * The copy constructor, needed since the id must be different.
   */
  BCUnit(const BCUnit& unit);
  /**
   */
  ~BCUnit();

  /**
   * Returns the value of the attribute with a given key name. If the key doesn't
   * exist, the method returns QString::null.
   *
   * @param name The attribute name
   * @return The value of the attribute
   */
  QString attribute(const QString& name) const;
  /**
   * Sets the value of an attribute for the unit. The method first verifies that
   * the value is allowed for that particular key.
   *
   * @param name The key name of the attribute
   * @param value The value of the attribute
   * @return A boolean indicating whether or not the attribute was successfully set
   */
  bool setAttribute(const QString& name, const QString& value);
  /**
   * Returns a pointer to the parent collection of the unit.
   *
   * @return The collection pointer
   */
  BCCollection* collection();
  /**
   * Returns the id of the unit
   *
   * @return The id
   */
  int id() const;

private:
  int m_id;
  BCCollection* m_coll;
  QMap<QString, QString> m_attributes;
};

#endif
