/***************************************************************************
                                bcattribute.h
                             -------------------
    begin                : Sun Sep 23 2001
    copyright            : (C) 2001, 2002, 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BCATTRIBUTE_H
#define BCATTRIBUTE_H

class BCAttribute;

#include <qstringlist.h>
#include <qstring.h>
#include <qmap.h>

typedef QPtrList<BCAttribute> BCAttributeList;
typedef QPtrListIterator<BCAttribute> BCAttributeListIterator;

/**
 * The BCAttribute class encapsulates all the possible properties of a unit.
 *
 * An attribute can be one of four types. It has a name, a title, and a category,
 * along with some flags characterizing certain properties
 *
 * @author Robby Stephenson
 * @version $Id: bcattribute.h 219 2003-10-24 03:04:04Z robby $
 */
class BCAttribute {
public:
  /**
   * The possible attribute types. A Line is represented by a KLineEdit,
   * a Para is a QMultiLineEdit encompassing multiple lines, a Choice is
   * limited to set values shown in a KComboBox, and a Bool is either true
   * or not and is thus a QCheckBox. A Number type is obvious, though it used
   * to be a Year. A ReadOnly is a hidden value. A URL is obvious, too.
   * A Table looks like a small spreadsheet with one column.
   * Don't forget to change BCAttribute::typeMap().
   *
   * @see KLineEdit
   * @see QMultiLineEdit
   * @see KComboBox
   * @see QCheckBox
   * @see QTable
   **/
  enum AttributeType {
    Line     = 1,
    Para     = 2,
    Choice   = 3,
    Bool     = 4,
    ReadOnly = 5,
    Number   = 6,
    URL      = 7,
    Table    = 8,
    Table2   = 9
  };

  /**
   * The attribute flags. The properties should be bit-wise OR'd together.
   *
   * @li AllowCompletion - Include a completion object in the lineedit.
   * @li AllowMultiple - Multiple values are allowed in one attribute and are
   *                     separated by a semi-colon (";").
   * @li AllowGrouped - Units may be grouped by this attribute.
   */
  enum AttributeFlags {
    AllowMultiple   = 1 << 0,   // allow multiple values, separated by a semi-colon
    AllowGrouped    = 1 << 1,   // this attribute can be used to group units
    AllowCompletion = 1 << 2,   // allow auto-completion
    NoDelete        = 1 << 3    // don't allow user to delete this attribute
  };

  /**
   * The attribute formatting flags.
   *
   * @li FormatTitle - The attribute should be formatted as a title
   * @li FormatName - The attribute should be formatted as a personal name
   * @li FormatDate - The attribute should be formatted as a date.
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
   * By default, the attribute category is set to "General", and should be modified
   * using the @ref setCategory() method.
   *
   * @param name The attribute name
   * @param title The attribute title
   * @param type The attribute type
   */
  BCAttribute(const QString& name, const QString& title, AttributeType type = Line);
  /**
   * The constructor for Choice types attributes.
   * By default, the attribute category is set to "General", and should be modified
   * using the @ref setCategory() method.
   *
   * @param name The attribute name
   * @param title The attribute title
   * @param allowed The allowed values of the attribute
   */
  BCAttribute(const QString& name, const QString& title, const QStringList& allowed);
  /**
   * The copy constructor
   */
  BCAttribute(const BCAttribute& att);
  /**
   * The assignment operator
   */
  BCAttribute& operator=(const BCAttribute& att);
  virtual ~BCAttribute() {};
  // default copy constructor is ok, right?
  virtual BCAttribute* clone() { return new BCAttribute(*this); }

