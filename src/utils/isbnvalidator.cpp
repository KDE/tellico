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

#include "isbnvalidator.h"
#include "upcvalidator.h"

#include <QStringList>
#include <QRegularExpression>

using Tellico::ISBNValidator;

//static
QString ISBNValidator::isbn10(QString isbn13) {
  QString original = isbn13;
  isbn13.remove(QLatin1Char('-'));
  if(isbn13.length() == 10) {
    fixup10(isbn13);
    return isbn13;
  }
  if(!isbn13.startsWith(QStringLiteral("978"))) {
    return original;
  }
  if(isbn13.length() < 13) {
    fixup10(isbn13);
    return isbn13;
  }
  isbn13 = isbn13.mid(3);
  // remove checksum
  isbn13.truncate(isbn13.length()-1);
  // add new checksum
  isbn13 += checkSum10(isbn13);
  fixup10(isbn13);
  return isbn13;
}

QString ISBNValidator::isbn13(QString isbn10) {
  isbn10.remove(QLatin1Char('-'));
  if(isbn10.length() < 10) {
    return isbn10;
  }
  if(isbn10.length() > 10) {
    // assume it's already an isbn13 value
    fixup13(isbn10);
    return isbn10;
  }
  // remove checksum
  isbn10.truncate(isbn10.length()-1);
  // begins with 978
  isbn10.prepend(QStringLiteral("978"));
  // add new checksum
  isbn10 += checkSum13(isbn10);
  fixup13(isbn10);
  return isbn10;
}

QStringList ISBNValidator::listDifference(const QStringList& list1_, const QStringList& list2_) {
  ISBNComparison comp;

  QStringList notFound;
  foreach(const QString& value1, list1_) {
    bool found = false;
    foreach(const QString& value2, list2_) {
      if(comp(value1, value2)) {
        found = true;
        break;
      }
    }
    if(!found) {
      notFound.append(value1);
    }
  }
  return notFound;
}

QString ISBNValidator::cleanValue(QString isbn) {
  static const QRegularExpression badChars(QStringLiteral("[^xX0123456789]"));
  isbn.remove(badChars);
  return isbn;
}

ISBNValidator::ISBNValidator(QObject* parent_)
    : QValidator(parent_) {
}

QValidator::State ISBNValidator::validate(QString& input_, int& pos_) const {
  // check if it's a cuecat first
  State catState = CueCat::decode(input_);
  if(catState != Invalid) {
    pos_ = input_.length();
    return catState;
  }

  if(input_.startsWith(QStringLiteral("978")) ||
     input_.startsWith(QStringLiteral("979"))) {
    return validate13(input_, pos_);
  } else {
    return validate10(input_, pos_);
  }
}

void ISBNValidator::fixup(QString& input_) const {
  staticFixup(input_);
}

void ISBNValidator::staticFixup(QString& input_) {
  static const QRegularExpression digits(QStringLiteral("\\d"));
  if((input_.startsWith(QStringLiteral("978"))
       || input_.startsWith(QStringLiteral("979")))
     && input_.count(digits) > 10) {
    fixup13(input_);
  } else {
    fixup10(input_);
  }
}

QValidator::State ISBNValidator::validate10(QString& input_, int& pos_) const {
  int len = input_.length();
/*
  // Don't do this since the hyphens may be in the wrong place, can't put that in a regexp
  if(isbn.exactMatch(input_) // put the exactMatch() first since I use matchedLength() later
     && (len == 12 || len == 13)
     && input_[len-1] == checkSum(input_)) {
    return QValidator::Acceptable;
  }
*/
  // two easy invalid cases are too many hyphens and the 'X' not in the last position
  if(input_.count(QLatin1Char('-')) > 3
     || input_.count(QLatin1Char('X'), Qt::CaseInsensitive) > 1
     || (input_.indexOf(QLatin1Char('X'), 0, Qt::CaseInsensitive) != -1 && input_[len-1].toUpper() != QLatin1Char('X'))) {
    return QValidator::Invalid;
  }

  // remember if the cursor is at the end
  bool atEnd = (pos_ == static_cast<int>(len));

  // fix the case where the user attempts to delete a character from a non-checksum
  // position; the solution is to delete the checksum, but only if it's X
  if(!atEnd && input_[len-1].toUpper() == QLatin1Char('X')) {
    input_.truncate(len-1);
    --len;
  }

  // fix the case where the user attempts to delete the checksum; the
  // solution is to delete the last digit as well
  static const QRegularExpression digit(QStringLiteral("\\d"));
  if(atEnd && input_.count(digit) == 9 && input_[len-1] == QLatin1Char('-')) {
    input_.truncate(len-2);
    pos_ -= 2;
  }

  // now fixup the hyphens and maybe add a checksum
  fixup10(input_);
  len = input_.length(); // might have changed in fixup()
  if(atEnd) {
    pos_ = len;
  }

  // first check to see if it's a "perfect" ISBN
  // A perfect ISBN has 9 digits plus either an 'X' or another digit
  // A perfect ISBN may have 2 or 3 hyphens
  // The final digit or 'X' is the correct check sum
  static const QRegularExpression isbn(QStringLiteral("^(\\d-?){9,11}-[\\dX]$"));
  if(isbn.match(input_).hasMatch() && (len == 12 || len == 13)) {
    return QValidator::Acceptable;
  } else {
    return QValidator::Intermediate;
  }
}

