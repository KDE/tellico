/***************************************************************************
                          isbnvalidator.cpp  -  description
                             -------------------
    begin                : Sun Oct 6 2002
    copyright            : (C) 2002 by Robby Stephenson
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

  // an ISBN must be less than 13 characters
  // only allow digits, hyphens, and "X" or "x"
  // only allow maximum of three hyphens
  // only allow up to a single "X" or "x"
  // the "X" or "x" can only be the last character
  // only allow up to 10 digits
  QRegExp validChars(QString::fromLatin1("[\\d-X]{0,13}"));
  if(!validChars.exactMatch(input_)) {
    QRegExp rx(QString::fromLatin1("[^\\d-X]"));
    input_.replace(rx, QString::null);
    input_.truncate(13);
    while(pos_ > input_.length()) {
      --pos_;
    }
  }
  if(input_.contains(QString::fromLatin1("-")) > 3
      || input_.contains(QString::fromLatin1("X"), false) > 1
      || (input_.find(QString::fromLatin1("X"), 0, false) != -1
          && input_.find(QString::fromLatin1("X"), 0, false) < input_.length()-1)
      || input_.contains(QRegExp(QString::fromLatin1("\\d"))) > 10) {
    return QValidator::Invalid;
  }
  // remember if the cursor is at the end
  bool atEnd = (pos_ == input_.length());
  // fix the case where the user attempts to delete a character from a non-checksum
  // position; the solution is to delete the checksum, but only if it's X
  if(pos_ != input_.length()
      && input_.contains(QRegExp(QString::fromLatin1("-[Xx]$")))) {
    input_.truncate(input_.length()-2);
  }
  // fix the case where the user attempts to delete the checksum; the
  // solution is to delete the last digit as well
  if(pos_ == input_.length()
      && input_.contains(QRegExp(QString::fromLatin1("\\d"))) == 9
      && input_[pos_-1] == '-') {
    input_.truncate(input_.length()-2);
    pos_ -= 2;
  }
  // now fixup the hyphens and maybe add a checksum
  fixup(input_);
  if(atEnd) {
    pos_ = input_.length();
  }
  
  QRegExp re(QString::fromLatin1("(\\d-?){9}[\\dX]"));
  if(re.exactMatch(input_)) {
    return QValidator::Acceptable;
  } else {
    return QValidator::Intermediate;
  }
}

void ISBNValidator::fixup(QString& input_) const {
  //replace "x" with "X"
  input_.replace(QRegExp(QString::fromLatin1("x")), QString::fromLatin1("X"));
  // remove dashes
  input_.replace(QRegExp(QString::fromLatin1("-")), QString::null);
  // only add the checksum if more than 8 digits are present
  if(input_.length() > 8) {
    checkSum(input_);
  }
  insertDashes(input_);
}

void ISBNValidator::checkSum(QString& input_) const {
  int sum = 0;
  int multiplier = 10;

  input_ = input_.left(9);
  for(int i = 0; i < 9; ++i) {
    sum += input_[i].digitValue() * multiplier--;
  }
  sum %= 11;
  sum = 11-sum;
  if(sum == 10) {
    input_.append(QString::fromLatin1("X"));
  } else if(sum == 11) {
    input_.append(QString::fromLatin1("0"));
  } else {
    input_.append(QString::number(sum));
  }
}

void ISBNValidator::insertDashes(QString& input_) const{
  int range = input_.leftJustify(9, '0', true).toInt();
  uint whereFirstDash = 0;
  uint whereMidDash = 0;
  uint whereLastDash = 9;
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
    200000000,
    // -------10  2-rrrrrrrr-x groups 2 .. 7
    800000000,
    // -------11  80-rrrrrrr-x groups 80 .. 94
    950000000,
    // -------12  950-rrrrrr-x groups 950 .. 995
    996000000,
    // -------13  9960-rrrrr-x groups 9960 .. 9989
    999000000,
    // -------14   groups 99900 .. 99999
    1000000000
  };
  int band = 0;
  for(uint i = 0; i < sizeof(bands)/sizeof(int); ++i) {
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
    case 10: /* group codes 2 .. 7 : 2-rrrrrrrr-x */
      /* leave out dash between publisher and title */
      whereFirstDash = 1;
      break;
    case 11: /* group codes 80 .. 94:  80-rrrrrrr-x */
      /* leave out dash between publisher and title  */
      whereFirstDash = 2;
      break;
    case 12: /* group codes 950..995 : 950-rrrrrr-x */
      /* leave out dash between publisher and title */
      whereFirstDash = 3;
      break;
    case 13: /* group codes 9960 .. 9989 : 9960-rrrrr-x */
      /* leave out dash between publisher and title */
      whereFirstDash = 4;
      break;
    case 14: /* group codes 99900 .. 99999 : 99900-rrrr-x */
      whereFirstDash = 5;
      break;
    }
  if(input_.length() > whereFirstDash) {
    input_.insert(whereFirstDash, QString::fromLatin1("-"));
    // 0 means the middle dash is not to be inserted
    if(whereMidDash > 0) {
      ++whereMidDash;
    }
    ++whereLastDash;
  }
  if(whereMidDash > 0 && input_.length() > whereMidDash) {
    //add 1 since one "-" has already been inserted
    input_.insert(whereMidDash, QString::fromLatin1("-"));
    ++whereLastDash;
  }
  // add a "-" before the checkdigit
  if(input_.length() > whereLastDash) {
    input_.insert(whereLastDash, QString::fromLatin1("-"));
  }
}

