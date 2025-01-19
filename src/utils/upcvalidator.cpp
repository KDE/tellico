/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#include "upcvalidator.h"
#include "isbnvalidator.h"

using Tellico::UPCValidator;

UPCValidator::UPCValidator(QObject* parent_)
    : QValidator(parent_), m_checkISBN(false), m_isbn(false) {
}

QValidator::State UPCValidator::validate(QString& input_, int& pos_) const {
  // check if it's a cuecat first
  State catState = CueCat::decode(input_);
  if(catState == Acceptable) {
    pos_ = input_.length();
    return catState;
  }

  // no spaces allowed
  if(input_.contains(QLatin1Char(' '))) {
    return QValidator::Invalid;
  }

  // no real checking, just if starts with 978, use isbnvalidator
  const uint len = input_.length();
  if(len < 10) {
    m_isbn = false;
  }

  if(!m_checkISBN || (!m_isbn && len < 13)) {
    return QValidator::Intermediate;
  }

  // once it gets converted to an ISBN, remember that, and use it for later
  if(input_.startsWith(QLatin1String("978")) || input_.startsWith(QLatin1String("979"))) {
    ISBNValidator val;
    QValidator::State s = val.validate(input_, pos_);
    if(s == QValidator::Acceptable) {
      m_isbn = true;
      Q_EMIT signalISBN();
    }
    return s;
  }

  // TODO: return QValidator::Acceptable if check sum is correct
  return QValidator::Intermediate;
}

void UPCValidator::fixup(QString& input_) const {
  if(input_.isEmpty()) {
    return;
  }
  input_ = input_.trimmed();

  int pos = input_.indexOf(QLatin1Char(' '));
  if(pos > -1) {
    input_ = input_.left(pos);
  }

  if(!m_checkISBN) {
    return;
  }

  const uint len = input_.length();
  if(len > 12 && (input_.startsWith(QLatin1String("978")) || input_.startsWith(QLatin1String("979")))) {
    QString s = input_;
    ISBNValidator val;
    int p = 0;
    int state = val.validate(s, p);
    if(state == QValidator::Acceptable) {
      Q_EMIT signalISBN();
      input_ = s;
    }
  }
}

QValidator::State Tellico::CueCat::decode(QString& input_) {
  if(input_.length() < 3) {
    return QValidator::Intermediate;
  }
 if(!input_.startsWith(QLatin1String(".C3"))) { // all cuecat codes start with .C3
    return QValidator::Invalid;
  }
  const int periods = input_.count(QLatin1Char('.'));
  if(periods < 4) {
    return QValidator::Intermediate; // not enough yet
  } else if(periods > 4) {
    return QValidator::Invalid;
  }

  // ok, let's have a go, take the third token
  QString code = input_.section(QLatin1Char('.'), 2, 2, QString::SectionSkipEmpty);
  while(code.length() % 4 > 0) {
    code += QLatin1Char('=');
  }

  for(int i = 0; i < code.length(); ++i) {
    if(code[i] >= QLatin1Char('A') && code[i] <= QLatin1Char('Z')) {
      code.replace(i, 1, code[i].toLower());
    } else if(code[i] >= QLatin1Char('a') && code[i] <= QLatin1Char('z')) {
      code.replace(i, 1, code[i].toUpper());
    }
  }

  code = QString::fromLatin1(QByteArray::fromBase64(code.toLatin1()));

  for(int i = 0; i < code.length(); ++i) {
    char c = code[i].toLatin1() ^ 'C';
    code.replace(i, 1, QLatin1Char(c));
  }

  input_ = code;
  return QValidator::Acceptable;
}
