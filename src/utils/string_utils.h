/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef STRING_UTILS_H
#define STRING_UTILS_H

class QByteArray;
class QString;

/**
 * This file contains utility functions for manipulating strings.
 *
 * @author Robby Stephenson
 */
namespace Tellico {
  /**
   * Decode HTML entities. Only numeric entities are handled currently.
   */
  QString decodeHTML(const QByteArray& data);
  QString decodeHTML(const QString& text);

  /**
   * Return a random, and almost certainly unique UID.
   *
   * @param length The UID starts with "Tellico" and adds enough letters to be @p length long.
   */
  QString uid(int length=20, bool prefix=true);
  int toInt(const QString& string, bool* ok);
  unsigned int toUInt(const QString& string, bool* ok);
  /**
   * Replace all occurrences  of <i18n>text</i18n> with i18n("text")
   */
  QString i18nReplace(QString text);
  QString removeAccents(const QString& value);

  int stringHash(const QString& str);
  /** take advantage string collisions to reduce memory
  */
  QString shareString(const QString& str);

  QString minutes(int seconds);
  QString fromHtmlData(const QByteArray& data, const char* codecName = nullptr);

  QByteArray obfuscate(const QString& string);
  QString reverseObfuscate(const QByteArray& bytes);

  QString removeControlCodes(const QString& string);
}

#endif