// not needed yet
#if 0

void ISBNValidator::buildValidGroupLookup() {
  validGroupLookup.resize(100000);
  for(int i = 0; i < numGroups; ++i) {
    int group = validGroups[i];
    int width = QString::number(group).length();
    switch(width) {
      case 5:
        validGroupLookup.setBit(group);
        break;
      case 4:
        for(int j = group*10; j < group*10+9; ++j) {
          validGroupLookup.setBit(j);
        }
        break;
      case 3:
        for(int j = group*100; j < group*100+99; ++j) {
          validGroupLookup.setBit(j);
        }
        break;
      case 2:
        for(int j = group*1000; j < group*1000+999; ++j) {
          validGroupLookup.setBit(j);
        }
        break;
      case 1:
        for(int j = group*10000; j < group*10000+9999; ++j) {
          validGroupLookup.setBit(j);
        }
        break;
      default:
        kdDebug() << "ISBNValidator::buildValidGroupLookup() - invalid group length" << endl;
        break;
    }
  }
}

/* List of legal groups as of 5 October 2002
   Taken from http://www.isbn.spk-berlin.de/html/prefix/allpref.htm  */
const long ISBNValidator::validGroups[] = {
      0,
      1,
      2,
      3,
      4,
      5,
      7,
      80,
      81,
      82,
      83,
      84,
      85,
      86,
      87,
      88,
      89,
      90,
      91,
      92,
      93,
      950,
      951,
      952,
      953,
      954,
      955,
      956,
      957,
      958,
      959,
      960,
      961,
      962,
      963,
      964,
      965,
      966,
      967,
      968,
      969,
      970,
      971,
      972,
      973,
      974,
      975,
      976,
      977,
      978,
      979,
      980,
      981,
      982,
      983,
      984,
      985,
      986,
      987,
      988,
      989,
      9948,
      9949,
      9950,
      9951,
      9952,
      9953,
      9954,
      9955,
      9956,
      9957,
      9958,
      9959,
      9960,
      9961,
      9962,
      9963,
      9964,
      9965,
      9966,
      9967,
      9968,
      9970,
      9971,
      9972,
      9973,
      9974,
      9975,
      9976,
      9977,
      9978,
      9979,
      9980,
      9982,
      9983,
      9984,
      9985,
      9986,
      9987,
      9988,
      9989,
      99901,
      99903,
      99904,
      99905,
      99906,
      99908,
      99909,
      99910,
      99911,
      99912,
      99913,
      99914,
      99915,
      99916,
      99917,
      99918,
      99919,
      99920,
      99921,
      99922,
      99923,
      99924,
      99925,
      99926,
      99927,
      99928,
      99929,
      99930,
      99931,
      99932,
      99933,
      99934,
      99935,
      99936,
      99937,
      99938,
      99939,
      99940
   };

const int ISBNValidator::numGroups = sizeof(ISBNValidator::validGroups)/sizeof(long);

#endif
