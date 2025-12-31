/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include "bibtexhandler.h"
#include "../collections/bibtexcollection.h"
#include "../entry.h"
#include "../field.h"
#include "../core/filehandler.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

#include <QUrl>

#include <QDomDocument>

// don't add braces around capital letters by default
#define TELLICO_BIBTEX_BRACES 0

using Tellico::BibtexHandler;

BibtexHandler::StringListHash BibtexHandler::s_utf8LatexMap;
BibtexHandler::QuoteStyle BibtexHandler::s_quoteStyle = BibtexHandler::BRACES;
const QRegularExpression BibtexHandler::s_badKeyChars(QStringLiteral("[^0-9a-zA-Z-]"));

QStringList BibtexHandler::bibtexKeys(const Tellico::Data::EntryList& entries_) {
  QStringList keys;
  foreach(Data::EntryPtr entry, entries_) {
    QString s = bibtexKey(entry);
    if(!s.isEmpty()) {
      keys << s;
    }
  }
  return keys;
}

QString BibtexHandler::bibtexKey(Tellico::Data::EntryPtr entry_) {
  if(!entry_ || !entry_->collection() || entry_->collection()->type() != Data::Collection::Bibtex) {
    return QString();
  }

  const Data::BibtexCollection* c = static_cast<const Data::BibtexCollection*>(entry_->collection().data());
  Data::FieldPtr f = c->fieldByBibtexName(QStringLiteral("key"));
  if(f) {
    const QString key = entry_->field(f);
    if(!key.isEmpty()) {
      return key;
    }
  }

  QString author;
  Data::FieldPtr authorField = c->fieldByBibtexName(QStringLiteral("author"));
  if(authorField) {
    if(authorField->hasFlag(Data::Field::AllowMultiple)) {
      // grab first author only;
      QString tmp = entry_->field(authorField);
      author = tmp.section(QLatin1Char(';'), 0, 0);
    } else {
      author = entry_->field(authorField);
    }
  }

  Data::FieldPtr titleField = c->fieldByBibtexName(QStringLiteral("title"));
  QString title;
  if(titleField) {
    title = entry_->field(titleField);
  }

  Data::FieldPtr yearField = c->fieldByBibtexName(QStringLiteral("year"));
  QString year;
  if(yearField) {
    year = entry_->field(yearField);
  }
  if(year.isEmpty()) {
    year = entry_->field(QStringLiteral("pub_year"));
    if(year.isEmpty()) {
      year = entry_->field(QStringLiteral("cr_year"));
    }
  }
  year = year.section(QLatin1Char(';'), 0, 0);

  return bibtexKey(author, title, year);
}

QString BibtexHandler::bibtexKey(const QString& author_, const QString& title_, const QString& year_) {
  QString key;
  // if no comma, take the last word
  if(!author_.isEmpty()) {
    if(author_.indexOf(QLatin1Char(',')) == -1) {
      key += author_.section(QLatin1Char(' '), -1).toLower() + QLatin1Char('-');
    } else {
      // if there is a comma, take the string up to the first comma
      key += author_.section(QLatin1Char(','), 0, 0).toLower() + QLatin1Char('-');
    }
  }
  QStringList words = title_.split(QLatin1Char(' '), Qt::SkipEmptyParts);
  foreach(const QString& word, words) {
    key += word.at(0).toLower();
  }
  key += year_;
  // bibtex key may only contain [0-9a-zA-Z-]
  return key.remove(s_badKeyChars);
}

void BibtexHandler::loadTranslationMaps() {
  QString mapfile = DataFileRegistry::self()->locate(QStringLiteral("bibtex-translation.xml"));
  if(mapfile.isEmpty()) {
    static bool showMsg = true;
    if(showMsg) {
      myWarning() << "bibtex-translation.xml not found";
      showMsg = false;
    }
    return;
  }

  QUrl u = QUrl::fromLocalFile(mapfile);
  // no namespace processing
  QDomDocument dom = FileHandler::readXMLDocument(u, false);

  QDomNodeList keyList = dom.elementsByTagName(QStringLiteral("key"));

  for(int i = 0; i < keyList.count(); ++i) {
    QDomNodeList strList = keyList.item(i).toElement().elementsByTagName(QStringLiteral("string"));
    // the strList might have more than one node since there are multiple ways
    // to represent a character in LaTex.
    QString s = keyList.item(i).toElement().attribute(QStringLiteral("char"));
    for(int j = 0; j < strList.count(); ++j) {
      s_utf8LatexMap[s].append(strList.item(j).toElement().text());
//      myDebug() << s << " = " << strList.item(j).toElement().text();
    }
  }
}

