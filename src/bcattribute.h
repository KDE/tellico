/* *************************************************************************
                                bcattribute.h
                             -------------------
    begin                : Sun Sep 23 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@radiojodi.com
 * *************************************************************************/

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 * *************************************************************************/

#ifndef BCATTRIBUTE_H
#define BCATTRIBUTE_H

#include <qstringlist.h>
#include <qstring.h>

/**
 * The BCAttribute class encapsulates all the possible properties of a unit.
 *
 * An attribute can be one of four types. It has a name, a title, and a group, along with
 * some flags characterizing certain properties
 *
 * @author Robby Stephenson
 * @version $Id: bcattribute.h,v 1.13 2002/02/09 07:11:25 robby Exp $
 */
class BCAttribute {
public:
  /**
   * The possible attribute types. A Line is represented by a KLineEdit,
   * a Para is a QMultiLineEdit encompassing multiple lines, a Choice is
   * limited to set values shown in a KComboBox, and a Bool is either true
   * or not and is thus a QCheckBox.
   *
   * @see KLineEdit
   * @see QMultiLineEdit
   * @see KComboBox
   * @see QCheckBox
   **/
  enum AttributeType {
    Line = 1,
    Para = 2,
    Choice = 3,
    Bool = 4
  };

  /**
   * The attribute flags. All the visibility and formatting properties are smushed into
   * one single flags variable. The properties should be bit-wise AND'd together.
   * @li DontComplete - Don't include a completion object in the lineedit
   * @li DontShow - Don't show this attribute in the listviews.
   * @li AllowMultiple - Multiple values are allowed in one attribute and are
   *                     separated by a semi-colon (";").
   * @li FormatTitle - The attribute should be formatted as a title
   * @li FormatName - The attribute should be formatted as a personal name
   * @li FormatDate - The attribute should be formatted as a date.
   * @li AllowGrouped - Units may be grouped by this attribute.
   *
   * Obviously, the last three are mutually exclusive, but this is neither checked nor enforced.
   **/
  enum AttributeFlags {
    DontComplete    = 1 << 0,   // don't allow auto-completion
    DontShow        = 1 << 1,   // dont show in list view
    AllowMultiple   = 1 << 2,   // allow multiple values, separated by a comma
    FormatTitle     = 1 << 3,   // format as a title, i.e. shift articles to end
    FormatName      = 1 << 4,   // format as a personal full name
    FormatDate      = 1 << 5,   // format as a date
    AllowGrouped    = 1 << 6    // can be used to group units
  };

  /**
   * The constructor for all types except Choice. The default type is Line.
   * By default, the attribute group is set to "General", and should be modified using
   * the @ref setGroup() method.
   *
   * @param name The attribute name
   * @param title The attribute title
   * @param type The attribute type
   */
  BCAttribute(const QString& name, const QString& title, AttributeType type = Line);
  /**
   * The constructor for Choice types attributes.
   * By default, the attribute group is set to "General", and should be modified using
   * the @ref setGroup() method.
   *
   * @param name The attribute name
   * @param title The attribute title
   * @param allowed The allowed values of the attribute
   */
  BCAttribute(const QString& name, const QString& title, const QStringList& allowed);
  /**
   */
  ~BCAttribute();

  /**
   * Returns the name of the attribute.
   *
   * @return The attribute name
   */
  const QString& name() const;
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
   * Returns the group name of the attribute.
   *
   * @return The attribute group name
   */
  const QString& group() const;
  /**
   * Sets the group name of the attribute.
   *
   * @param group The attribute group name
   */
  void setGroup(const QString& group);
  /**
   * Returns the name of the attribute.
   *
   * @return The attribute name
   */
  const QStringList& allowed() const;
  /**
   * Sets the list of the allowed values of the attribute.
   *
   * @param list The allowed attribute values
   */
  void setAllowed(const QStringList& list);
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
   * A convenience function to format a string as a title.
   * At the moment, this means that some articles such as "the" are placed
   * at the end of the title.
   *
   * @param title The string to be formatted
   * @return The formatted string
   */
  static QString formatTitle(const QString& title);
  /**
   * A convenience function to format a string as a personal name.
   * At the moment, this means that the name is split at the last white space,
   * and the last name is moved in front. If multiple=true, then the string
   * is split using a semi-colon (";"), and each string is formatted and then
   * joined back together.
   *
   * @param name The string to be formatted
   * @param multiple A boolean indicating if the string can contain multiple values
   * @return The formatted string
   */
  static QString formatName(const QString& name, bool multiple=true);
  /**
   * A convenience function to format a string as a date.
   * At the moment, this does nothing.
   *
   * @param date The string to be formatted
   * @return The formatted string
   */
  static QString formatDate(const QString& date);
  /**
   * Returns the default article list.
   *
   * @return The article list
   */
  static QStringList defaultArticleList();
  /**
   * Returns the list of articles.
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
   * @return If capitalization is automatically fixed
   */
  static bool isAutoCapitalization();
  /**
   * Set whether the capitalization of titles and authors is automatic.
   *
   * @param fix
   */
  static void setAutoCapitalization(bool autoCapital);

protected:
  /**
   * The copy constructor is private, to ensure that it's never used.
   */
  BCAttribute(const BCAttribute& att);
  /**
   * The assignment operator is private, to ensure that it's never used.
   */
  BCAttribute operator=(const BCAttribute& att);
  /**
   * Helper method to fix capitalization.
   *
   * @param str String to fix
   */
  static QString capitalize(const QString& str);

private:
  QString m_name;
  QString m_title;
  QString m_group;
  AttributeType m_type;
  QStringList m_allowed;
  int m_flags;

  static QStringList m_articles;
  static QStringList m_suffixes;
  static bool m_autoCapitalization;
};
#endif
