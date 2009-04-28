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

#ifndef TELLICO_COLLECTION_H
#define TELLICO_COLLECTION_H

#include "field.h"
#include "entry.h"
#include "filter.h"
#include "borrower.h"
#include "datavectors.h"

#include <ksharedptr.h>

#include <QStringList>
#include <QHash>
#include <QObject>

namespace Tellico {
  namespace Data {
    class EntryGroup;
    typedef QHash<QString, EntryGroup*> EntryGroupDict;

/**
 * The Collection class is the primary data object, holding a
 * list of fields and entries.
 *
 * A collection holds entries of a single type, whether it be books, CDs, or whatever.
 * It has a list of attributes which apply for the whole collection. A unique id value
 * identifies each collection object.
 *
 * @see Entry
 * @see Field
 *
 * @author Robby Stephenson
 */
class Collection : public QObject, public QSharedData {
Q_OBJECT

public:
  enum Type {
    Base = 1,
    Book = 2,
    Video = 3,
    Album = 4,
    Bibtex = 5,
    ComicBook = 6,
    Wine = 7,
    Coin = 8,
    Stamp = 9,
    Card = 10,
    Game = 11,
    File = 12,
    BoardGame = 13
    // if you want to add custom collection types, use a number sure to be unique like 101
  };

  Collection(const QString& title);
  /**
   * The constructor is only used to create custom collections. It adds a title field,
   * in the "General" group.
   *
   * @param title The title of the collection itself
   */
  Collection(bool addDefaultFields, const QString& title=QString());
  /**
   */
  virtual ~Collection();

  /**
   * Returns the type of the collection.
   *
   * @return The type
   */
  virtual Type type() const { return Base; }
  /**
   * Returns the id of the collection.
   *
   * @return The id
   */
  long id() const { return m_id; }
  /**
   * Returns the name of the collection.
   *
   * @return The name
   */
  const QString& title() const { return m_title; }
  /**
   * Sets the title of the collection.
   *
   * @param title The new collection title
   */
  void setTitle(const QString& title) { m_title = title; }
  /**
   * Returns the name of the entries in the collection, e.g. "book".
   * Not translated.
   *
   * @return The type name
   */
  QString typeName() const;
  /**
   * Returns a reference to the list of all the entries in the collection.
   *
   * @return The list of entries
   */
  const EntryList& entries() const { return m_entries; }
  /**
   * Returns a reference to the list of the collection attributes.
   *
   * @return The list of fields
   */
  const FieldList& fields() const { return m_fields; }
  EntryPtr entryById(long id);
  /**
   * Returns a reference to the list of the collection's people fields.
   *
   * @return The list of fields
   */
  const FieldList& peopleFields() const { return m_peopleFields; }
  /**
   * Returns a reference to the list of the collection's image fields.
   *
   * @return The list of fields
   */
  const FieldList& imageFields() const { return m_imageFields; }
  /**
   * Returns a reference to the list of field groups. This value is cached rather
   * than generated with each call, so the method should be fairly fast.
   *
   * @return The list of group names
   */
  const QStringList& fieldCategories() const { return m_fieldCategories; }
  /**
   * Returns the name of the field used to group the entries by default.
   *
   * @return The field name
   */
  const QString& defaultGroupField() const { return m_defaultGroupField; }
  /**
   * Sets the name of the default field used to group the entries.
   *
   * @param name The name of the field
   */
  void setDefaultGroupField(const QString& name) { m_defaultGroupField = name; }
  /**
   * Returns the number of entries in the collection.
   *
   * @return The number of entries
   */
  int entryCount() const { return m_entries.count(); }
  /**
   * Adds a entry to the collection. The collection takes ownership of the entry object.
   *
   * @param entry A pointer to the entry
   */
  void addEntries(const EntryList& entries);
  void addEntries(EntryPtr entry) { addEntries(EntryList() << entry); }
  /**
   * Updates the dicts that include the entry.
   *
   * @param entry A pointer to the entry
   */
  void updateDicts(const EntryList& entries);
  /**
   * Deletes a entry from the collection.
   *
   * @param entry The pointer to the entry
   * @return A boolean indicating if the entry was in the collection and was deleted
   */
  bool removeEntries(const EntryList& entries);
  /**
   * Adds a whole list of attributes. It's gotta be virtual since it calls
   * @ref addField, which is virtual.
   *
   * @param list List of attributes to add
   * @return A boolean indicating if the field was added or not
   */
  virtual bool addFields(FieldList list);
  /**
   * Adds an field to the collection, unless an field with that name
   * already exists. The collection takes ownership of the field object.
   *
   * @param field A pointer to the field
   * @return A boolean indicating if the field was added or not
   */
  virtual bool addField(FieldPtr field);
  virtual bool mergeField(FieldPtr field);
  virtual bool modifyField(FieldPtr field);
  virtual bool removeField(FieldPtr field, bool force=false);
  virtual bool removeField(const QString& name, bool force=false);
  void reorderFields(const FieldList& list);

  // the reason this is not static is so I can call it from a collection pointer
  // it also gets virtualized for different collection types
  // the return values should be compared against the GOOD and PERFECT
  // static match constants in this class
  virtual int sameEntry(Data::EntryPtr, Data::EntryPtr) const;

