/***************************************************************************
                                  bcunit.h
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

#ifndef BCUNIT_H
#define BCUNIT_H

class BCCollection;
class BCUnit;
class BCUnitGroup;

#include "bcattribute.h"

#include <qstringlist.h>
#include <qmap.h>
#include <qstring.h>
#include <qptrlist.h>

typedef QPtrList<BCUnit> BCUnitList;
typedef QPtrListIterator<BCUnit> BCUnitListIterator;

/**
 * The BCUnit class represents a book, a CD, or whatever is the basic entity
 * in the collection.
 *
 * Each BCUnit object has a set of attribute values, such as title, artist, or format,
 * and must belong to a collection. A unique id number identifies each unit.
 *
 * @see BCAttribute
 *
 * @author Robby Stephenson
 * @version $Id: bcunit.h,v 1.5 2003/05/11 00:35:11 robby Exp $
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
   * The assignment operator is overloaded, since the id must be different.
   */
  BCUnit operator= (const BCUnit& unit);

  /**
   * Every unit has a title.
   */
  QString title() const;
  /**
   * Returns the value of the attribute with a given key name. If the key doesn't
   * exist, the method returns @ref QString::null.
   *
   * @param name The attribute name
   * @return The value of the attribute
   */
  QString attribute(const QString& name) const;
  /**
   * Returns the formatted value of the attribute with a given key name. If the
   * key doesn't exist, the method returns @ref QString::null. The value is cached,
   * so the first time the value is requested, @ref BCAttribute::format is called.
   * The second time, that lookup isn't necessary.
   *
   * @param name The attribute name
   * @param flag The attribute's format flag
   * @return The value of the attribute
   */
  QString attributeFormatted(const QString& name,
                             BCAttribute::FormatFlag flag = BCAttribute::FormatPlain) const;
  /**
   * Sets the value of an attribute for the unit. The method first verifies that
   * the value is allowed for that particular key.
   *
   * @param name The name of the attribute
   * @param value The value of the attribute
   * @return A boolean indicating whether or not the attribute was successfully set
   */
  bool setAttribute(const QString& name, const QString& value);
  /**
   * Returns a pointer to the parent collection of the unit.
   *
   * @return The collection pointer
   */
  BCCollection* const collection() const;
  /**
   * Returns the id of the unit
   *
   * @return The id
   */
  int id() const;
  /**
   * Adds the unit to a group. The group list within the unit is updated
   * and the unit is added to the group.
   *
   * @param group The group
   * @return a bool indicating if it was successfully added. If the unit was already
   * in the group, the return value is false
   */
  bool addToGroup(BCUnitGroup* group);
  /**
   * Removes the unit from a group. The group list within the unit is updated
   * and the unit is removed from the group.
   *
   * @param group The group
   * @return a bool indicating if the group was successfully removed
   */
  bool removeFromGroup(BCUnitGroup* group);
  /**
   * Returns a list of the groups to which the unit belongs
   *
   * @return The list of groups
   */
  const QPtrList<BCUnitGroup>& groups() const;
  /**
   * Returns a list containing the names of the groups for
   * a certain attribute to which the unit belongs
   *
   * @param attName The name of the attribute
   * @return The list of names
   */
  QStringList groupNamesByAttributeName(const QString& attName) const;
  /**
   * Returns a list of all the attribute values contained in the unit.
   *
   * @return The list of attribute values
   */
  QStringList attributeValues() const;
  /**
   * Returns a boolean indicating if the unit's parent collection recognizes
   * it existence, that is, the parent collection has this unit in its list.
   *
   * @return Whether the unit is ownder or not
   */
  bool isOwned() const;
  /**
   * Removes the formatted value of the attribute from the map. This should be used when
   * the attributes format flag has changed.
   */
  void invalidateFormattedAttributeValue(const QString& name);

private:
  QString m_title;
  int m_id;
  BCCollection* m_coll;
  QMap<QString, QString> m_attributes;
  mutable QMap<QString, QString> m_formattedAttributes;
  QPtrList<BCUnitGroup> m_groups;
};

#endif
