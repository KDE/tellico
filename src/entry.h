/***************************************************************************
    copyright            : (C) 2001-2004 by Robby Stephenson
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

#include "field.h"

#include <qstringlist.h>
#include <qmap.h>
#include <qstring.h>
#include <qptrlist.h>

namespace Bookcase {
  namespace Data {
    class Entry;
    class Collection;

    typedef QPtrList<Entry> EntryList;
    typedef QPtrListIterator<Entry> EntryListIterator;

/**
 * The EntryGroup is simply a QPtrList of entries which knows the name of its group,
 * and the name of the field to which that group belongs.
 *
 * An example for a book collection would be a group of books, all written by
 * David Weber. The @ref groupName() would be "Weber, David" and the
 * @ref fieldName() would be "author".
 *
 * @author Robby Stephenson
 * @version $Id: entry.h 386 2004-01-24 05:12:28Z robby $
 */
class EntryGroup : public EntryList {

public:
  EntryGroup(const QString& group, const QString& field)
   : EntryList(), m_group(group), m_field(field) {}
  EntryGroup(const EntryGroup& group)
   : EntryList(group), m_group(group.groupName()), m_field(group.fieldName()) {}
  ~EntryGroup() {}

  const QString& groupName() const { return m_group; }
  const QString& fieldName() const { return m_field; }

private:
  QString m_group;
  QString m_field;
};

/**
 * The Entry class represents a book, a CD, or whatever is the basic entity
 * in the collection.
 *
 * Each Entry object has a set of field values, such as title, artist, or format,
 * and must belong to a collection. A unique id number identifies each unit.
 *
 * @see Field
 *
 * @author Robby Stephenson
 * @version $Id: entry.h 386 2004-01-24 05:12:28Z robby $
 */
class Entry {
  // two entries are equal if all their field values are equal
  friend bool operator==(const Entry& e1, const Entry& e2) {
    bool match = (e1.m_fields.count() == e2.m_fields.count());
    if(!match) {
      return false;
    }
    for(QMap<QString, QString>::ConstIterator it = e1.m_fields.begin(); it != e1.m_fields.end(); ++it) {
      if(!e2.m_fields.contains(it.key()) || e2.m_fields[it.key()] != it.data()) {
        return false;
      }
    }
    return true;
  }

public:
  /**
   * The constructor, which automatically sets the id to the current number
   * of units in the collection.
   *
   * @param coll A pointer to the parent collection object
   */
  Entry(Collection* coll);
  /**
   * The copy constructor, needed since the id must be different.
   */
  Entry(const Entry& unit);
  /**
   * If a copy of the unit is needed in a new collection, needs another pointer.
   */
  Entry(const Entry& unit, Collection* coll);
  /**
   * The assignment operator is overloaded, since the id must be different.
   */
  Entry operator= (const Entry& entry) { return Entry(entry); }

  /**
   * Every entry has a title.
   *
   * @return The entry title
   */
  QString title() const;
  /**
   * Returns the value of the field with a given key name. If the key doesn't
   * exist, the method returns @ref QString::null.
   *
   * @param name The field name
   * @return The value of the field
   */
  QString field(const QString& name) const;
  /**
   * Returns the formatted value of the field with a given key name. If the
   * key doesn't exist, the method returns @ref QString::null. The value is cached,
   * so the first time the value is requested, @ref Field::format is called.
   * The second time, that lookup isn't necessary.
   *
   * @param name The field name
   * @return The formatted value of the field
   */
  QString formattedField(const QString& name) const;
  /**
   * Splits a field value. This is faster than calling Data::Field::split() since
   * a regexp is not used, only a string.
   *
   * @param field The field name
   * @param format Whether to format the values or not
   * @return The list of field values
   */
  QStringList fields(const QString& field, bool format) const;
  /**
   * Sets the value of an field for the unit. The method first verifies that
   * the value is allowed for that particular key.
   *
   * @param name The name of the field
   * @param value The value of the field
   * @return A boolean indicating whether or not the field was successfully set
   */
  bool setField(const QString& name, const QString& value);
  /**
   * Returns a pointer to the parent collection of the unit.
   *
   * @return The collection pointer
   */
  Collection* const collection() const { return m_coll; }
  /**
   * Returns the id of the unit
   *
   * @return The id
   */
  int id() const { return m_id; }
  /**
   * Adds the unit to a group. The group list within the unit is updated
   * and the unit is added to the group.
   *
   * @param group The group
   * @return a bool indicating if it was successfully added. If the unit was already
   * in the group, the return value is false
   */
  bool addToGroup(EntryGroup* group);
  /**
   * Removes the unit from a group. The group list within the unit is updated
   * and the unit is removed from the group.
   *
   * @param group The group
   * @return a bool indicating if the group was successfully removed
   */
  bool removeFromGroup(EntryGroup* group);
  /**
   * Returns a list of the groups to which the unit belongs
   *
   * @return The list of groups
   */
  const QPtrList<EntryGroup>& groups() const { return m_groups; }
  /**
   * Returns a list containing the names of the groups for
   * a certain field to which the unit belongs
   *
   * @param fieldName The name of the field
   * @return The list of names
   */
  QStringList groupNamesByFieldName(const QString& fieldName) const;
  /**
   * Returns a list of all the field values contained in the unit.
   *
   * @return The list of field values
   */
  QStringList fieldValues() const;
  /**
   * Returns a boolean indicating if the unit's parent collection recognizes
   * it existence, that is, the parent collection has this unit in its list.
   *
   * @return Whether the unit is owned or not
   */
  bool isOwned() const;
  /**
   * Removes the formatted value of the field from the map. This should be used when
   * the attributes format flag has changed.
   */
  void invalidateFormattedFieldValue(const QString& name);

protected:
  /**
   * Construct the derived valued for an field. The format string should be
   * of the form "%{name1} %{name2}" where the names are replaced by the value
   * of that field for the unit. Whether or not formatting is done on the
   * strings themselves shoudl be taken into account.
   *
   * @param formatString The format string
   * @param autoFormat Whether the inserted values should be auto-formatted
   * @return The constructed field value
   */
  QString dependentValue(const QString& formatString, bool autoFormat) const;

private:
  Collection* m_coll;
  int m_id;
  StringMap m_fields;
  mutable StringMap m_formattedFields;
  QPtrList<EntryGroup> m_groups;
};

  } // end namespace
} // end namespace

#endif
