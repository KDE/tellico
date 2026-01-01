/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#include "entrycomparison.h"
#include "entry.h"
#include "field.h"
#include "fieldformat.h"
#include "collection.h"
#include "utils/isbnvalidator.h"
#include "utils/lccnvalidator.h"

using Tellico::EntryComparison;

QUrl EntryComparison::s_documentUrl;

void EntryComparison::setDocumentUrl(const QUrl& url_) {
  s_documentUrl = url_;
}

int EntryComparison::score(const Tellico::Data::EntryPtr& e1, const Tellico::Data::EntryPtr& e2,
                           const QString& f, const Tellico::Data::Collection* c) {
  return score(e1, e2, c->fieldByName(f));
}

int EntryComparison::score(const Tellico::Data::EntryPtr& e1, const Tellico::Data::EntryPtr& e2, Tellico::Data::FieldPtr f) {
  if(!e1 || !e2 || !f) {
    return MATCH_VALUE_NONE;
  }
  QString s1 = e1->field(f);
  if(s1.isEmpty()) {
    return MATCH_VALUE_NONE;
  }
  QString s2 = e2->field(f);
  if(s2.isEmpty()) {
    return MATCH_VALUE_NONE;
  }
  // complicated string matching, here are the cases I want to match
  // "bend it like beckham" == "bend it like beckham (widescreen edition)"
  // "the return of the king" == "return of the king"
  if(s1 == s2) {
    return MATCH_VALUE_STRONG;
  }
  // special case for isbn
  if(f->name() == QLatin1StringView("isbn")) {
    return ISBNValidator::isbn13(s1) == ISBNValidator::isbn13(s2) ? MATCH_VALUE_STRONG : MATCH_VALUE_NONE;
  }
  if(f->name() == QLatin1StringView("lccn")) {
    return LCCNValidator::formalize(s1) == LCCNValidator::formalize(s2) ? MATCH_VALUE_STRONG : MATCH_VALUE_NONE;
  }
  if(f->name() == QLatin1StringView("url") && e1->collection() && e1->collection()->type() == Data::Collection::File) {
    // versions before 1.2.7 could have saved the url without the protocol
    QUrl u1(s1);
    QUrl u2(s2);
    return (u1 == u2 ||
            (f->property(QStringLiteral("relative")) == QLatin1StringView("true") &&
             s_documentUrl.resolved(u1) == s_documentUrl.resolved(u2))) ? MATCH_VALUE_STRONG : MATCH_VALUE_NONE;
  }
  if(f->name() == QLatin1StringView("imdb")) {
    // imdb might be a different host since we query akas.imdb.com and normally it is www.imdb.com
    QUrl us1 = QUrl::fromUserInput(s1);
    QUrl us2 = QUrl::fromUserInput(s2);
    us1.setHost(QString());
    us2.setHost(QString());
    return (us1 == us2) ? MATCH_VALUE_STRONG : MATCH_VALUE_BAD;
  }
  if(f->formatType() == FieldFormat::FormatName) {
    const QString s1n = e1->formattedField(f, FieldFormat::ForceFormat);
    const QString s2n = e2->formattedField(f, FieldFormat::ForceFormat);
    if(s1n == s2n) {
      // let this one fall through if no match, without returning 0
      return MATCH_VALUE_STRONG;
    }
  }
  // now do case-insensitive comparison
  if(s1.compare(s2, Qt::CaseInsensitive) == 0) {
    return MATCH_VALUE_STRONG;
  }

  if(f->formatType() == FieldFormat::FormatTitle) {
    const QString s1t = e1->formattedField(f, FieldFormat::ForceFormat);
    const QString s2t = e2->formattedField(f, FieldFormat::ForceFormat);
    if(s1t.compare(s2t, Qt::CaseInsensitive) == 0) {
      // let this one fall through if no match, without returning 0
      return MATCH_VALUE_WEAK;
    }
  }
  if(f->hasFlag(Data::Field::AllowMultiple)) {
    QStringList sl1 = FieldFormat::splitValue(e1->field(f));
    QStringList sl2 = FieldFormat::splitValue(e2->field(f));
    int matches = 0;
    for(QStringList::ConstIterator it = sl1.constBegin(); it != sl1.constEnd(); ++it) {
      matches += MATCH_VALUE_STRONG*sl2.count(*it);
    }
    if(matches == 0 && f->formatType() == FieldFormat::FormatName) {
      sl1 = FieldFormat::splitValue(e1->formattedField(f, FieldFormat::ForceFormat));
      sl2 = FieldFormat::splitValue(e2->formattedField(f, FieldFormat::ForceFormat));
      for(QStringList::ConstIterator it = sl1.constBegin(); it != sl1.constEnd(); ++it) {
        matches += MATCH_VALUE_STRONG*sl2.count(*it);
      }
    }
    return matches / sl1.count();
  }
  if(f->name() == QLatin1StringView("arxiv")) {
    // normalize and unVersion arxiv ID
    static const QRegularExpression rx1(QStringLiteral("^arxiv:"));
    static const QRegularExpression rx2(QStringLiteral("v\\d+$"));
    s1.remove(rx1);
    s1.remove(rx2);
    s2.remove(rx1);
    s2.remove(rx2);
    return (s1 == s2) ? MATCH_VALUE_STRONG : MATCH_VALUE_BAD;
  }

  // last resort try removing punctuation
  static const QRegularExpression notAlphaNum(QStringLiteral("[^\\s\\w]"));
  QString s1a = s1;
  s1a.remove(notAlphaNum);
  QString s2a = s2;
  s2a.remove(notAlphaNum);
  if(!s1a.isEmpty() && s1a.compare(s2a, Qt::CaseInsensitive) == 0) {
    return MATCH_VALUE_STRONG;
  }
  return MATCH_VALUE_BAD;
}
