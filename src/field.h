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

#ifndef TELLICO_FIELD_H
#define TELLICO_FIELD_H

#include "datavectors.h"

#include <ksharedptr.h>

#include <qstringlist.h>
#include <qstring.h>
#include <qregexp.h>

namespace Tellico {
  namespace Data {

/**
 * The Field class encapsulates all the possible properties of a entry.
 *
 * A field can be one of eleven types. It has a name, a title, and a category,
 * along with some flags characterizing certain properties
 *
 * @author Robby Stephenson
 */
class Field : public KShared {
public:
  /**
   * The possible field types. A Line is represented by a KLineEdit,
   * a Para is a QMultiLineEdit encompassing multiple lines, a Choice is
   * limited to set values shown in a KComboBox, and a Bool is either true
   * or not and is thus a QCheckBox. A Number type is obvious, though it used
   * to be a Year. A ReadOnly is a hidden value. A URL is obvious, too.
   * A Table looks like a small spreadsheet with one column, and a Table2
   * type has two columns.  An Image points to a QImage. A Dependent field
   * depends on the values of other attributes. A Date contains a date.
   *
   * Don't forget to change Field::typeMap().
   *
   * @see KLineEdit
   * @see QTextEdit
   * @see KComboBox
   * @see QCheckBox
   * @see QTable
   **/
  enum Type {
    Undef      = 0,
    Line       = 1,
    Para       = 2,
    Choice     = 3,
    Bool       = 4,
    ReadOnly   = 5,
    Number     = 6,
    URL        = 7,
    Table      = 8,
    Table2     = 9, // deprecated in favor of property("columns")
    Image      = 10,
    Dependent  = 11,
    Date       = 12,
    // Michael Zimmermann used 13 for his Keyword field, so go ahead and skip it
    Rating     = 14 // similar to a Choice field, but allowed values are numbers only
    // if you add your own field type, best to start at a high number
    // say 100, to ensure uniqueness
  };
  typedef QMap<Field::Type, QString> FieldMap;

  /**
   * The field flags. The properties should be bit-wise OR'd together.
   *
   * @li AllowCompletion - Include a completion object in the lineedit.
   * @li AllowMultiple - Multiple values are allowed in one field and are
   *                     separated by a semi-colon (";").
   * @li AllowGrouped - Entries may be grouped by this field.
   * @li NoDelete - The user may not delete this field.
   */
  enum FieldFlags {
    AllowMultiple   = 1 << 0,   // allow multiple values, separated by a semi-colon
    AllowGrouped    = 1 << 1,   // this field can be used to group entries
    AllowCompletion = 1 << 2,   // allow auto-completion
    NoDelete        = 1 << 3    // don't allow user to delete this field
  };

  /**
   * The field formatting flags.
   *
   * @li FormatTitle - The field should be formatted as a title
   * @li FormatName - The field should be formatted as a personal name
   * @li FormatDate - The field should be formatted as a date.
   * @li FormatPlain - The field only be formatted with capitalization.
   * @li FormatNone - The field should not be formatted.
   */
  enum FormatFlag {
    FormatPlain     = 0,   // format plain, allows capitalization
    FormatTitle     = 1,   // format as a title, i.e. shift articles to end
    FormatName      = 2,   // format as a personal full name
    FormatDate      = 3,   // format as a date
    FormatNone      = 4    // no format, i.e. no capitalization allowed
  };

  /**
   * The constructor for all types except Choice. The default type is Line.
   * By default, the field category is set to "General", and should be modified
   * using the @ref setCategory() method.
   *
   * @param name The field name
   * @param title The field title
   * @param type The field type
   */
  Field(const QString& name, const QString& title, Type type = Line);
  /**
   * The constructor for Choice types attributes.
   * By default, the field category is set to "General", and should be modified
   * using the @ref setCategory() method.
   *
   * @param name The field name
   * @param title The field title
   * @param allowed The allowed values of the field
   */
  Field(const QString& name, const QString& title, const QStringList& allowed);
  /**
   * The copy constructor
   */
  Field(const Field& field);
  /**
   * The assignment operator
   */
  Field& operator=(const Field& field);
  /**
   * Destructor
   */
  ~Field();

