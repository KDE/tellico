/***************************************************************************
    Copyright (C) 2008-2020 Robby Stephenson <robby@periapsis.org>
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

#include "lccnvalidator.h"
#include "../tellico_debug.h"

using Tellico::LCCNValidator;

LCCNValidator::LCCNValidator(QObject* parent_) : QRegularExpressionValidator(parent_) {
  QRegularExpression rx(QStringLiteral("[a-z ]{0,3}"
                                       "("
                                       "\\d{2}-?\\d{1,6}"
                                       "|"
                                       "\\d{4}-?\\d{1,6}"
                                       ")"
                                       " ?\\w*"));
  setRegularExpression(rx);
}

// static
QString LCCNValidator::formalize(const QString& value_) {
  QString value = value_.simplified();
  // remove spaces
  value.remove(QLatin1Char(' '));
  // remove everything after the forward slash
  value = value.section(QLatin1Char('/'), 0, 0);

  const int len = value_.length();
  // first remove alpha prefix
  QString alpha;
  for(int pos = 0; pos < len; ++pos) {
    QChar c = value.at(pos);
    if(c.isNumber()) {
      break;
    }
    alpha += value.at(pos);
  }
  QString afterAlpha = value.mid(alpha.length());
  alpha = alpha.trimmed(); // possible to have a space at the end

  QString year;
  QString serial;
  // have to be able to differentiate 2 and 4-digit years, first check for hyphen position
  int pos = afterAlpha.indexOf(QLatin1Char('-'));
  if(pos > -1) {
    year = afterAlpha.section(QLatin1Char('-'), 0, 0);
    serial = afterAlpha.section(QLatin1Char('-'), 1);
  } else {
    // make two assumptions, the user will never have a book from the year 1920
    // or from any year after 2100. Reasonable, right?
    // so if the string starts with '20' we take the first 4 digits as the year
    // otherwise the first 2
    if(afterAlpha.startsWith(QLatin1String("20"))) {
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
  serial = serial.rightJustified(6, QLatin1Char('0'));
  return alpha + year + serial + suffix;
}
