/***************************************************************************
                                bcattribute.h
                             -------------------
    begin                : Sun Sep 23 2001
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

#ifndef BCATTRIBUTE_H
#define BCATTRIBUTE_H

class BCAttribute;

#include <qstringlist.h>
#include <qstring.h>

typedef QPtrList<BCAttribute> BCAttributeList;
typedef QPtrListIterator<BCAttribute> BCAttributeListIterator;

/**
 * The BCAttribute class encapsulates all the possible properties of a unit.
 *
 * An attribute can be one of four types. It has a name, a title, and a category,
 * along with some flags characterizing certain properties
 *
 * @author Robby Stephenson
 * @version $Id: bcattribute.h,v 1.31 2003/03/15 05:54:03 robby Exp $
 */
class BCAttribute {
public:
  /**
   * The possible attribute types. A Line is represented by a KLineEdit,
   * a Para is a QMultiLineEdit encompassing multiple lines, a Choice is
   * limited to set values shown in a KComboBox, and a Bool is either true
   * or not and is thus a QCheckBox. A Year type is obvious.
   * A ReadOnly is a hidden value.
   *
   * @see KLineEdit
   * @see QMultiLineEdit
   * @see KComboBox
   * @see QCheckBox
   **/
  enum AttributeType {
    Line     = 1,
    Para     = 2,
    Choice   = 3,
    Bool     = 4,
    ReadOnly = 5,
    Year     = 6
  };

  /**
   * The attribute flags. All the visibility and formatting properties are smushed into
   * one single flags variable. The properties should be bit-wise OR'd together.
   * @li NoComplete - Don't include a completion object in the lineedit
   * @li AllowMultiple - Multiple values are allowed in one attribute and are
   *                     separated by a semi-colon (";").
   * @li FormatTitle - The attribute should be formatted as a title
   * @li FormatName - The attribute should be formatted as a personal name
   * @li FormatDate - The attribute should be formatted as a date.
   * @li AllowGrouped - Units may be grouped by this attribute.
   *
   * Obviously, the three format options are mutually exclusive, but this is
   * neither checked nor enforced.
   */
  enum AttributeFlags {
    NoComplete      = 1 << 0,   // don't allow auto-completion
    AllowMultiple   = 1 << 2,   // allow multiple values, separated by a semi-colon
    FormatTitle     = 1 << 3,   // format as a title, i.e. shift articles to end
    FormatName      = 1 << 4,   // format as a personal full name
    FormatDate      = 1 << 5,   // format as a date
    AllowGrouped    = 1 << 6    // this attribute can be used to group units
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
   * Returns the name of the attribute.
   *
   * @return The attribute name
   */
  const QString& name() const;
  /**
   * Sets the name of the attribute.
   *
   * @param name The attribute name
   */
//  void setName(const QString& name);
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
   * Returns the type of the attribute.
   *
   * @return The attribute type
   */
  AttributeType type() const;
  /**
   * Returns the visibility and formatting flags for the attribute.
   *
   * @return The attribute flags
   */
  int flags() const;
  /**
   * Sets the visibility and formatting flags of the attribute. The value is
   * set to the argument, so old flags are effectively removed.
   *
   * @param flags The attribute flags
   */
  void setFlags(int flags);
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

  /*************************** STATIC **********************************/

  /**
   * A wrapper method around all the format functions. The flags
   * determine which is called on the string.
   */
   static QString format(const QString& value, int flags);
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
   * @return The article list
   */
  static const QStringList& suffixList();
  /**
   * Set the words used as suffixes in personal names.
   *
   * @param list The list if articles
   */
  static void setSuffixList(const QStringList& list);
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

private:
  /**
   * The copy constructor is private, to ensure that it's never used.
   */
  BCAttribute(const BCAttribute& att);
  /**
   * The assignment operator is private, to ensure that it's never used.
   */
  BCAttribute operator=(const BCAttribute& att);

private:
  QString m_name;
  QString m_title;
  QString m_category;
  QString m_desc;
  AttributeType m_type;
  QStringList m_allowed;
  int m_flags;

  static QStringList m_articles;
  static QStringList m_suffixes;
  static bool m_autoCapitalize;
  static bool m_autoFormat;
};
#endif