QValidator::State ISBNValidator::validate13(QString& input_, int& pos_) const {
  int len = input_.length();

  const uint countX = input_.count(QLatin1Char('X'), Qt::CaseInsensitive);
  // two easy invalid cases are too many hyphens or 'X'
  if(input_.count(QLatin1Char('-')) > 4 || countX > 1) {
    return QValidator::Invalid;
  }

  // now, it's not certain that we're getting a EAN-13,
  // it could be a ISBN-10 from Nigeria or Indonesia
  if(countX > 0 && (len > 13 || input_[len-1].toUpper() != QLatin1Char('X'))) {
    return QValidator::Invalid;
  }

  // remember if the cursor is at the end
  bool atEnd = (pos_ == len);

  // fix the case where the user attempts to delete a character from a non-checksum
  // position; the solution is to delete the checksum, but only if it's X
  if(!atEnd && input_[len-1].toUpper() == QLatin1Char('X')) {
    input_.truncate(len-1);
    --len;
  }

  // fix the case where the user attempts to delete the checksum; the
  // solution is to delete the last digit as well
  static const QRegularExpression digit(QStringLiteral("\\d"));
  const uint countN = input_.count(digit);
  if(atEnd && (countN == 12 || countN == 9) && input_[len-1] == QLatin1Char('-')) {
    input_.truncate(len-2);
    pos_ -= 2;
  }

  // now fixup the hyphens and maybe add a checksum
  if(countN > 10) {
    fixup13(input_);
  } else {
    fixup10(input_);
  }

  len = input_.length(); // might have changed in fixup()
  if(atEnd) {
    pos_ = len;
  }

  // first check to see if it's a "perfect" ISBN13
  // A perfect ISBN13 has 13 digits
  // A perfect ISBN13 may have 3 or 4 hyphens
  // The final digit is the correct check sum
  static const QRegularExpression isbn(QStringLiteral("^(\\d-?){13,17}$"));
  if(isbn.match(input_).hasMatch()) {
    return QValidator::Acceptable;
  } else {
    return QValidator::Intermediate;
  }
}

