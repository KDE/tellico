/***************************************************************************
    copyright            : (C) 2002-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "isbnvalidator.h"

#include <kdebug.h>

using Bookcase::ISBNValidator;

ISBNValidator::ISBNValidator(QObject* parent_, const char* name_/*=0*/)
    : QValidator(parent_, name_) {
}

QValidator::State ISBNValidator::validate(QString& input_, int& pos_) const {
  //kdDebug() << "ISBNValidator::validate() - " << input_ << endl;
  // could use QRegExp("(\\d-?){9}[\\dXx]") but it causes problems.
  // For example, if "X" is the checksum and the user attempts to delete a
  // digit from the number, the validator would not allow it because the "X" would
  // then be in an invalid position. So allow an X anywhere for a string with less
  // than 10 digits

  //replace "x" with "X"
  input_.replace('x', QString::fromLatin1("X"));

  // an ISBN must no more than 13 characters
  // only allow digits, hyphens, and "X"
  // only allow maximum of three hyphens
  // only allow up to a single "X"
  // only allow up to 10 digits
  // the "X" can only be the last character
  static const QRegExp validChars(QString::fromLatin1("[\\d-X]{0,12}"));
  static QRegExp rx(QString::fromLatin1("[^\\d-X]"));
  if(!validChars.exactMatch(input_)) {
    input_.replace(rx, QString::null);
    // special case for EAN values that start with 978 or 979. That's the case
    // for things like barcode readers that essentially 'type' the string at
    // once. The simulated typing has already caused the input to be normalized,
    // so strip that off, as well as the generated checksum. Then continue as normal.
    //  If someone were to input a regular 978- or 979- ISBN _including_ the
    // checksum, it will be regarded as barcode input and the input will be stripped accordingly.
    // I consider the likelihood that someone wants to input an EAN to be higher than someone
    // using a Nigerian ISBN and not noticing that the checksum gets added automatically.
    if(input_.length() > 12
       && (input_.startsWith(QString::fromLatin1("978-")))
           || input_.startsWith(QString::fromLatin1("979-"))) {
      // Strip the first 4 characters (the invalid publisher)
      input_ = input_.right(input_.length() - 4);
      // Remove the checksum
      input_ = input_.left(6) + input_.right(1);
    } else {
      input_.truncate(13);
    }
    while(pos_ > static_cast<int>(input_.length())) {
      --pos_;
    }
  }
  int len = input_.length();

  // special case for EAN values that start with 978 or 979
  // only convert if 13 digits in string, that's the case for things
  // like barcode readers that essentially paste the whole string at once
  if(input_.contains(QRegExp(QString::fromLatin1("[\\dX]"))) == 13
     && (input_.startsWith(QString::fromLatin1("978")))
         || input_.startsWith(QString::fromLatin1("979"))) {
    input_ = input_.right(len-3);
    len -= 3;
    pos_ = QMAX(0, pos_-3);
  }

  if(input_.contains(QString::fromLatin1("-")) > 3
      || input_.contains(QString::fromLatin1("X"), false) > 1
      || (input_.find(QString::fromLatin1("X"), 0, false) != -1
          && input_.find(QString::fromLatin1("X"), 0, false) < len-1)
      || input_.contains(QRegExp(QString::fromLatin1("\\d"))) > 10) {
    return QValidator::Invalid;
  }
  // remember if the cursor is at the end
  bool atEnd = (pos_ == len);
  // fix the case where the user attempts to delete a character from a non-checksum
  // position; the solution is to delete the checksum, but only if it's X
  if(!atEnd && input_.contains(QRegExp(QString::fromLatin1("-X$")))) {
    input_.truncate(input_.length()-2);
    len -= 2;
  }
  // fix the case where the user attempts to delete the checksum; the
  // solution is to delete the last digit as well
  if(pos_ == len
     && input_.contains(QRegExp(QString::fromLatin1("\\d"))) == 9
     && input_[pos_-1] == '-') {
    input_.truncate(input_.length()-2);
    pos_ -= 2;
    len -= 2;
  }
  // now fixup the hyphens and maybe add a checksum
  fixup(input_);
  if(atEnd) {
    // can't use len here since it might have changed in fixup()
    pos_ = input_.length();
  }

  static QRegExp re(QString::fromLatin1("[\\d-]{0,11}-[\\dX]"));
  if(re.exactMatch(input_)) {
    return QValidator::Acceptable;
  } else {
    return QValidator::Intermediate;
  }
}

void ISBNValidator::fixup(QString& input_) const {
  if(input_.isEmpty()) {
    return;
  }

  // only add the checksum if more than 8 digits are present
  QString digits = input_;
  digits.replace('-', QString::null);
  if(digits.length() > 9) {
    digits.truncate(9);
  }
  if(digits.length() > 8) {
    digits += checkSum(digits);
  }
  insertDashes(input_, digits);
}

QChar ISBNValidator::checkSum(const QString& input_) const {
#ifndef NDEBUG
  if(input_.length() != 9) {
    kdDebug() << "ISBNValidator::checkSum() - only supposed to call with 9 digits!" << endl;
  }
#endif
  unsigned sum = 0;
  unsigned multiplier = 10;

  // hyphens are already gone
  for(unsigned i = 0; i < 9; ++i) {
    sum += input_[i].digitValue() * multiplier--;
  }
  sum %= 11;
  sum = 11-sum;
  if(sum == 10) {
    return QChar('X');
  } else if(sum == 11) {
    return QChar('0');
  } else {
    return QString::number(sum)[0];
  }
}

