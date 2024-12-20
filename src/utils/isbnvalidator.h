/***************************************************************************
    Copyright (C) 2002-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_ISBNVALIDATOR_H
#define TELLICO_ISBNVALIDATOR_H

#include <QValidator>

namespace Tellico {

/**
 * @author Robby Stephenson
 *
 * @see https://web.archive.org/web/20130126042049/http://www.isbn.org/standards/home/isbn/international/hyphenation-instructions.asp
 * @see https://www.eblong.com/zarf/bookscan/
 * @see https://doc.qt.io/archives/qq/qq01-seriously-weird-qregexp.html
 */
class ISBNValidator : public QValidator {
Q_OBJECT

public:
  ISBNValidator(QObject* parent = nullptr);

  /**
   * Certain conditions are checked. Character, length and position
   * restrictions are checked. Certain cases where the user is deleting
   * characters are caught and compensated for. The string is then passed to
   * @ref fixup. Finally, the text is @ref Valid if it is a certain length and
   * @ref Intermediate if not.
   *
   * @param input The text to validate
   * @param pos The position of the cursor
   * @return The condition of the text
   */
  virtual QValidator::State validate(QString& input, int& pos) const override;

  /**
   * The input string is examined. Hyphens are inserted appropriately,
   * and the checksum is calculated.
   *
   * For correct presentation, the 10 digits of an ISBN must
   * be divided, by hyphens, into four parts:
   * @li Part 1: The country or group of countries identifier
   * @li Part 2: The publisher identifier
   * @li Part 3: The title identifier
   * @li Part 4: The check digit
   * For details
   * @see https://web.archive.org/web/20130126042049/http://www.isbn.org/standards/home/isbn/international/hyphenation-instructions.asp
   * For details on ranges
   * @see https://www.isbn-international.org/range_file_generation
   * For info on group codes
   * @see https://web.archive.org/web/20030609050408/http://www.isbn.spk-berlin.de/html/prefix/allpref.htm
   * For info on French language publisher codes
   * @see https://www.afnil.org/
   * <pre>
   *  Group Identifiers    First Hyphen After
   *  -----------------------------------------
   *  0........7              1st digit
   *  80.......94             2nd   "
   *  950......993            3rd   "
   *  9940.....9989           4th   "
   *  99900....99999          5th   "
   *
   *  Group                   Insert Hyphens
   *  Identifier "0"            After
   *  -----------------------------------------
   *  00.......19          1st  3rd  9th digit
   *  200......699          "   4th       "
   *  7000.....8499         "   5th       "
   *  85000....89999        "   6th       "
   *  900000...949999       "   7th       "
   *  9500000..9999999      "   8th       "
   *
   *
   *  Group                  Insert Hyphens
   *  Identifier "1"           After
   *  ----------------------------------------
   *  0........54999           illegal
   *  55000....86979      1st  6th  9th digit
   *  869800...998999      "   7th       "
   *  9990000..9999999     "   8th       "
   *
   *
   *  Group                   Insert Hyphens
   *  Identifier "2"            After
   *  -----------------------------------------
   *  00.......19          1st  3rd  9th digit
   *  200......349          "   4th       "
   *  34000....39999        "   6th       "
   *  400......699          "   4th       "
   *  7000.....8399         "   5th       "
   *  84000....89999        "   6th       "
   *  900000...949999       "   7th       "
   *  9500000..9999999      "   8th       "
   *
   *  The position of the hyphens are determined by the publisher
   *  prefix range established by each national agency in accordance
   *  with the industry needs. The knowledge of the prefix ranges for
   *  each country or group of countries is necessary to develop the
   *  hyphenation output program. For groups 3 through 99999, the hyphenation
   *  rules are currently unknown. So just leave out the hyphen between
   *  the publisher and title for now, but allow it if the user inserts it.
   * </pre>
   *
   * @param input The raw string, hyphens included
   */
  virtual void fixup(QString& input) const override;

  static void staticFixup(QString& input);
  static void fixup10(QString& input);
  static void fixup13(QString& input);

  static QString isbn10(QString isbn13);
  static QString isbn13(QString isbn10);
  static QString cleanValue(QString isbn);
  // returns the values in list1 that are not in list2
  static QStringList listDifference(const QStringList& list1, const QStringList& list2);

private:
  static struct isbn_band {
    unsigned long MaxValue;
    int First;
    int Mid;
    int Last;
  } bands[];

  QValidator::State validate10(QString& input, int& pos) const;
  QValidator::State validate13(QString& input, int& pos) const;

  /**
   * This function calculates and returns the ISBN checksum. The
   * algorithm is based on some code by Andrew Plotkin, available at
   * https://www.eblong.com/zarf/bookscan/
   *
   * @see https://www.eblong.com/zarf/bookscan/
   *
   * @param input The raw string, with no hyphens
   */
  static QChar checkSum10(const QString& input);
  static QChar checkSum13(const QString& input);
};

class ISBNComparison {
public:
  bool operator()(const QString& value1, const QString& value2) const;
};

} // end namespace
#endif
