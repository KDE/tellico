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

int EntryComparison::score(Tellico::Data::EntryPtr e1, Tellico::Data::EntryPtr e2,
                           const QString& f, const Tellico::Data::Collection* c) {
  return score(e1, e2, c->fieldByName(f));
}

int EntryComparison::score(Tellico::Data::EntryPtr e1, Tellico::Data::EntryPtr e2, Tellico::Data::FieldPtr f) {
  if(!e1 || !e2 || !f) {
    return 0;
  }
  QString s1 = e1->field(f).toLower();
  QString s2 = e2->field(f).toLower();
  if(s1.isEmpty() || s2.isEmpty()) {
    return 0;
  }
  // complicated string matching, here are the cases I want to match
  // "bend it like beckham" == "bend it like beckham (widescreen edition)"
  // "the return of the king" == "return of the king"
  if(s1 == s2) {
    return 5;
  }
  // special case for isbn
  if(f->name() == QLatin1String("isbn") && ISBNValidator::isbn10(s1) == ISBNValidator::isbn10(s2)) {
    return 5;
  }
  if(f->name() == QLatin1String("lccn") && LCCNValidator::formalize(s1) == LCCNValidator::formalize(s2)) {
    return 5;
  }
  if(f->name() == QLatin1String("url") && e1->collection() && e1->collection()->type() == Data::Collection::File) {
    // versions before 1.2.7 could have saved the url without the protocol
    if(QUrl(s1) == QUrl(s2) ||
       (f->property(QLatin1String("relative")) == QLatin1String("true") &&
        s_documentUrl.resolved(QUrl(s1)) == s_documentUrl.resolved(QUrl(s2)))) {
      return 5;
    }
  }
  if (f->name() == QLatin1String("imdb")) {
    // imdb might be a different host since we query akas.imdb.com and normally it is www.imdb.com
    QUrl us1 = QUrl::fromUserInput(s1);
    QUrl us2 = QUrl::fromUserInput(s2);
    us1.setHost(QString());
    us2.setHost(QString());
    if(us1 == us2) {
      return 5;
    }
  }
  if(f->name() == QLatin1String("arxiv")) {
    // normalize and unVersion arxiv ID
    s1.remove(QRegExp(QLatin1String("^arxiv:")));
    s1.remove(QRegExp(QLatin1String("v\\d+$")));
    s2.remove(QRegExp(QLatin1String("^arxiv:")));
    s2.remove(QRegExp(QLatin1String("v\\d+$")));
    if(s1 == s2) {
      return 5;
    }
  }
  if(f->formatType() == FieldFormat::FormatName) {
    const QString s1n = e1->formattedField(f, FieldFormat::ForceFormat);
    const QString s2n = e2->formattedField(f, FieldFormat::ForceFormat);
    if(s1n == s2n) {
      return 5;
    }
  }
  // try removing punctuation
  QRegExp notAlphaNum(QLatin1String("[^\\s\\w]"));
  QString s1a = s1;
  s1a.remove(notAlphaNum);
  QString s2a = s2;
  s2a.remove(notAlphaNum);
  if(!s1a.isEmpty() && s1a == s2a) {
//    myDebug() << "match without punctuation";
    return 5;
  }
  FieldFormat::stripArticles(s1);
  FieldFormat::stripArticles(s2);
  if(!s1.isEmpty() && s1 == s2) {
//    myDebug() << "match without articles";
    return 3;
  }
  // try removing everything between parentheses
  QRegExp rx(QLatin1String("\\s*\\(.*\\)\\s*"));
  s1.remove(rx);
  s2.remove(rx);
  if(!s1.isEmpty() && s1 == s2) {
//    myDebug() << "match without parentheses";
    return 2;
  }
  if(f->hasFlag(Data::Field::AllowMultiple)) {
    QStringList sl1 = FieldFormat::splitValue(e1->field(f));
    QStringList sl2 = FieldFormat::splitValue(e2->field(f));
    int matches = 0;
    for(QStringList::ConstIterator it = sl1.constBegin(); it != sl1.constEnd(); ++it) {
      matches += sl2.count(*it);
    }
    if(matches == 0 && f->formatType() == FieldFormat::FormatName) {
      sl1 = FieldFormat::splitValue(e1->formattedField(f, FieldFormat::ForceFormat));
      sl2 = FieldFormat::splitValue(e2->formattedField(f, FieldFormat::ForceFormat));
      for(QStringList::ConstIterator it = sl1.constBegin(); it != sl1.constEnd(); ++it) {
        matches += sl2.count(*it);
      }
    }
    return matches;
  }
  return 0;
}