void ISBNValidator::fixup10(QString& input_) {
  if(input_.isEmpty()) {
    return;
  }

  //replace "x" with "X"
  input_.replace(QLatin1Char('x'), QLatin1Char('X'));

  // remove invalid chars
  static const QRegularExpression badChars(QStringLiteral("[^\\d\\-X]"));
  input_.remove(badChars);

  // special case for EAN values that start with 978 or 979. That's the case
  // for things like barcode readers that essentially 'type' the string at
  // once. The simulated typing has already caused the input to be normalized,
  // so strip that off, as well as the generated checksum. Then continue as normal.
  //  If someone were to input a regular 978- or 979- ISBN _including_ the
  // checksum, it will be regarded as barcode input and the input will be stripped accordingly.
  // I consider the likelihood that someone wants to input an EAN to be higher than someone
  // using a Nigerian ISBN and not noticing that the checksum gets added automatically.
  if(input_.length() > 12
     && (input_.startsWith(QStringLiteral("978"))
         || input_.startsWith(QStringLiteral("979")))) {
     // Strip the first 3 characters (the invalid publisher)
//     input_ = input_.right(input_.length() - 3);
  }

  // hyphen placement for some languages publishers is well-defined
  // remove all hyphens, and insert them ourselves
  // some countries have ill-defined second hyphen positions, and if
  // the user inserts one, then be sure to put it back

  // Find the first hyphen. If there is none,
  // input_.indexOf('-') returns -1 and hyphen1_position = -1
  int hyphen1_position = input_.indexOf(QLatin1Char('-'));

  // Find the second one. If none, hyphen2_position = -2
  int hyphen2_position = input_.indexOf(QLatin1Char('-'), hyphen1_position+1) - 1;

  // The second hyphen can not be in the last character
  if(hyphen2_position >= 9) {
    hyphen2_position = 0;
  }

  const bool hyphenAtEnd = input_.endsWith(QLatin1Char('-'));

  // Remove all existing hyphens. We will insert ours.
  input_.remove(QLatin1Char('-'));
  // the only place that 'X' can be is last spot
  for(int xpos = input_.indexOf(QLatin1Char('X')); xpos > -1; xpos = input_.indexOf(QLatin1Char('X'), xpos+1)) {
    if(xpos < 9) { // remove if not 10th char
      input_.remove(xpos, 1);
      --xpos;
    }
  }
  input_.truncate(10);

  // If we can find it, add the checksum
  // but only if not started with 978 or 979
  if(input_.length() > 8
     && !input_.startsWith(QStringLiteral("978"))
     && !input_.startsWith(QStringLiteral("979"))) {
    if(input_.length() == 9) input_.resize(10);
    input_[9] = checkSum10(input_);
  }

  ulong range = input_.leftJustified(9, QLatin1Char('0'), true).toULong();

  // now find which band the range falls in
  int band = 0;
  while(range >= bands[band].MaxValue) {
    ++band;
  }

  // if we have space to put the first hyphen, do it
  if(input_.length() > bands[band].First) {
    input_.insert(bands[band].First, QLatin1Char('-'));
  }

  //add 1 since one "-" character has already been inserted
  if(bands[band].Mid != 0) {
    hyphen2_position = bands[band].Mid;
    if(static_cast<int>(input_.length()) > (hyphen2_position + 1)) {
      input_.insert(hyphen2_position + 1, QLatin1Char('-'));
    }
  // or put back user's hyphen
  } else if(hyphen2_position > 0 && static_cast<int>(input_.length()) >= (hyphen2_position + 1)) {
    input_.insert(hyphen2_position + 1, QLatin1Char('-'));
  }

  // add a "-" before the checkdigit and another one if the middle "-" exists
  const int trueLast = bands[band].Last + 1 + (hyphen2_position > 0 ? 1 : 0);
  if(input_.length() > trueLast) {
    input_.insert(trueLast, QLatin1Char('-'));
  } else if(hyphenAtEnd && !input_.endsWith(QLatin1Char('-'))) {
    input_ += QLatin1Char('-');
  }
}

void ISBNValidator::fixup13(QString& input_) {
  if(input_.isEmpty()) {
    return;
  }

  // remove invalid chars
  static const QRegularExpression badChars(QStringLiteral("[^\\d-]"));
  input_.remove(badChars);

  // hyphen placement for some languages publishers is well-defined
  // remove all hyphens, and insert them ourselves
  // some countries have ill-defined second hyphen positions, and if
  // the user inserts one, then be sure to put it back

  QString after = input_.mid(3);
  if(after[0] == QLatin1Char('-')) {
    after = after.mid(1);
  }

  // Find the first hyphen. If there is none,
  // input_.indexOf('-') returns -1 and hyphen1_position = -1
  int hyphen1_position = after.indexOf(QLatin1Char('-'));

  // Find the second one. If none, hyphen2_position = -2
  int hyphen2_position = after.indexOf(QLatin1Char('-'), hyphen1_position+1) - 1;

  // The second hyphen can not be in the last characters
  if(hyphen2_position >= 9) {
    hyphen2_position = 0;
  }

  // Remove all existing hyphens. We will insert ours.
  after.remove(QLatin1Char('-'));
  after.truncate(10);

  // add the checksum
  if(after.length() > 8) {
    if(after.length() == 9) after.resize(10);
    after[9] = checkSum13(input_.left(3) + after);
  }

  ulong range = after.leftJustified(9, QLatin1Char('0'), true).toULong();

  // now find which band the range falls in
  int band = 0;
  while(range >= bands[band].MaxValue) {
    ++band;
  }

  // if we have space to put the first hyphen, do it
  if(after.length() > bands[band].First) {
    after.insert(bands[band].First, QLatin1Char('-'));
  }

  //add 1 since one "-" has already been inserted
  if(bands[band].Mid != 0) {
    hyphen2_position = bands[band].Mid;
    if(static_cast<int>(after.length()) > (hyphen2_position + 1)) {
      after.insert(hyphen2_position + 1, QLatin1Char('-'));
    }
  // or put back user's hyphen
  } else if(hyphen2_position > 0 && static_cast<int>(after.length()) >= (hyphen2_position + 1)) {
    after.insert(hyphen2_position + 1, QLatin1Char('-'));
  }

  // add a "-" before the checkdigit and another one if the middle "-" exists
  int trueLast = bands[band].Last + 1 + (hyphen2_position > 0 ? 1 : 0);
  if(after.length() > trueLast) {
    after.insert(trueLast, QLatin1Char('-'));
  }
  input_ = input_.left(3) + QLatin1Char('-') + after;
}

