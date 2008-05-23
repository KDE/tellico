/***************************************************************************
    copyright            : (C) 2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "lccnvalidator.h"
#include "tellico_debug.h"

using Tellico::LCCNValidator;

LCCNValidator::LCCNValidator(QObject* parent_) : QRegExpValidator(parent_) {
  QRegExp rx(QString::fromLatin1("[a-z ]{0,3}"
                                  "("
                                  "\\d{2}-?\\d{1,6}"
                                  "|"
                                  "\\d{4}-?\\d{1,6}"
                                  ")"
                                  " ?\\w*"));
  setRegExp(rx);
}

// static
QString LCCNValidator::formalize(const QString& value_) {
  const int len = value_.length();
  // first remove alpha prefix
  QString alpha;
  for(int pos = 0; pos < len; ++pos) {
    QChar c = value_.at(pos);
    if(c.isNumber()) {
      break;
    }
    alpha += value_.at(pos);
  }
  QString afterAlpha = value_.mid(alpha.length());
  alpha = alpha.stripWhiteSpace(); // possible to have a space at the end

  QString year;
  QString serial;
  // have to be able to differentiate 2 and 4-digit years, first check for hyphen position
  int pos = afterAlpha.find('-');
  if(pos > -1) {
    year = afterAlpha.section('-', 0, 0);
    serial = afterAlpha.section('-', 1);
  } else {
    // make two assumptions, the user will never have a book from the year 1920
    // or from any year after 2100. Reasonable, right?
    // so if the string starts with '20' we take the first 4 digits as the year
    // otherwise the first 2
    if(afterAlpha.startsWith(QString::fromLatin1("20"))) {
      year = afterAlpha.left(4);
      serial = afterAlpha.mid(4);
    } else {
      year = afterAlpha.left(2);
      serial = afterAlpha.mid(2);
    }
  }

  // now check for non digits in the serial
  pos = 0;
  for( ; pos < serial.length() && serial.at(pos).isNumber(); ++pos) { ; }
  QString suffix = serial.mid(pos);
  serial = serial.left(pos);
  // serial must be left-padded with zeros to 6 characters
  serial = serial.rightJustify(6, '0');
  return alpha + year + serial + suffix;
}