  /**
   * Determines whether or not a certain value is allowed for an field.
   *
   * @param key The name of the field
   * @param value The desired value
   * @return A boolean indicating if the value is an allowed value for that field
   */
  bool isAllowed(const QString& key, const QString& value) const;
  /**
   * Returns a list of all the field names.
   *
   * @return The list of names
   */
  const QStringList& fieldNames() const { return m_fieldNames; }
  /**
   * Returns a list of all the field titles.
   *
   * @return The list of titles
   */
  const QStringList& fieldTitles() const { return m_fieldTitles; }
  /**
   * Returns the title of an field, given its name.
   *
   * @param name The field name
   * @return The field title
   */
  QString fieldTitleByName(const QString& name) const;
  /**
   * Returns the name of an field, given its title.
   *
   * @param title The field title
   * @return The field name
   */
  QString fieldNameByTitle(const QString& title) const;
  /**
   * Returns a list of the values of a given field for every entry
   * in the collection. The values in the list are not repeated. Attribute
   * values which contain ";" are split into separate values. Since this method
   * iterates over all the entries, for large collections, it is expensive.
   *
   * @param name The name of the field
   * @return The list of values
   */
  QStringList valuesByFieldName(const QString& name) const;
  /**
   * Returns a list of all the fields in a given category.
   *
   * @param category The name of the category
   * @return The field list
   */
  FieldList fieldsByCategory(const QString& category);
  /**
   * Returns a pointer to an field given its name. If none is found, a NULL pointer
   * is returned.
   *
   * @param name The field name
   * @return The field pointer
   */
  FieldPtr fieldByName(const QString& name) const;
  /**
   * Returns a pointer to an field given its title. If none is found, a NULL pointer
   * is returned. This lookup is slower than by name.
   *
   * @param title The field title
   * @return The field pointer
   */
  FieldPtr fieldByTitle(const QString& title) const;
  /**
   * Returns @p true if the collection contains a field named @ref name;
   */
  bool hasField(const QString& name) const;
  /**
   * Returns a list of all the possible entry groups. This value is cached rather
   * than generated with each call, so the method should be fairly fast.
   *
   * @return The list of groups
   */
  const QStringList& entryGroups() const { return m_entryGroups; }
  /**
   * Returns a pointer to a dict of all the entries grouped by
   * a certain field
   *
   * @param name The name of the field by which the entries are grouped
   * @return The list of group names
   */
  EntryGroupDict* entryGroupDictByName(const QString& name);
  /**
   * Invalidates all group names in the collection.
   */
  void invalidateGroups();
  /**
   * Returns true if the collection contains at least one Image field.
   *
   * @return Returns true if the collection contains at least one Image field;
   */
  bool hasImages() const { return !m_imageFields.isEmpty(); }

  void setTrackGroups(bool b) { m_trackGroups = b; }

  void addBorrower(Data::BorrowerPtr borrower);
  const BorrowerList& borrowers() const { return m_borrowers; }
  /**
   * Clears all vectors which contain shared ptrs
   */
  void clear();

  void addFilter(FilterPtr filter);
  bool removeFilter(FilterPtr filter);
  const FilterList& filters() const { return m_filters; }

  /**
   * Return a vector of all the fields on which a field depends.
   * Returns an empty vector for non-Dpendent fields
   */
  FieldList fieldDependsOn(FieldPtr field) const;
  /**
   * Prepare text for formatting
   *
   * Useful only for BibtexCollection to strip bibtex strings
   */
  virtual QString prepareText(const QString& text) const;

  /**
   * The string used for empty values. This forces consistency.
   */
  static const char* s_emptyGroupTitle;
  /**
   * The string used for the people pseudo-group. This forces consistency.
   */
  static const QString s_peopleGroupName;

  // these are the values that should be compared against
  // the result from sameEntry()
  static const int ENTRY_GOOD_MATCH = 10;
  static const int ENTRY_PERFECT_MATCH = 20;

signals:
  void signalGroupsModified(Tellico::Data::CollPtr coll, QList<Tellico::Data::EntryGroup*> groups);
  void signalRefreshField(Tellico::Data::FieldPtr field);
  void mergeAddedField(Tellico::Data::CollPtr coll, Tellico::Data::FieldPtr field);

private:
  QStringList entryGroupNamesByField(EntryPtr entry, const QString& fieldName);
  void removeEntriesFromDicts(const EntryList& entries);
  void populateDict(EntryGroupDict* dict, const QString& fieldName, const EntryList& entries);
  void populateCurrentDicts(const EntryList& entries);
  void cleanGroups();
  bool dependentFieldHasRecursion(FieldPtr field);

  /*
   * Gets the preferred ID of the collection. Currently, it just gets incremented as
   * new collections are created.
   */
  static long getID();

  /**
   * The copy constructor is private, to ensure that it's never used.
   */
  Collection(const Collection& coll);
  /**
   * The assignment operator is private, to ensure that it's never used.
   */
  Collection operator=(const Collection& coll);

  long m_id;
  long m_nextEntryId;
  QString m_title;
  QString m_defaultGroupField;
  QString m_lastGroupField;

  FieldList m_fields;
  FieldList m_peopleFields; // keep separate list of people fields
  FieldList m_imageFields; // keep track of image fields
  QHash<QString, Field*> m_fieldNameDict;
  QHash<QString, Field*> m_fieldTitleDict;
  QStringList m_fieldCategories;
  QStringList m_fieldNames;
  QStringList m_fieldTitles;

  EntryList m_entries;
  QHash<long, Entry*> m_entryIdDict;

  QHash<QString, EntryGroupDict*> m_entryGroupDicts;
  QStringList m_entryGroups;
  QList<EntryGroup*> m_groupsToDelete;

  FilterList m_filters;
  BorrowerList m_borrowers;

  bool m_trackGroups : 1;
};

  } // end namespace
} //end namespace
#endif