  /**
   * Returns the id of the field.
   *
   * @return The id
   */
  long id() const { return m_id; }
  /**
   * Returns the name of the field.
   *
   * @return The field name
   */
  const QString& name() const { return m_name; }
  /**
   * Sets the name of the field. This should only be changed before the field is added
   * to a collection, i.e. before any entries use it, etc.
   *
   * @param name The field name
   */
  void setName(const QString& name) { m_name = name; }
  /**
   * Returns the title of the field.
   *
   * @return The field title
   */
  const QString& title() const { return m_title; }
  /**
   * Sets the title of the field.
   *
   * @param title The field title
   */
  void setTitle(const QString& title);
  /**
   * Returns the category of the field.
   *
   * @return The field category
   */
  const QString& category() const { return m_category; }
  /**
   * Sets the category of the field.
   *
   * @param category The field category
   */
  void setCategory(const QString& category);
  /**
   * Returns the name of the field.
   *
   * @return The field name
   */
  const QStringList& allowed() const { return m_allowed; }
  /**
   * Sets the allowed values of the field.
   *
   * @param allowed The allowed values
   */
  void setAllowed(const QStringList& allowed) { m_allowed = allowed; }
  /**
   * Add a value to the allowed list
   *
   * @param value The value to allow
   */
  void addAllowed(const QString& value);
  /**
   * Returns the type of the field.
   *
   * @return The field type
   */
  Type type() const { return m_type; }
  /**
   * Sets the type of the field. Be careful with this!
   *
   * @param type The field type
   */
  void setType(Type type);
  /**
   * Returns the flags for the field.
   *
   * @return The field flags
   */
  int flags() const { return m_flags; }
  /**
   * Sets the flags of the field. The value is
   * set to the argument, so old flags are effectively removed.
   *
   * @param flags The field flags
   */
  void setFlags(int flags);
  /**
   * Returns the formatting flag for the field.
   *
   * @return The format flag
   */
  FormatFlag formatFlag() const { return m_formatFlag; }
  /**
   * Sets the formatting flag of the field.
   *
   * @param flag The field flag
   */
  void setFormatFlag(FormatFlag flag);
  /**
   * Returns the description for the field.
   *
   * @return The field description
   */
  const QString& description() const { return m_desc; }
  /**
   * Sets the description of the field.
   *
   * @param desc The field description
   */
  void setDescription(const QString& desc) { m_desc = desc; }
  /**
   * Returns the default value for the field.
   *
   * @return The field default value
   */
  const QString& defaultValue() const;
  /**
   * Sets the default value of the field.
   *
   * @param value The field default value
   */
  void setDefaultValue(const QString& value);
  /**
   * Some attributes are always a category by themselves.
   *
   * @return Whether the field is th eonly member of its category
   */
  bool isSingleCategory() const;
  /**
   * Extends a field with an additional key and value property pair.
   *
   * @param key The property key
   * @param value The property value
   */
  void setProperty(const QString& key, const QString& value);
  /**
   * Sets all the extended properties. Any existing ones get erased.
   *
   * @param properties The property list
   */
  void setPropertyList(const StringMap& properties);
  /**
   * Return a property value.
   *
   * @param key The property key
   * @returnThe property value
   */
  const QString& property(const QString& key) const { return m_properties[key]; }
  /**
   * Return the list of properties.
   *
   * @return The property list
   */
  const StringMap& propertyList() const { return m_properties; }

  /*************************** STATIC **********************************/

  /**
   * A wrapper method around all the format functions. The flags
   * determine which is called on the string.
   */
   static QString format(const QString& value, FormatFlag flag);
  /**
   * A convenience function to format a string as a title.
   * At the moment, this means that some articles such as "the" are placed
   * at the end of the title. If autoCapitalize() is true, the title is capitalized.
   *
   * @param title The string to be formatted
   */
  static QString formatTitle(const QString& title);
  /**
   * A convenience function to format a string as a personal name.
   * At the moment, this means that the name is split at the last white space,
   * and the last name is moved in front. If multiple=true, then the string
   * is split using a semi-colon (";"), and each string is formatted and then
   * joined back together. If autoCapitalize() is true, the names are capitalized.
   *
   * @param name The string to be formatted
   * @param multiple A boolean indicating if the string can contain multiple values
   */
  static QString formatName(const QString& name, bool multiple=true);
  /**
   * A convenience function to format a string as a date.
   *
   * @param date The string to be formatted
   */
  static QString formatDate(const QString& date);
  /**
   * Helper method to fix capitalization.
   *
   * @param str String to fix
   */
  static QString capitalize(QString str);
  /**
   * Returns a mapping of the FieldType enum to translated titles for the types.
   */
  static FieldMap typeMap();
  /**
   * Returns a list of the titles of the field types.
   */
  static QStringList typeTitles();
  /**
   * Splits a string into multiple values;
   *
   * @param string The string to be split
   */
  static QStringList split(const QString& string, bool allowEmpty);
  /**
   * Returns the delimiter used to split field values
   *
   * @return The delimeter regexp
   */
  static const QRegExp& delimiter() { return s_delimiter; }
  /**
   * reset if the field is a rating field used for syntax version 7 and earlier */
  static void convertOldRating(Data::FieldPtr field);
  static void stripArticles(QString& value);
  static void articlesUpdated();


private:
  /*
   * Gets the preferred ID of the collection. Currently, it just gets incremented as
   * new collections are created.
   */
  static long getID();

  long m_id;
  QString m_name;
  QString m_title;
  QString m_category;
  QString m_desc;
  Type m_type;
  QStringList m_allowed;
  int m_flags;
  FormatFlag m_formatFlag;
  StringMap m_properties;

  // need to remember articles with apostrophes for capitalization
  static QStringList s_articlesApos;
  static QRegExp s_delimiter;
};

  } // end namespace
} // end namespace
#endif
