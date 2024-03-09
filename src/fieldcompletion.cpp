/***************************************************************************
    Copyright (C) 2003-2021 Robby Stephenson <robby@periapsis.org>
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

#include "fieldcompletion.h"
#include "fieldformat.h"

#include <KCompletionMatches>

using Tellico::FieldCompletion;

FieldCompletion::FieldCompletion(bool multiple_) : KCompletion(), m_multiple(multiple_) {
}

QString FieldCompletion::makeCompletion(const QString& string_) {
  if(completionMode() == KCompletion::CompletionNone) {
    m_beginText.clear();
    return QString();
  }

  if(!m_multiple) {
    return KCompletion::makeCompletion(string_);
  }

  static const QRegularExpression rx = FieldFormat::delimiterRegularExpression();
  QRegularExpressionMatch match;
  const int index = string_.lastIndexOf(rx, -1, &match);
  if(index < 0) {
    m_beginText.clear();
    return KCompletion::makeCompletion(string_);
  }

  const int pos = match.capturedEnd();
  m_beginText = string_.mid(0, pos);
  // m_beginText is added back to the string in postProcessMatch
  return KCompletion::makeCompletion(string_.mid(pos));
}

void FieldCompletion::clear() {
  m_beginText.clear();
  KCompletion::clear();
}

void FieldCompletion::postProcessMatch(QString* match_) const {
  if(m_multiple) {
    match_->prepend(m_beginText);
  }
}

void FieldCompletion::postProcessMatches(QStringList* matches_) const {
  if(m_multiple) {
    for(QStringList::Iterator it = matches_->begin(); it != matches_->end(); ++it) {
      (*it).prepend(m_beginText);
    }
  }
}

void FieldCompletion::postProcessMatches(KCompletionMatches* matches_) const {
  if(m_multiple) {
    for(KCompletionMatches::Iterator it = matches_->begin(); it != matches_->end(); ++it) {
      (*it).value().prepend(m_beginText);
    }
  }
}
