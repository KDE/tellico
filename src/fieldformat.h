/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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
   * @return The delimiter string
   */
  static QString delimiterString();
  /**
   * Returns the delimiter used to split field values
   *
   * @return The delimiter regexp
   */
  static QRegExp delimiterRegExp();

  static QString fixupValue(const QString& value);

  static QString columnDelimiterString();
  static QString rowDelimiterString();

  /**
   * Splits a string into multiple values;
   *
   * @param string The string to be split
   */
  enum SplitParsing { StringSplit, RegExpSplit };
  static QStringList splitValue(const QString& string,
                                SplitParsing parsing = RegExpSplit,
                                QString::SplitBehavior behavior = QString::KeepEmptyParts);
  static QStringList splitRow(const QString& string,
                              QString::SplitBehavior behavior = QString::KeepEmptyParts);
  static QStringList splitTable(const QString& string,
                                QString::SplitBehavior behavior = QString::KeepEmptyParts);

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
  static QString capitalize(QString str, bool checkConfig=false);
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
