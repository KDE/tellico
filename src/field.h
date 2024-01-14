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

#ifndef TELLICO_FIELD_H
#define TELLICO_FIELD_H

#include "datavectors.h"
#include "fieldformat.h"

#include <QStringList>

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
class Field : public QSharedData {
public:
  /**
   * The possible field types. A Line is represented by a QLineEdit,
   * a Para is a QMultiLineEdit encompassing multiple lines, a Choice is
   * limited to set values shown in a KComboBox, and a Bool is either true
   * or not and is thus a QCheckBox. A Number type is an integer, though it used
   * to be a Year. A URL is obvious, too.
   * A Table looks like a small spreadsheet with one column, and a Table2
   * type has two columns.  An Image points to a QImage. A Date contains a date.
   *
   * Table2, ReadOnly, and Dependent are deprecated
   *
   * Don't forget to change Field::typeMap().
   **/
  enum Type {
    Undef      = 0,
    Line       = 1,
    Para       = 2,
    Choice     = 3,
    Bool       = 4,
    ReadOnly   = 5, // deprecated in favor of FieldFlags::NoEdit
    Number     = 6,
    URL        = 7,
    Table      = 8,
    Table2     = 9, // deprecated in favor of property("columns")
    Image      = 10,
    Dependent  = 11, // deprecated in favor of FieldFlags::Derived
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
  enum FieldFlag {
    AllowMultiple   = 1 << 0,   // allow multiple values, separated by a semi-colon
    AllowGrouped    = 1 << 1,   // this field can be used to group entries
    AllowCompletion = 1 << 2,   // allow auto-completion
    NoDelete        = 1 << 3,   // don't allow user to delete this field
    NoEdit          = 1 << 4,   // don't allow user to edit this field
    Derived         = 1 << 5    // dependent value
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
  bool hasFlag(FieldFlag flag) const;
  /**
   * Returns the formatting flag for the field.
   *
   * @return The format flag
   */
  FieldFormat::Type formatType() const { return m_formatType; }
  /**
   * Sets the formatting flag of the field.
   *
   * @param flag The field flag
   */
  void setFormatType(FieldFormat::Type flag);
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
  QString defaultValue() const;
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
  QString property(const QString& key) const;
  /**
   * Return the list of properties.
   *
   * @return The property list
   */
  const StringMap& propertyList() const { return m_properties; }

  /*************************** STATIC **********************************/
  /**
   * Returns a mapping of the FieldType enum to translated titles for the types.
   */
  static FieldMap typeMap();
  /**
   * Returns a list of the titles of the field types.
   */
  static QStringList typeTitles();

 /**
  * reset if the field is a rating field used for syntax version 7 and earlier
  */
  static void convertOldRating(Data::FieldPtr field);

  enum DefaultField {
    IDField,
    TitleField,
    CreatedDateField,
    ModifiedDateField,
    IsbnField,
    LccnField,
    PegiField,
    ImdbField,
    EpisodeField,
    ScreenshotField,
    FrontCoverField
  };

  static FieldPtr createDefaultField(DefaultField field);

private:
  QString m_name;
  QString m_title;
  QString m_category;
  QString m_desc;
  Type m_type;
  QStringList m_allowed;
  int m_flags;
  FieldFormat::Type m_formatType;
  StringMap m_properties;
};

  } // end namespace

  Data::FieldList listIntersection(const Data::FieldList& list1, const Data::FieldList& list2);
} // end namespace

Q_DECLARE_METATYPE(Tellico::Data::FieldPtr)

#endif