QChar ISBNValidator::checkSum10(const QString& input_) {
  uint sum = 0;
  uint multiplier = 10;

  // hyphens are already gone, only use first nine digits
  for(int i = 0; i < input_.length() && multiplier > 1; ++i) {
    sum += input_[i].digitValue() * multiplier--;
  }
  sum %= 11;
  sum = 11-sum;

  QChar c;
  if(sum == 10) {
    c = QLatin1Char('X');
  } else if(sum == 11) {
    c = QLatin1Char('0');
  } else {
    c = QString::number(sum)[0];
  }
  return c;
}

QChar ISBNValidator::checkSum13(const QString& input_) {
  uint sum = 0;

  const int len = qMin(12, input_.length());
  // hyphens are already gone, only use first twelve digits
  for(int i = 0; i < len; ++i) {
    sum += input_[i].digitValue() * (1 + 2*(i%2));
    // multiplier goes 1, 3, 1, 3, etc...
  }
  sum %= 10;
  sum = 10-sum;

  QChar c;
  if(sum == 10) {
    c = QLatin1Char('0');
  } else {
    c = QString::number(sum)[0];
  }
  return c;
}

// ISBN code from Regis Boudin
#define ISBNGRP_1DIGIT(digit, max, middle, last)        \
          {((digit)*100000000) + (max), 1, middle, last}
#define ISBNGRP_2DIGIT(digit, max, middle, last)        \
          {((digit)*10000000) + ((max)/10), 2, middle, last}
#define ISBNGRP_3DIGIT(digit, max, middle, last)        \
          {((digit)*1000000) + ((max)/100), 3, middle, last}
#define ISBNGRP_4DIGIT(digit, max, middle, last)        \
          {((digit)*100000) + ((max)/1000), 4, middle, last}
#define ISBNGRP_5DIGIT(digit, max, middle, last)        \
          {((digit)*10000) + ((max)/10000), 5, middle, last}

#define ISBNPUB_2DIGIT(grp) (((grp)+1)*1000000)
#define ISBNPUB_3DIGIT(grp) (((grp)+1)*100000)
#define ISBNPUB_4DIGIT(grp) (((grp)+1)*10000)
#define ISBNPUB_5DIGIT(grp) (((grp)+1)*1000)
#define ISBNPUB_6DIGIT(grp) (((grp)+1)*100)
#define ISBNPUB_7DIGIT(grp) (((grp)+1)*10)
#define ISBNPUB_8DIGIT(grp) (((grp)+1)*1)

// how to format an ISBN, after categorising it into a range of numbers.
struct ISBNValidator::isbn_band ISBNValidator::bands[] = {
  /* Groups 0 & 1 : English */
  ISBNGRP_1DIGIT(0,     ISBNPUB_2DIGIT(19),      3, 9),
  ISBNGRP_1DIGIT(0,     ISBNPUB_3DIGIT(699),     4, 9),
  ISBNGRP_1DIGIT(0,     ISBNPUB_4DIGIT(8499),    5, 9),
  ISBNGRP_1DIGIT(0,     ISBNPUB_5DIGIT(89999),   6, 9),
  ISBNGRP_1DIGIT(0,     ISBNPUB_6DIGIT(949999),  7, 9),
  ISBNGRP_1DIGIT(0,     ISBNPUB_7DIGIT(9999999), 8, 9),

  ISBNGRP_1DIGIT(1,     ISBNPUB_5DIGIT(54999),   6, 9),
  ISBNGRP_1DIGIT(1,     ISBNPUB_5DIGIT(86979),   6, 9),
  ISBNGRP_1DIGIT(1,     ISBNPUB_6DIGIT(998999),  7, 9),
  ISBNGRP_1DIGIT(1,     ISBNPUB_7DIGIT(9999999), 8, 9),
  /* Group 2 : French */
  ISBNGRP_1DIGIT(2,     ISBNPUB_2DIGIT(19),      3, 9),
  ISBNGRP_1DIGIT(2,     ISBNPUB_3DIGIT(349),     4, 9),
  ISBNGRP_1DIGIT(2,     ISBNPUB_5DIGIT(39999),   6, 9),
  ISBNGRP_1DIGIT(2,     ISBNPUB_3DIGIT(699),     4, 9),
  ISBNGRP_1DIGIT(2,     ISBNPUB_4DIGIT(8399),    5, 9),
  ISBNGRP_1DIGIT(2,     ISBNPUB_5DIGIT(89999),   6, 9),
  ISBNGRP_1DIGIT(2,     ISBNPUB_6DIGIT(949999),  7, 9),
  ISBNGRP_1DIGIT(2,     ISBNPUB_7DIGIT(9999999), 8, 9),

