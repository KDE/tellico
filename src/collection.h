/***************************************************************************
    copyright            : (C) 2001-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef COLLECTION_H
#define COLLECTION_H

#include "field.h"
#include "entry.h"
#include "filter.h"
#include "borrower.h"
#include "datavectors.h"

#include <klocale.h>
#include <ksharedptr.h>

#include <qstringlist.h>
#include <qptrlist.h>
#include <qstring.h>
#include <qdict.h>
#include <qintdict.h>
#include <qobject.h>

namespace Tellico {
  namespace Data {
    class EntryGroup;
    typedef QDict<EntryGroup> EntryGroupDict;

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
class Collection : public QObject, public KShared {
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
    Game = 11
    // if you want to add custom collection types, use a numebr sure to be unique like 101
  };

  /**
   * The constructor is only used to create custom collections. It adds a title field,
   * in the "General" group. The iconName is set to be the entryName;
   *
   * @param title The title of the collection itself
   * @param entryName The name of the entries that the collection holds (not translated)
   * @param entryTitle The title of the entries, which can be translated
   */
  Collection(const QString& title, const QString& entryName, const QString& entryTitle);
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
  int id() const { return m_id; }
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
   * @return The entry name
   */
  const QString& entryName() const { return m_entryName; }
  /**
   * Returns the title of the entries in the collection, e.g. "Book".
   * Translated.
   *
   * @return The entry title
   */
  const QString& entryTitle() const { return m_entryTitle; }
  /**
   * Returns the name of the icon used for the entries in the collection.
   *
   * @return The icon name
   */
  const QString& iconName() const { return m_iconName; }
  /**
   * Sets the name of the icon used for entries in the collection.
   *
   * @param name The new icon name
   */
  void setIconName(const QString& name) { m_iconName = name; }
  /**
   * Returns a reference to the list of all the entries in the collection.
   *
   * @return The list of entries
   */
  const EntryVec& entries() const { return m_entries; }
  /**
   * Returns a reference to the list of the collection attributes.
   *
   * @return The list of fields
   */
  const FieldVec& fields() const { return m_fields; }
  Entry* entryById(int id);
  /**
   * Returns a reference to the list of the collection's people fields.
   *
   * @return The list of fields
   */
  const FieldVec& peopleFields() const { return m_peopleFields; }
  /**
   * Returns a reference to the list of the collection's image fields.
   *
   * @return The list of fields
   */
  const FieldVec& imageFields() const { return m_imageFields; }
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
  size_t entryCount() const { return m_entries.count(); }
  /**
   * Adds a entry to the collection. The collection takes ownership of the entry object.
   *
   * @param entry A pointer to the entry
   */
  void addEntry(Entry* entry);
  /**
   * Updates the dicts that include the entry.
   *
   * @param entry A pointer to the entry
   */
  void updateDicts(Entry* entry);
  /**
   * Deletes a entry from the collection.
   *
   * @param entry The pointer to the entry
   * @return A boolean indicating if the entry was in the collection and was deleted
   */
  bool removeEntry(Entry* entry);
  /**
   * Adds a whole list of attributes. It's gotta be virtual since it calls
   * @ref addField, which is virtual.
   *
   * @param list List of attributes to add
   * @return A boolean indicating if the field was added or not
   */
  virtual bool addFields(FieldVec list);
  /**
   * Adds an field to the collection, unless an field with that name
   * already exists. The collection takes ownership of the field object.
   *
   * @param field A pointer to the field
   * @return A boolean indicating if the field was added or not
   */
  virtual bool addField(Field* field);
  virtual bool mergeField(Field* field);
  virtual bool modifyField(Field* field);
  virtual bool removeField(Field* field, bool force=false);
  virtual bool removeField(const QString& name, bool force=false);
  void reorderFields(const FieldVec& list);

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
  const QString& fieldTitleByName(const QString& name) const;
  /**
   * Returns the name of an field, given its title.
   *
   * @param title The field title
   * @return The field name
   */
  const QString& fieldNameByTitle(const QString& title) const;
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
  FieldVec fieldsByCategory(const QString& category);
  /**
   * Returns a pointer to an field given its name. If none is found, a NULL pointer
   * is returned.
   *
   * @param name The field name
   * @return The field pointer
   */
  Field* const fieldByName(const QString& name) const;
  /**
   * Returns a pointer to an field given its title. If none is found, a NULL pointer
   * is returned. This lookup is slower than by name.
   *
   * @param title The field title
   * @return The field pointer
   */
  Field* const fieldByTitle(const QString& title) const;
  /**
   * Returns a list of all the possible entry groups. This value is cached rather
   * than generated with each call, so the method should be fairly fast.
   *
   * @return The list of groups
   */
  const QStringList& entryGroups() const { return m_entryGroups; }
  /**
   * Returns a pointer to a const dict of all the entries grouped by
   * a certain field
   *
   * @param name The name of the field by which the entries are grouped
   * @return The list of group names
   */
  EntryGroupDict* const entryGroupDictByName(const QString& name) const;
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

  void addBorrower(Data::Borrower* borrower);
  const BorrowerVec& borrowers() const { return m_borrowers; }

  void addFilter(Filter* filter);
  bool removeFilter(Filter* filter);
  const FilterVec& filters() const { return m_filters; }

  /**
   * The string used for empty values. This forces consistency.
   */
  static const QString s_emptyGroupTitle;
  /**
   * The string used for the people pseudo-group. This forces consistency.
   */
  static const QString s_peopleGroupName;

signals:
  void signalGroupModified(Tellico::Data::Collection* coll, Tellico::Data::EntryGroup* group);
  void signalRefreshField(Tellico::Data::Field* field);

protected:
  void removeEntryFromDicts(Entry* entry);
  void populateDicts(Entry* entry);
  QStringList entryGroupNamesByField(Entry* entry, const QString& fieldName);
  /*
   * Gets the preferred ID of the collection. A QIntDict is used to keep track of which
   * id's are in use, and the actual ID is returned.
   */
  static int getID();

private:
  /**
   * The copy constructor is private, to ensure that it's never used.
   */
  Collection(const Collection& coll);
  /**
   * The assignment operator is private, to ensure that it's never used.
   */
  Collection operator=(const Collection& coll);

  int m_id;
  QString m_title;
  QString m_entryName;
  QString m_entryTitle;
  QString m_iconName;
  QString m_defaultGroupField;

  FieldVec m_fields;
  FieldVec m_peopleFields; // keep separate list of people fields
  FieldVec m_imageFields; // keep track of image fields
  QDict<Field> m_fieldNameDict;
  QDict<Field> m_fieldTitleDict;
  QStringList m_fieldCategories;
  QStringList m_fieldNames;
  QStringList m_fieldTitles;

  EntryVec m_entries;
  QIntDict<Entry> m_entryIdDict;

  QDict<EntryGroupDict> m_entryGroupDicts;
  QStringList m_entryGroups;

  FilterVec m_filters;
  BorrowerVec m_borrowers;
};

  } // end namespace
} //end namespace
#endif
