/* *************************************************************************
                               bccollection.h
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

#ifndef BCCOLLECTION_H
#define BCCOLLECTION_H

#include "bcattribute.h"
#include "bcunitgroup.h"

#include <qstringlist.h>
#include <qptrlist.h>
#include <qstring.h>
#include <qdict.h>
#include <qobject.h>

typedef QDict<BCUnitGroup> BCUnitGroupDict;

/**
 * The BCCollection class is the primary data object, holding a list of attributes and units.
 *
 * A collection holds units of a single type, whether it be books, CDs, or whatever.
 * It has a list of attributes which apply for the whole collection, and is capable
 * of generating the XML required to represent all of its data. A unique id value
 * identifies each collection object.
 *
 * @see BCUnit
 * @see BCAttribute
 *
 * @author Robby Stephenson
 * @version $Id: bccollection.h,v 1.26 2002/11/25 00:56:22 robby Exp $
 */
class BCCollection : public QObject {
Q_OBJECT

public:
  /**
   * The constructor is only used to create custom collections. It adds a title attribute,
   * in the "General" group, and sets the unitGroupAttribute to a NULL pointer.
   *
   * @param id The id of the collection, which should be unique. The collection
   *            does not itself guarantee uniqueness.
   * @param title The title of the collection itself
   * @param unitName The name of the units that the collection holds (not translated)
   * @param unitTitle The title of the units, which can be translated
   * @param iconName The name of the icon used to represent the units in the collection
   */
  BCCollection(int id, const QString& title, const QString& unitName,
               const QString& unitTitle, const QString& iconName=QString("unknown"));
  /**
   */
  ~BCCollection();

  /**
   * Returns the id of the collection.
   *
   * @return The id
   */
  int id() const;
  /**
   * Returns the name of the collection.
   *
   * @return The name
   */
  const QString& title() const;
  /**
   * Sets the title of the collection.
   *
   * @param title The new collection title
   */
  void setTitle(const QString& title);
  /**
   * Returns the name of the units in the collection.
   *
   * @return The unit name
   */
  const QString& unitName() const;
  /**
   * Returns the title of the units in the collection.
   *
   * @return The unit title
   */
  const QString& unitTitle() const;
  /**
   * Returns the name of the icon used for the units in the collection.
   *
   * @return The icon name
   */
  const QString& iconName() const;
  /**
   * Sets the name of the icon used for units in the collection.
   *
   * @param title The new icon name
   */
  void setIconName(const QString& name);
  /**
   * Returns a reference to the list of all the units in the collection.
   *
   * @return The list of units
   */
  const BCUnitList& unitList() const;
  /**
   * Returns a reference to the list of the collection attributes.
   *
   * @return The list of attributes
   */
  const QPtrList<BCAttribute>& attributeList() const;
  /**
   * Returns a reference to the list of attribute groups. This value is cached rather
   * than generated with each call, so the method should be fairly fast.
   *
   * @return The list of group names
   */
  const QStringList& attributeCategories() const;
  /**
   * Returns the name of the attribute used to group the units by default.
   *
   * @return The attribute name
   */
  const QString& defaultGroupAttribute() const;
  /**
   * Sets the name of the default attribute used to group the units.
   *
   * @param name The name of the attribute
   */
  void setDefaultGroupAttribute(const QString& name);
  /**
   * Returns the number of units in the collection.
   *
   * @return The number of units
   */
  unsigned unitCount() const;
  /**
   * Adds a unit to the collection. The collection takes ownership of the unit object.
   *
   * @param unit A pointer to the unit
   */
  void addUnit(BCUnit* unit);
  /**
   * Updates the dicts that include the unit.
   *
   * @param unit A pointer to the unit
   */
  void updateDicts(BCUnit* unit);
  /**
   * Deletes a unit from the collection.
   *
   * @param unit The pointer to the unit
   * @return A boolean indicating if the unit was in the collection and was deleted
   */
  bool deleteUnit(BCUnit* unit);
  /**
   * Adds an attribute to the collection, unless an attribute with that name
   * already exists. The collection takes ownership of the attribute object.
   *
   * @see BCAttribute
   *
   * @param att A pointer to the attribute
   * @return A boolean indicating if the attribute was added or not
   */
  bool addAttribute(BCAttribute* att);
  /**
   * Determines whether or not a certain value is allowed for an attribute.
   *
   * @param key The name of the attribute
   * @param value The desired value
   * @return A boolean indicating if the value is an allowed value for that attribute
   */
  bool isAllowed(const QString& key, const QString& value) const;
  /**
   * Returns a boolean indicating if the collection is a custom one, i.e.
   * different from a collection returned from one of the static methods
   *
   * @return A boolean indicating if the collection is custom
   */
  bool isCustom() const;
  /**
   * Returns a list of all the attribute names.
   *
   * @param all Whether all attributes should be included or only those which have the
   *            visible flag set
   * @return The list of names
   */
  QStringList attributeNames(bool all=true) const;
  /**
   * Returns a list of all the attribute titles.
   *
   * @param all Whether all attributes should be included or only those which have the
   *            visible flag set
   * @return The list of titles
   */
  QStringList attributeTitles(bool all=true) const;
  /**
   * Returns a list of the values of a given attribute for every unit
   * in the collection. The values in the list are not repeated. Attribute
   * values which contain ";" are split into separate values. Since this method
   * iterates over all the units, for large collections, it is expensive.
   *
   * @param name The name of the attribute
   * @return The list of values
   */
  QStringList valuesByAttributeName(const QString& name) const;
  /**
   * Returns a list of all the attributes in a given category.
   *
   * @param category The name of the category
   * @return The attribute list
   */
  QPtrList<BCAttribute> attributesByCategory(const QString& category) const;
  /**
   * Returns a pointer to an attribute given its name. If none is found, a NULL pointer
   * is returned.
   *
   * @param name The attribute name
   * @return The attribute pointer
   */
  BCAttribute* attributeByName(const QString& name) const;
  /**
   * Returns a list of all the possible unit groups. This value is cached rather
   * than generated with each call, so the method should be fairly fast.
   *
   * @return The list of groups
   */
  const QStringList& unitGroups() const;
  /**
   * Returns a pointer to a const dict of all the units grouped by
   * a certain attribute
   *
   * @param name The name of the attribute by which the units are grouped
   * @return The list of group names
   */
  BCUnitGroupDict* unitGroupDictByName(const QString& name) const;
  /**
   * A helper method to emit @ref signalGroupModified, since signals are not public
   * This typically gets called via a unit.
   *
   * @param group The group that was modified
   */
  void groupModified(BCUnitGroup* group);