  /* Group 2 : German */
  ISBNGRP_1DIGIT(3,     ISBNPUB_2DIGIT(19),      3, 9),
  ISBNGRP_1DIGIT(3,     ISBNPUB_3DIGIT(699),     4, 9),
  ISBNGRP_1DIGIT(3,     ISBNPUB_4DIGIT(8499),    5, 9),
  ISBNGRP_1DIGIT(3,     ISBNPUB_5DIGIT(89999),   6, 9),
  ISBNGRP_1DIGIT(3,     ISBNPUB_6DIGIT(949999),  7, 9),
  ISBNGRP_1DIGIT(3,     ISBNPUB_7DIGIT(9999999), 8, 9),

  ISBNGRP_1DIGIT(7,     ISBNPUB_2DIGIT(99),      0, 9),
  /* Group 80 : Czech */
  ISBNGRP_2DIGIT(80,    ISBNPUB_2DIGIT(19),      4, 9),
  ISBNGRP_2DIGIT(80,    ISBNPUB_3DIGIT(699),     5, 9),
  ISBNGRP_2DIGIT(80,    ISBNPUB_4DIGIT(8499),    6, 9),
  ISBNGRP_2DIGIT(80,    ISBNPUB_5DIGIT(89999),   7, 9),
  ISBNGRP_2DIGIT(80,    ISBNPUB_6DIGIT(949999),  8, 9),

  /* Group 83 : Poland */
  ISBNGRP_2DIGIT(83,    ISBNPUB_2DIGIT(19),      4, 9),
  ISBNGRP_2DIGIT(83,    ISBNPUB_3DIGIT(599),     5, 9),
  ISBNGRP_2DIGIT(83,    ISBNPUB_5DIGIT(69999),   7, 9),
  ISBNGRP_2DIGIT(83,    ISBNPUB_4DIGIT(8499),    6, 9),
  ISBNGRP_2DIGIT(83,    ISBNPUB_5DIGIT(89999),   7, 9),
  ISBNGRP_2DIGIT(83,    ISBNPUB_6DIGIT(949999),  8, 9),

  /* Group 90 * Netherlands */
  ISBNGRP_2DIGIT(90,    ISBNPUB_2DIGIT(19),      4, 9),
  ISBNGRP_2DIGIT(90,    ISBNPUB_3DIGIT(499),     5, 9),
  ISBNGRP_2DIGIT(90,    ISBNPUB_4DIGIT(6999),    6, 9),
  ISBNGRP_2DIGIT(90,    ISBNPUB_5DIGIT(79999),   7, 9),
  ISBNGRP_2DIGIT(90,    ISBNPUB_6DIGIT(849999),  8, 9),
  ISBNGRP_2DIGIT(90,    ISBNPUB_4DIGIT(8999),    6, 9),
  ISBNGRP_2DIGIT(90,    ISBNPUB_7DIGIT(9999999), 9, 9),

  ISBNGRP_2DIGIT(94,    ISBNPUB_2DIGIT(99),      0, 9),
  ISBNGRP_3DIGIT(993,   ISBNPUB_2DIGIT(99),      0, 9),
  ISBNGRP_4DIGIT(9989,  ISBNPUB_2DIGIT(99),      0, 9),
  ISBNGRP_5DIGIT(99999, ISBNPUB_2DIGIT(99),      0, 9)
};

bool Tellico::ISBNComparison::operator()(const QString& value1_, const QString& value2_) const {
  QString value1 = ISBNValidator::cleanValue(value1_).toUpper();
  QString value2 = ISBNValidator::cleanValue(value2_).toUpper();

  if(value1 == value2) {
    return true;
  }
  const int len1 = value1.length();
  const int len2 = value2.length();
  if(len1 < 10 || len2 < 10) {
    // they're not ISBN values at all
    return false;
  }
  if(len1 == 13) {
    ISBNValidator::fixup13(value1);
  } else {
    value1 = ISBNValidator::isbn13(value1);
  }
  if(len2 == 13) {
    ISBNValidator::fixup13(value2);
  } else {
    value2 = ISBNValidator::isbn13(value2);
  }
  return value1 == value2;
}