  /**
   * Returns the name of the attribute.
   *
   * @return The attribute name
   */
  const QString& name() const;
  /**
   * Sets the name of the attribute. This should only be changed before the attribute is added
   * to a collection, i.e. before any units use it, etc.
   *
   * @param name The attribute name
   */
  void setName(const QString& name);
  /**
   * Returns the title of the attribute.
   *
   * @return The attribute title
   */
  const QString& title() const;
  /**
   * Sets the title of the attribute.
   *
   * @param title The attribute title
   */
  void setTitle(const QString& title);
  /**
   * Returns the category of the attribute.
   *
   * @return The attribute category
   */
  const QString& category() const;
  /**
   * Sets the category of the attribute.
   *
   * @param category The attribute category
   */
  void setCategory(const QString& category);
  /**
   * Returns the name of the attribute.
   *
   * @return The attribute name
   */
  const QStringList& allowed() const;
  /**
   * Sets the allowed values of the attribute.
   *
   * @param allowed The allowed values
   */
  void setAllowed(const QStringList& allowed);
  /**
   * Returns the type of the attribute.
   *
   * @return The attribute type
   */
  AttributeType type() const;
  /**
   * Sets the type of the attribute. Be careful with this!
   *
   * @param type The attribute type
   */
  void setType(AttributeType type);
  /**
   * Returns the flags for the attribute.
   *
   * @return The attribute flags
   */
  int flags() const;
  /**
   * Sets the flags of the attribute. The value is
   * set to the argument, so old flags are effectively removed.
   *
   * @param flags The attribute flags
   */
  void setFlags(int flags);
  /**
   * Returns the formatting flag for the attribute.
   *
   * @return The format flag
   */
  FormatFlag formatFlag() const;
  /**
   * Sets the formatting flag of the attribute.
   *
   * @param flag The attribute flag
   */
  void setFormatFlag(FormatFlag flag);
  /**
   * Returns the description for the attribute.
   *
   * @return The attribute description
   */
  const QString& description() const;
  /**
   * Sets the description of the attribute.
   *
   * @param desc The attribute description
   */
  void setDescription(const QString& desc);
  /**
   * Some attributes are always a category by themselves.
   *
   * @return Whether the attribute is th eonly member of its category
   */
  bool isSingleCategory() const;
  /**
   * Method to determine whether the attribute is a BibtexAttribute
   *
   * @return True if the attribute is a @ref BibtexAttribute
   */
  virtual bool isBibtexAttribute() const { return false; };

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
   * At the moment, this does nothing.
   *
   * @param date The string to be formatted
   */
  static QString formatDate(const QString& date);
  /**
   * Returns the default article list.
   *
   * @return The article list
   */
  static QStringList defaultArticleList();
  /**
   * Returns the current list of articles.
   *
   * @return The article list
   */
  static const QStringList& articleList();
  /**
   * Set the words used as articles.
   *
   * @param list The list if articles
   */
  static void setArticleList(const QStringList& list);
  /**
   * Returns the default suffix list.
   *
   * @return The suffix list
   */
  static QStringList defaultSuffixList();
  /**
   * Returns the list of suffixes used in personal names.
   *
   * @return The suffix list
   */
  static const QStringList& suffixList();
  /**
   * Set the words used as suffixes in personal names.
   *
   * @param list The list of suffixes
   */
  static void setSuffixList(const QStringList& list);
  /**
   * Returns the default surname prefix list.
   *
   * @return The prefix list
   */
  static QStringList defaultSurnamePrefixList();
  /**
   * Returns the list of surname prefixes used in personal names.
   *
   * @return The prefix list
   */
  static const QStringList& surnamePrefixList();
  /**
   * Set the words used as surname prefixes in personal names.
   *
   * @param list The list of prefixes
   */
  static void setSurnamePrefixList(const QStringList& list);
  /**
   * Returns true if the capitalization of titles and authors is set to be consistent.
   *
   * @return If capitalization is automatically done
   */
  static bool autoCapitalize();
  /**
   * Set whether the capitalization of titles and authors is automatic.
   *
   * @param autoCapitalize Whether to capitalize or not
   */
  static void setAutoCapitalize(bool autoCapitalize);
  /**
   * Returns true if the titles and authors should be formatted.
   *
   * @return If formatting is done
   */
  static bool autoFormat();
  /**
   * Set whether the formatting of titles and authors is automatic.
   *
   * @param autoFormat Whether to format or not
   */
  static void setAutoFormat(bool autoFormat);
  /**
   * Helper method to fix capitalization.
   *
   * @param str String to fix
   */
  static QString capitalize(QString str);
  /**
   * Returns a mapping of the AttribtueType enum to translated titles for the types.
   */
  static QMap<BCAttribute::AttributeType, QString> typeMap();

private:
  QString m_name;
  QString m_title;
  QString m_category;
  QString m_desc;
  AttributeType m_type;
  QStringList m_allowed;
  int m_flags;
  FormatFlag m_formatFlag;

  static QStringList m_articles;
  static QStringList m_suffixes;
  static QStringList m_surnamePrefixes;
  static QStringList m_noCapitalize;
  static bool m_autoCapitalize;
  static bool m_autoFormat;
};
#endif
