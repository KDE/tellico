/***************************************************************************
    copyright            : (C) 2002-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef ISBNVALIDATOR_H
#define ISBNVALIDATOR_H

#include <qvalidator.h>

namespace Tellico {

/**
 * @author Robby Stephenson
 *
 * @see http://www.isbn.org/standards/home/isbn/international/hyphenation-instructions.asp
 * @see http://www.eblong.com/zarf/bookscan/
 * @see http://doc.trolltech.com/qq/qq01-seriously-weird-qregexp.html
 */
class ISBNValidator : public QValidator {
public:
  ISBNValidator(QObject* parent, const char* name=0);

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
  virtual QValidator::State validate(QString& input, int& pos) const;

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
   * @see http://www.isbn.org/standards/home/isbn/international/hyphenation-instructions.asp
   * For info on group codes
   * @see http://www.isbn.spk-berlin.de/html/prefix/allpref.htm
   * For info on French language publisher codes
   * @see http://www.afnil.org
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
  virtual void fixup(QString& input) const;
  static void staticFixup(QString& input);

  static QString isbn10(QString isbn13);
  static QString isbn13(QString isbn10);
  static QString cleanValue(QString isbn);

private:
  static struct isbn_band {
    unsigned long MaxValue;
    unsigned int First;
    unsigned int Mid;
    unsigned int Last;
  } bands[];

  QValidator::State validate10(QString& input, int& pos) const;
  QValidator::State validate13(QString& input, int& pos) const;

  static void fixup10(QString& input);
  static void fixup13(QString& input);

  /**
   * This function calculates and returns the ISBN checksum. The
   * algorithm is based on some code by Andrew Plotkin, available at
   * http://www.eblong.com/zarf/bookscan/
   *
   * @see http://www.eblong.com/zarf/bookscan/
   *
   * @param input The raw string, with no hyphens
   */
  static QChar checkSum10(const QString& input);
  static QChar checkSum13(const QString& input);
};

} // end namespace
#endif
