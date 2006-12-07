/***************************************************************************
    copyright            : (C) 2001-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_ENTRY_H
#define TELLICO_ENTRY_H

#include "datavectors.h"

#include <ksharedptr.h>

#include <qstringlist.h>
#include <qstring.h>
#include <qptrlist.h>
#include <qobject.h>

#include <functional>

namespace Tellico {
  namespace Data {
    class Collection;

/**
 * The EntryGroup is simply a vector of entries which knows the name of its group,
 * and the name of the field to which that group belongs.
 *
 * An example for a book collection would be a group of books, all written by
 * David Weber. The @ref groupName() would be "Weber, David" and the
 * @ref fieldName() would be "author".
 *
 * It's a QObject because EntryGroupItem holds a QGuardedPtr
 *
 * @author Robby Stephenson
 */
class EntryGroup : public QObject, public EntryVec {
Q_OBJECT

public:
  EntryGroup(const QString& group, const QString& field);
  ~EntryGroup();

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
 * and must belong to a collection. A unique id number identifies each entry.
 *
 * @see Field
 *
 * @author Robby Stephenson
 */
class Entry : public KShared {

public:
  /**
   * The constructor, which automatically sets the id to the current number
   * of entries in the collection.
   *
   * @param coll A pointer to the parent collection object
   */
  Entry(CollPtr coll);
  Entry(CollPtr coll, int id);
  /**
   * The copy constructor, needed since the id must be different.
   */
  Entry(const Entry& entry);
  /**
   * The assignment operator is overloaded, since the id must be different.
   */
  Entry& operator=(const Entry& other);
  /**
   * two entries are equal if all their field values are equal, except for
   * file catalogs which match on the url only
   */
  bool operator==(const Entry& other);

  ~Entry();

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
   * @param fieldName The field name
   * @param formatted Whether to format the field or not.
   * @return The value of the field
   */
  QString field(const QString& fieldName, bool formatted=false) const;
  QString field(Data::FieldPtr field, bool formatted=false) const;
  /**
   * Returns the formatted value of the field with a given key name. If the
   * key doesn't exist, the method returns @ref QString::null. The value is cached,
   * so the first time the value is requested, @ref Field::format is called.
   * The second time, that lookup isn't necessary.
   *
   * @param fieldName The field name
   * @return The formatted value of the field
   */
  QString formattedField(const QString& fieldName) const;
  QString formattedField(Data::FieldPtr field) const;
  /**
   * Splits a field value. This is faster than calling Data::Field::split() since
   * a regexp is not used, only a string.
   *
   * @param field The field name
   * @param format Whether to format the values or not
   * @return The list of field values
   */
  QStringList fields(const QString& fieldName, bool formatted) const;
  QStringList fields(Data::FieldPtr field, bool formatted) const;
  /**
   * Sets the value of an field for the entry. The method first verifies that
   * the value is allowed for that particular key.
   *
   * @param fieldName The name of the field
   * @param value The value of the field
   * @return A boolean indicating whether or not the field was successfully set
   */
  bool setField(const QString& fieldName, const QString& value);
  bool setField(Data::FieldPtr field, const QString& value);
  /**
   * Returns a pointer to the parent collection of the entry.
   *
   * @return The collection pointer
   */
  CollPtr collection() const;
  /**
   * Changes the collection owner of the entry
   */
  void setCollection(CollPtr coll);
  /**
   * Returns the id of the entry
   *
   * @return The id
   */
  long id() const { return m_id; }
  void setId(long id) { m_id = id; }
  /**
   * Adds the entry to a group. The group list within the entry is updated
   * and the entry is added to the group.
   *
   * @param group The group
   * @return a bool indicating if it was successfully added. If the entry was already
   * in the group, the return value is false
   */
  bool addToGroup(EntryGroup* group);
  /**
   * Removes the entry from a group. The group list within the entry is updated
   * and the entry is removed from the group.
   *
   * @param group The group
   * @return a bool indicating if the group was successfully removed
   */
  bool removeFromGroup(EntryGroup* group);
  void clearGroups();
  /**
   * Returns a list of the groups to which the entry belongs
   *
   * @return The list of groups
   */
  const PtrVector<EntryGroup>& groups() const { return m_groups; }
  /**
   * Returns a list containing the names of the groups for
   * a certain field to which the entry belongs
   *
   * @param fieldName The name of the field
   * @return The list of names
   */
  QStringList groupNamesByFieldName(const QString& fieldName) const;
  /**
   * Returns a list of all the field values contained in the entry.
   *
   * @return The list of field values
   */
  QStringList fieldValues() const { return m_fields.values(); }
  /**
   * Returns a list of all the formatted field values contained in the entry.
   *
   * @return The list of field values
   */
  QStringList formattedFieldValues() const { return m_formattedFields.values(); }
  /**
   * Returns a boolean indicating if the entry's parent collection recognizes
   * it existence, that is, the parent collection has this entry in its list.
   *
   * @return Whether the entry is owned or not
   */
  bool isOwned();
  /**
   * Removes the formatted value of the field from the map. This should be used when
   * the field's format flag has changed.
   *
   * @param name The name of the field that changed. QString::null means invalidate all fields.
   */
  void invalidateFormattedFieldValue(const QString& name=QString::null);

  static int compareValues(EntryPtr entry1, EntryPtr entry2, FieldPtr field);
  static int compareValues(EntryPtr entry1, EntryPtr entry2, const QString& field, ConstCollPtr coll);

  /**
   * Construct the derived valued for an field. The format string should be
   * of the form "%{name1} %{name2}" where the names are replaced by the value
   * of that field for the entry. Whether or not formatting is done on the
   * strings themselves shoudl be taken into account.
   *
   * @param formatString The format string
   * @param autoCapitalize Whether the inserted values should be auto-capitalized. They're never formatted.
   * @return The constructed field value
   */
  static QString dependentValue(ConstEntryPtr e, const QString& formatString, bool autoCapitalize);

private:
  CollPtr m_coll;
  long m_id;
  StringMap m_fields;
  mutable StringMap m_formattedFields;
  PtrVector<EntryGroup> m_groups;
};

class EntryCmp : public std::binary_function<EntryPtr, EntryPtr, bool> {

public:
  EntryCmp(const QString& field) : m_field(field) {}

  bool operator()(EntryPtr e1, EntryPtr e2) const {
    return e1->field(m_field) < e2->field(m_field);
  }

private:
  QString m_field;
};

  } // end namespace
} // end namespace

#endif
