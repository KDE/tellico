/***************************************************************************
    Copyright (C) 2024 Robby Stephenson <robby@periapsis.org>
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

#include "filterparser.h"

#include <KShell>

using Tellico::FilterParser;

FilterParser::FilterParser(const QString& text_, bool allowRegExp_)
    : m_text(text_)
    , m_allowRegExp(allowRegExp_) {
}

Tellico::FilterPtr FilterParser::filter() {
  if(m_text.isEmpty()) {
    return FilterPtr();
  }

  FilterPtr filter(new Filter(Filter::MatchAll));

  const auto tokens = KShell::splitArgs(m_text);
  foreach(const QString& token, tokens) {
    QString fieldName; // empty field name means match on any field
    QString fieldText;
    // if the text contains '=' assume it's a field name or title
    const auto pos = token.indexOf(QLatin1Char('='));
    if(pos > 0) {
      fieldName = token.left(pos).trimmed();
      fieldText = token.mid(pos+1).trimmed();
      // check that the field name might be a title
      if(m_coll && !m_coll->hasField(fieldName)) {
        // fall back to looking by title
        auto f = m_coll->fieldByTitle(fieldName);
        if(f) fieldName = f->name();
        // if there's no fieldName match, then use the whole string in the filter
        if(fieldName.isEmpty()) {
          fieldText = m_text;
        }
      }
    } else {
      fieldText = token;
    }
    parseToken(filter, fieldName, fieldText);
  }

  return filter;
}

void FilterParser::parseToken(FilterPtr filter_, const QString& fieldName_, const QString& fieldText_) {
  // if the text contains any non-word characters, assume it's a regexp
  // but \W in qt is letter, number, or '_', I want to be a bit less strict
  static const QRegularExpression rx(QLatin1String("[^\\w\\s\\-']"));
  if(m_allowRegExp && rx.match(fieldText_).hasMatch()) {
    QString text = fieldText_;
    QRegularExpression tx(text);
    if(!tx.isValid()) {
      text = QRegularExpression::escape(text);
      tx.setPattern(text);
    }
    if(tx.isValid()) {
      filter_->append(new FilterRule(fieldName_, text, FilterRule::FuncRegExp));
      return;
    }
  }

  filter_->append(new FilterRule(fieldName_, fieldText_, FilterRule::FuncContains));
}
