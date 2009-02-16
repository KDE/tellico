/***************************************************************************
    copyright            : (C) 2009 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_FIELDFORMAT_H
#define TELLICO_FIELDFORMAT_H

#include <QString>
#include <QStringList>
#include <QRegExp>

namespace Tellico {

class FieldFormat {
public:
  /**
   * Returns the delimiter used to split field values
   *
   * @return The delimeter regexp
   */
  static const QRegExp& delimiter();
  /**
   * A convenience function to format a string as a title.
   * At the moment, this means that some articles such as "the" are placed
   * at the end of the title. If autoCapitalize() is true, the title is capitalized.
   *
   * @param title The string to be formatted
   */
  static QString title(const QString& title);
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
  static QString name(const QString& name, bool multiple=true);
  /**
   * A convenience function to format a string as a date.
   *
   * @param date The string to be formatted
   */
  static QString date(const QString& date);
  /**
   * Helper method to fix capitalization.
   *
   * @param str String to fix
   */
  static QString capitalize(QString str);
  /**
   * Return the key to be used for sorting titles
   */
  static QString sortKeyTitle(const QString& title);

  static void articlesUpdated();
  static void stripArticles(QString& value);

private:
  // need to remember articles with apostrophes for capitalization
  static QStringList articles; //saved so the Config doesn't have to split the string every time
  static QStringList articlesApos;
  static QRegExp delimiterRx;
  static QRegExp commaSplitRx;
};

} // namespace

#endif