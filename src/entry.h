/***************************************************************************
    Copyright (C) 2001-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_ENTRY_H
#define TELLICO_ENTRY_H

#include "datavectors.h"
#include "fieldformat.h"

#include <QStringList>
#include <QHash>

namespace Tellico {

  namespace Data {
    class Collection;
    class EntryGroup;

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
class Entry : public QSharedData {

public:
  /**
   * The constructor, which automatically sets the id to the current number
   * of entries in the collection.
   *
   * @param coll A pointer to the parent collection object
   */
  Entry(CollPtr coll);
  Entry(CollPtr coll, ID id);
  /**
   * The copy constructor, needed since the id must be different.
   */
  Entry(const Entry& entry);
  /**
   * The assignment operator is overloaded, since the id must be different.
   */
  Entry& operator=(const Entry& other);

  ~Entry();

  /**
   * Every entry has a title.
   *
   * @return The entry title
   */
  QString title(FieldFormat::Request formatted = FieldFormat::DefaultFormat) const;
  /**
   * Returns the value of the field with a given key name.
   *
   * @param fieldName The field name
   * @return The value of the field
   */
  QString field(const QString& fieldName) const;
  QString field(Data::FieldPtr field) const;
  /**
   * Returns the formatted value of the field with a given key name.
   *
   * @param field The field
   * @return The formatted value of the field
   */
  QString formattedField(const QString& fieldName,
                         FieldFormat::Request formatted = FieldFormat::DefaultFormat) const;
  QString formattedField(Data::FieldPtr field,
                         FieldFormat::Request formatted = FieldFormat::DefaultFormat) const;
  /**
   * Sets the value of an field for the entry. The method first verifies that
   * the value is allowed for that particular key.
   *
   * @param fieldName The name of the field
   * @param value The value of the field
   * @param updateMDate Whether to update the modified date of the entry
   * @return A boolean indicating whether or not the field was successfully set
   */
  bool setField(const QString& fieldName, const QString& value, bool updateMDate=true);
  bool setField(Data::FieldPtr field, const QString& value, bool updateMDate=true);
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
  ID id() const { return m_id; }
  void setId(ID id) { m_id = id; }
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
  const QList<EntryGroup*>& groups() const { return m_groups; }
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
  QStringList fieldValues() const { return m_fieldValues.values(); }
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
   * @param name The name of the field that changed. an empty string means invalidate all fields.
   */
  void invalidateFormattedFieldValue(const QString& name=QString());

private:
  // not used
  Entry();

  bool operator==(const Entry& other) const;

  bool setFieldImpl(Data::FieldPtr field, const QString& value);

  CollPtr m_coll;
  ID m_id;
  QHash<QString, QString> m_fieldValues;
  mutable QHash<QString, QString> m_formattedFields;
  QList<EntryGroup*> m_groups;
};

class EntryCmp {

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

Q_DECLARE_METATYPE(Tellico::Data::EntryPtr)

#endif
