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
//  buildValidGroupLookup();
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
  QRegExp validChars(QString::fromLatin1("[\\d-X]{0,12}"));
  if(!validChars.exactMatch(input_)) {
    QRegExp rx(QString::fromLatin1("[^\\d-X]"));
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

  QRegExp re(QString::fromLatin1("[\\d-]{0,11}-[\\dX]"));
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

  if(input_[0] == '0' || input_[0] == '1') {
    // english-speaking group
    input_.replace(QRegExp(QString::fromLatin1("-")), QString::null);
    // only add the checksum if more than 8 digits are present
    if(input_.length() > 8) {
      input_[9] = checkSum(input_);
    }
    insertDashesEnglish(input_);
    return;
  } else {
    insertDashesNonEnglish(input_);
  }
}

QChar ISBNValidator::checkSum(const QString& input_) const {
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

void ISBNValidator::insertDashesNonEnglish(QString& input_) const {
/* for non-english-language publishers, the first and last hyphen positions are well-defined
   but the middle one isn't. Poossible publisher groups are
                           0 - 7
                          80 - 94
                         950 - 993
                        9940 - 9989
                       99900 - 99999
*/
  // first grab digits
  QString digits = input_;
  digits.replace('-', QString::null);

  int whereFirstDash;
  if(digits.left(1).toInt() < 8) {
    whereFirstDash = 1;
  } else if(digits.left(2).toInt() < 95) {
    whereFirstDash = 2;
  } else if(digits.left(3).toInt() < 994) {
    whereFirstDash = 3;
  } else if(digits.left(4).toInt() < 9990) {
    whereFirstDash = 4;
  } else {
    whereFirstDash = 5;
  }

  QString pub = digits.left(whereFirstDash);
  if(pub == digits) {
    return;
  }

  // now want to find the position in the input string where the publisher group ends
  unsigned pos1 = 0;
  for(unsigned i = 0; i < input_.length(); ++i) {
    if(input_[i] == pub[pos1]) {
      ++pos1;
    }
    if(pos1 == pub.length()) {
      pos1 = i+1;
      break;
    }
  }
  // now pos1 is the position in the input string where the publisher group ends
  // shift by one, so that now, pos1 is where the rest of the string starts
  if(input_[pos1] == '-') {
    ++pos1;
  }

  int pos2 = input_.find('-', pos1+1);
  if(pos2 == -1) {
    QString tail = input_.mid(pos1);
    if(pub.length() + tail.length() < 9) {
      input_ = pub + QString::fromLatin1("-") + tail;
    } else {
      input_ = pub + QString::fromLatin1("-") + tail.left(10-pub.length()) + QString::fromLatin1("-") + checkSum(digits);
    }
    return;
  }

  // is the second hyphen the one before the check sum or is there a third hyphen?
  int pos3 = input_.find('-', pos2+1);

  QString middle;
  if(pos3 == -1) {
    middle = input_.mid(pos1, pos2-pos1);
  } else {
    middle = input_.mid(pos1, pos3-pos1);
  }

  if(pub.length() + middle.contains(QRegExp(QString::fromLatin1("\\d"))) < 9) {
    input_ = pub + QString::fromLatin1("-") + middle;
    return;
  }
  input_ = pub + QString::fromLatin1("-") + middle + QString::fromLatin1("-") + checkSum(digits);
}

void ISBNValidator::insertDashesEnglish(QString& input_) const {
  // hyphen placement for english-language publishers is well-defined
  // remove all hyphens, and insert ourselves
  input_.replace('-', QString::null);
  int range = input_.leftJustify(9, '0', true).toInt();
  unsigned whereFirstDash = 0;
  unsigned whereMidDash = 0;
  unsigned whereLastDash = 9;
  // how to format an ISBN, after categorising it into a range of numbers.
  // number is high+1 for the band.
  int bands[] = {
    // --------0  0-00-bbbbbb-x group 0
    20000000,
    // --------1  0-200-bbbbb-x
    70000000,
    // --------2  0-7000-bbbb-x
    85000000,
    // --------3  0-85000-bbb-x
    90000000,
    // --------4  0-90000-bb-x
    95000000,
    // --------5  0-950000-b-x
    100000000,
    // --------6  1-1000-bbbb-x  group 1
    155000000,
    // --------7  1-55000-bbb-x
    186980000,
    // --------8  1-869800-bb-x
    199900000,
    // --------9  1-1999000-b-x
    200000000
  };
  unsigned band = 0;
  for(unsigned i = 0; i < sizeof(bands)/sizeof(int); ++i) {
    if(range < bands[i]) {
      band = i;
      break;
    }
  }
  switch(band) {
    /* cases 0..5 handle the standard publisher pattern, for group 0 */
    case 0:
      /* publisher 00 .. 19 : 0-00-bbbbbb-x */
      whereFirstDash = 1;
      whereMidDash = whereFirstDash + 2;
      break;
    case 1:
      /* publisher 200 .. 699 : 0-200-bbbbb-x */
      whereFirstDash = 1;
      whereMidDash = whereFirstDash + 3;
      break;
    case 2:
      /* publisher 7000 .. 8499 : 0-7000-bbbb-x */
      whereFirstDash = 1;
      whereMidDash = whereFirstDash + 4;
      break;
    case 3:
      /* publisher 85000 .. 89999 : 0-85000-bbb-x  */
      whereFirstDash = 1;
      whereMidDash = whereFirstDash + 5;
      break;
    case 4:
      /* publisher 900000 .. 94999 : 0-90000-bb-x */
      whereFirstDash = 1;
      whereMidDash = whereFirstDash + 6;
      break;
    case 5:
      /* publisher 9500000 .. 9999999 : 0-950000-b-x */
      whereFirstDash = 1;
      whereMidDash = whereFirstDash + 7;
      break;
      /* cases 6..9 : 1-1000-bbbb-x handle nonstandard publisher pattern of group-1 */
    case 6:
      whereFirstDash = 1;
      whereMidDash = 5;
      break;
    case 7: /*  1-55000-bbb-x  */
      whereFirstDash = 1;
      whereMidDash = 6;
      break;
    case 8:  /* 1-55000-bbb-x */
      whereFirstDash = 1;
      whereMidDash = 7;
      break;
    case 9: /* 1-1999000-b-x */
      whereFirstDash = 1;
      whereMidDash = 8;
      break;
  }

  if(input_.length() > whereFirstDash) {
    input_.insert(whereFirstDash, '-');
    ++whereMidDash;
    ++whereLastDash;
  }
  if(input_.length() > whereMidDash) {
    //add 1 since one "-" has already been inserted
    input_.insert(whereMidDash, '-');
    ++whereLastDash;
  }
  // add a "-" before the checkdigit
  if(input_.length() > whereLastDash) {
    input_.insert(whereLastDash, '-');
  }
}