  /**
   * Returns the string used for empty values.
   *
   * @return The empty string
   */
  static QString emptyGroupName();
  /**
   * A convenience function to return a pointer a the standard book collection.
   * It has the following attributes:
   * @li Title
   * @li Subtitle
   * @li Author
   * @li Binding
   * @li Purchase Date
   * @li Purchase Price
   * @li Publisher
   * @li Edition
   * @li Copyright Year
   * @li Publication Year
   * @li ISBN Number
   * @li Library of Congress Catalog Number
   * @li Pages
   * @li Language
   * @li Genre
   * @li Keywords
   * @li Series
   * @li Series Number
   * @li Condition
   * @li Signed
   * @li Read
   * @li Gift
   * @li Loaned
   * @li Rating
   * @li Comments
   *
   * @param id The id for the collection
   */
  static BCCollection* Books(int id);
#if 0
  /**
   * A convenience function to return a pointer to a standard CD collection.
   * It has the following attributes:
   * @li Title
   * @li Artist
   * @li Year
   * @li Genre
   * @li Comments
   *
   * @param id The id for the collection
   */
  static BCCollection* CDs(int id);
  /**
   * A convenience function to return a pointer to a standard video collection.
   * It has the following attributes:
   * @li Title
   * @li Year
   * @li Medium
   * @li Comments
   *
   * @param id The id for the collection
   */
  static BCCollection* Videos(int id);
#endif

signals:
  void signalGroupModified(BCCollection* coll, BCUnitGroup* group);
  
private:
  /**
   * The copy constructor is private, to ensure that it's never used.
   */
  BCCollection(const BCCollection& coll);
  /**
   * The assignment operator is private, to ensure that it's never used.
   */
  BCCollection operator=(const BCCollection& coll);
  void removeUnitFromDicts(BCUnit* unit);
  void populateDicts(BCUnit* unit);

  int m_id;
  QString m_title;
  QString m_unitName;
  QString m_unitTitle;
  QString m_iconName;
  QString m_defaultGroupAttribute;
  bool m_isCustom;
  
  QPtrList<BCAttribute> m_attributeList;
  QDict<BCAttribute> m_attributeDict;
  QStringList m_attributeCategories;
  
  BCUnitList m_unitList;
  
  QDict<BCUnitGroupDict> m_unitGroupDicts;
  QStringList m_unitGroups;
};

#endif