void ISBNValidator::insertDashes(QString& input_, const QString& digits_) const {
  static const QRegExp rxDigit(QString::fromLatin1("\\d"));
  // hyphen placement for some language's publishers is well-defined
  unsigned long range = digits_.leftJustify(9, '0', true).toInt();
  unsigned whereFirstDash = 0;
  unsigned whereMidDash = 10;
  unsigned whereLastDash = 9;
  // how to format an ISBN, after categorising it into a range of numbers.
  // number is high+1 for the band.
  static const struct isbn_band {
    unsigned long MaxValue;
    unsigned First;
    unsigned Mid;
    unsigned Last;
  } bands[] = {
    /* high+1    First Middle Last */
    /* Groups 0 & 1 : English */
    { 20000000,  1,    3,     9}, /* Grp 0 Publishers 00-19 */
    { 70000000,  1,    4,     9}, /* Grp 0 Publishers 200-699 */
    { 85000000,  1,    5,     9}, /* Grp 0 Publishers 7000-8499 */
    { 90000000,  1,    6,     9}, /* Grp 0 Publishers 85000-89999 */
    { 95000000,  1,    7,     9}, /* Grp 0 Publishers 900000-949999 */
    {100000000,  1,    8,     9}, /* Grp 0 Publishers 9500000-9999999 */
    {155000000,  1,    6,     9}, /* Grp 1 Publishers 00000-54999 (illegal) */
    {186980000,  1,    6,     9}, /* Grp 1 Publishers 55000-86979 */
    {199900000,  1,    7,     9}, /* Grp 1 Publishers 969800-998999 */
    {200000000,  1,    8,     9}, /* Grp 1 Publishers 9900000-9999999 */

    /* Group 2 : French */
    {220000000,  1,    3,     9}, /* Grp 2 Publishers 00-19 */
    {234000000,  1,    4,     9}, /* Grp 2 Publishers 200-349 */
    {240000000,  1,    6,     9}, /* Grp 2 Publishers 35000-39999 */
    {270000000,  1,    4,     9}, /* Grp 2 Publishers 400-699 */
    {284000000,  1,    5,     9}, /* Grp 2 Publishers 7000-8399 */
    {290000000,  1,    6,     9}, /* Grp 2 Publishers 84000-89999 */
    {295000000,  1,    7,     9}, /* Grp 2 Publishers 900000-949999 */
    {300000000,  1,    10,    9}, /* Grp 2 Publishers 9500000-9999999 */

    /* Languages with unknown rules */
    {800000000,  1,    10,    9}, /* Groups 3 - 7 */
    {950000000,  2,    10,    9}, /* Groups 80 - 94 */
    {994000000,  3,    10,    9}, /* Groups 950 - 993 */
    {999000000,  4,    10,    9}, /* Groups 9940 - 9989 */
    {1000000000, 5,    10,    9}  /* Groups 99900 - 99999 */
  };

  bool keepUserHyphens = false;
  for(unsigned i = 0; i < sizeof(bands)/sizeof(struct isbn_band); ++i) {
    if(range < bands[i].MaxValue) {
      whereFirstDash = bands[i].First;
      whereMidDash = bands[i].Mid;
      whereLastDash = bands[i].Last;
      if(range >= 300000000) { // change this if other language's publishers are added
        keepUserHyphens = true;
      }
      break;
    }
  }

  if(!keepUserHyphens) {
    QString isbn = digits_;
    if(isbn.length() > whereFirstDash) {
      isbn.insert(whereFirstDash, '-');
      ++whereMidDash;
      ++whereLastDash;
    }
    if(isbn.length() > whereMidDash) {
      isbn.insert(whereMidDash, '-');
      ++whereLastDash;
    }
    if(isbn.length() > whereLastDash) {
      isbn.insert(whereLastDash, '-');
    }
    // if we're not keeping any user hyphens, we're done.
    input_ = isbn;
    return;
  }

  // now the user may have added a hyphen somewhere between the publisher and title parts
  // this is really dumb code but seems to work
  QString group = digits_.left(whereFirstDash); // group string
  if(group == digits_) {
    return;
  }

  // now want to find the position in the input string where the group ends
  unsigned pos1 = 0;
  for(unsigned i = 0; i < input_.length(); ++i) {
    if(input_[i] == group[pos1]) {
      ++pos1;
    }
    if(pos1 == group.length()) {
      pos1 = i+1;
      break;
    }
  }
  // now pos1 is the position in the input string where the group ends
  // shift by one, so that now, pos1 is where the rest of the string starts
  if(input_[pos1] == '-') {
    ++pos1;
  }

  int pos2 = input_.find('-', pos1+1);
  if(pos2 == -1) {
    QString tail = input_.mid(pos1);
    if(group.length() + tail.length() < 9) {
      input_ = group + '-' + tail;
    } else {
      input_ = group + '-' + tail.left(9-group.length()) + '-' + digits_[9];
    }
    return;
  }

  ++pos2; // since it points to a hyphen
  // is the second hyphen the one before the check sum or is there a third hyphen?
  int pos3 = input_.find('-', pos2+1);

  QString middle;
  if(pos3 == -1) {
    middle = input_.mid(pos1);
  } else {
    middle = input_.mid(pos1, pos3-pos1);
  }

  if(group.length() + middle.contains(rxDigit) < 9) {
    input_ = group + '-' + middle;
  } else {
    input_ = group + '-' + middle.left(9-group.length()) + '-' + digits_[9];
  }
}
