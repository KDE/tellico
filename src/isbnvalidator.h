/***************************************************************************
                              isbnvalidator.h
                             -------------------
    begin                : Sun Oct 6 2002
    copyright            : (C) 2002 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ISBNVALIDATOR_H
#define ISBNVALIDATOR_H

#include <qvalidator.h>
#include <qbitarray.h>

/**
 * @author Robby Stephenson
 * @version $Id: isbnvalidator.h,v 1.7 2002/11/26 05:46:19 robby Exp $
 *
 * Parts of this card are based on Java code
 * copyright (c) 1998-2002 Roedy Green, Canadian Mind Products which
 * may be copied and used freely for any purpose but military.
 *
 * @see http://mindprod.com/isbn.html
 * @see http://www.eblong.com/zarf/bookscan/
 * @see http://doc.trolltech.com/qq/qq01-seriously-weird-qregexp.html
 */
class ISBNValidator : public QValidator  {
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
  QValidator::State validate(QString& input, int& pos) const;

  /**
   * The input string is examined. Hyphens are inserted appropriately,
   * and the checksum is calculated.
   *
   * @param input The raw string, hyphens included
   */
  void fixup(QString& input) const;

private:
  /** This function calculates the checksum and adds it to the number. The
   * algorithm is based on some code by Andrew Plotkin, available at 
   * http://www.eblong.com/zarf/bookscan/
   *
   * @see http://www.eblong.com/zarf/bookscan/
   *
   * @param input The raw string, with no hyphens
   */
  void checkSum(QString& input) const;
  
  /**
   * This function inserts hyphens. The code is heavily based on the Java ISBN
   * validator by Roedy Green at http://mindprod.com/isbn.html and copyright by him.
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
   * <pre>
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
   *  For other groups, 2 .. 99999 properly we would need to find out the rules
   *  separately for each country. For now we simply leave out the dash between
   *  publisher and title
   * </pre>
   * @see http://mindprod.com/isbn.html
   *
   * @param input The raw string, with no hyphens
   */
  void insertDashes(QString& input) const;
#if 0
  /**
   * This function takes the list of valid group numbers and populates a
   * QBitArray. This is necessary since not all groups have the same number
   * of digits.
   */
  void buildValidGroupLookup();

  QBitArray validGroupLookup;

  static const int validGroups[];
  static const int numGroups;
#endif
};

#endif
