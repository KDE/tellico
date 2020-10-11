/***************************************************************************
    Copyright (C) 2009-2020 Robby Stephenson <robby@periapsis.org>
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
#include <QRegularExpression>

namespace Tellico {

class FieldFormat {
public:
  /**
   * The field formatting flags.
   *
   * @li FormatTitle - The field should be formatted as a title
   * @li FormatName - The field should be formatted as a personal name
   * @li FormatDate - The field should be formatted as a date.
   * @li FormatPlain - The field only be formatted with capitalization.
   * @li FormatNone - The field should not be formatted.
   */
  enum Type {
    FormatPlain     = 0,   // format plain, allows capitalization
    FormatTitle     = 1,   // format as a title, i.e. shift articles to end
    FormatName      = 2,   // format as a personal full name
    FormatDate      = 3,   // format as a date
    FormatNone      = 4    // no format, i.e. no capitalization allowed
  };
  enum Request {
    AsIsFormat,
    DefaultFormat,
    ForceFormat
  };
  enum Option {
    FormatCapitalize     = 1 << 0,
    FormatAuto           = 1 << 1,
    SplitMultiple        = 1 << 2
  };
  Q_DECLARE_FLAGS(Options, Option)

  /**
   * Splits a string into multiple values;
   *
   * @param string The string to be split
   */
  enum SplitParsing { StringSplit, RegExpSplit };
  static QStringList splitValue(const QString& string,
                                SplitParsing parsing = RegExpSplit);
  static QStringList splitRow(const QString& string);
  static QStringList splitTable(const QString& string);
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
  static QRegularExpression delimiterRegularExpression();
  static QRegularExpression commaSplitRegularExpression();
  static QString columnDelimiterString();
  static QString rowDelimiterString();
  static QString matchValueRegularExpression(const QString& value);

  static QString fixupValue(const QString& value);
  /**
   * Return the key to be used for sorting titles
   */
  static QString sortKeyTitle(const QString& title);

  static void stripArticles(QString& value);

  static QString format(const QString& value, Type type, Request req);

  /**
   * A convenience function to format a string as a title.
   * At the moment, this means that some articles such as "the" are placed
   * at the end of the title. If autoCapitalize() is true, the title is capitalized.
   *
   * @param title The string to be formatted
   */
  static QString title(const QString& title, Options options);
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
  static QString name(const QString& name, Options options);
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
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FieldFormat::Options)

} // namespace

#endif