QString BibtexHandler::importText(char* text_) {
  QString str = QString::fromUtf8(text_);

  if(s_utf8LatexMap.isEmpty()) {
    loadTranslationMaps();
  }

  for(StringListHash::ConstIterator it = s_utf8LatexMap.constBegin(); it != s_utf8LatexMap.constEnd(); ++it) {
    foreach(const QString& word, it.value()) {
      str.replace(word, it.key());
    }
  }

  // now replace capitalized letters, such as {X}
  // but since we don't want to turn "... X" into "... {X}" later when exporting
  // we need to lower-case any capitalized text after the first letter that is
  // NOT contained in braces

  static const QRegularExpression rx(QStringLiteral("\\{([A-Z]+?)\\}"));
  str.replace(rx, QStringLiteral("\\1"));

  return str;
}

QString BibtexHandler::exportText(const QString& text_, const QStringList& macros_) {
  if(s_utf8LatexMap.isEmpty()) {
    loadTranslationMaps();
  }

  QChar lquote, rquote;
  switch(s_quoteStyle) {
    case BRACES:
      lquote = QLatin1Char('{');
      rquote = QLatin1Char('}');
      break;
    case QUOTES:
      lquote =  QLatin1Char('"');
      rquote =  QLatin1Char('"');
      break;
  }

  QString text = text_;

  for(StringListHash::ConstIterator it = s_utf8LatexMap.constBegin(); it != s_utf8LatexMap.constEnd(); ++it) {
    text.replace(it.key(), it.value()[0]);
  }

  if(macros_.isEmpty()) {
    return lquote + addBraces(text) + rquote;
  }

// Now, split the text by the character QLatin1Char('#'), and examine each token to see if it is in
// the macro list. If it is not, then add left-quote and right-quote around it. If it is, don't
// change it. Then, in case QLatin1Char('#') occurs in a non-macro string, replace any occurrences of '}#{' with '#'

// list of new tokens
  QStringList list;

// first, split the text
  const QStringList tokens = text.split(QLatin1Char('#'), Qt::KeepEmptyParts);
  foreach(const QString& token, tokens) {
    // check to see if token is a macro
    if(macros_.indexOf(token.trimmed()) == -1) {
      // the token is NOT a macro, add braces around whole words and also around capitals
      list << lquote + addBraces(token) + rquote;
    } else {
      list << token;
    }
  }

  const QChar octo = QLatin1Char('#');
  text = list.join(octo);
  text.replace(QString(rquote)+octo+lquote, octo);

  return text;
}

QString& BibtexHandler::cleanText(QString& text_) {
  // FIXME: need to improve this for removing all Latex entities
//  QRegularExpression rx(QLatin1String("(?=[^\\\\])\\\\.+\\{"));
  static const QRegularExpression rx(QStringLiteral("\\\\.+?\\{"));
  static const QRegularExpression brackets(QStringLiteral("[{}]"));
  text_.remove(rx);
  text_.remove(brackets);
  return text_;
}

// add braces around capital letters
QString BibtexHandler::addBraces(const QString& text_) {
  QString text = text_;
#if !TELLICO_BIBTEX_BRACES
  return text;
#else
  int inside = 0;
  uint l = text.length();
  // start at first letter, but skip if only the first is capitalized
  for(uint i = 0; i < l; ++i) {
    const QChar c = text.at(i);
    if(inside == 0 && c >= 'A' && c <= 'Z') {
      uint j = i+1;
      while(text.at(j) >= 'A' && text.at(j) <= 'Z' && j < l) {
        ++j;
      }
      if(i == 0 && j == 1) {
        continue; // no need to do anything to first letter
      }
      text.insert(i, '{');
      // now j should be incremented
      text.insert(j+1, '}');
      i = j+1;
      l += 2; // the length changed
    } else if(c == '{') {
      ++inside;
    } else if(c == '}') {
      --inside;
    }
  }
  return text;
#endif
}
