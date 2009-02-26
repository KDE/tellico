/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bibtexhandler.h"
#include "../collections/bibtexcollection.h"
#include "../entry.h"
#include "../field.h"
#include "../collection.h"
#include "../document.h"
#include "../filehandler.h"
#include "../tellico_debug.h"

#include <kstandarddirs.h>
#include <kurl.h>
#include <kstringhandler.h>
#include <klocale.h>

#include <QDomDocument>

// don't add braces around capital letters by default
#define TELLICO_BIBTEX_BRACES 0

using Tellico::BibtexHandler;

BibtexHandler::StringListHash* BibtexHandler::s_utf8LatexMap = 0;
BibtexHandler::QuoteStyle BibtexHandler::s_quoteStyle = BibtexHandler::BRACES;
const QRegExp BibtexHandler::s_badKeyChars(QLatin1String("[^0-9a-zA-Z-]"));

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
  Data::FieldPtr f = c->fieldByBibtexName(QLatin1String("key"));
  if(f) {
    QString key = entry_->field(f->name());
    if(!key.isEmpty()) {
      return key;
    }
  }

  QString author;
  Data::FieldPtr authorField = c->fieldByBibtexName(QLatin1String("author"));
  if(authorField) {
    if(authorField->flags() & Data::Field::AllowMultiple) {
      // grab first author only;
      QString tmp = entry_->field(authorField->name());
      author = tmp.section(QLatin1Char(';'), 0, 0);
    } else {
      author = entry_->field(authorField->name());
    }
  }

  Data::FieldPtr titleField = c->fieldByBibtexName(QLatin1String("title"));
  QString title;
  if(titleField) {
    title = entry_->field(titleField->name());
  }

  Data::FieldPtr yearField = c->fieldByBibtexName(QLatin1String("year"));
  QString year;
  if(yearField) {
    year = entry_->field(yearField->name());
  }
  if(year.isEmpty()) {
    year = entry_->field(QLatin1String("pub_year"));
    if(year.isEmpty()) {
      year = entry_->field(QLatin1String("cr_year"));
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
  QStringList words = title_.split(QLatin1Char(' '), QString::SkipEmptyParts);
  foreach(const QString& word, words) {
    key += word.left(1).toLower();
  }
  key += year_;
  // bibtex key may only contain [0-9a-zA-Z-]
  return key.remove(s_badKeyChars);
}

void BibtexHandler::loadTranslationMaps() {
  QString mapfile = KStandardDirs::locate("appdata", QLatin1String("bibtex-translation.xml"));
  if(mapfile.isEmpty()) {
    return;
  }

  s_utf8LatexMap = new StringListHash();

  KUrl u;
  u.setPath(mapfile);
  // no namespace processing
  QDomDocument dom = FileHandler::readXMLFile(u, false);

  QDomNodeList keyList = dom.elementsByTagName(QLatin1String("key"));

  for(int i = 0; i < keyList.count(); ++i) {
    QDomNodeList strList = keyList.item(i).toElement().elementsByTagName(QLatin1String("string"));
    // the strList might have more than one node since there are multiple ways
    // to represent a character in LaTex.
    QString s = keyList.item(i).toElement().attribute(QLatin1String("char"));
    for(int j = 0; j < strList.count(); ++j) {
      (*s_utf8LatexMap)[s].append(strList.item(j).toElement().text());
//      kDebug() << "BibtexHandler::loadTranslationMaps - "
//       << s << " = " << strList.item(j).toElement().text() << endl;
    }
  }
}

QString BibtexHandler::importText(char* text_) {
  if(!s_utf8LatexMap) {
    loadTranslationMaps();
  }

  QString str = QString::fromUtf8(text_);
  for(StringListHash::const_iterator it = s_utf8LatexMap->constBegin(); it != s_utf8LatexMap->constEnd(); ++it) {
    foreach(const QString& word, it.value()) {
      str.replace(word, it.key());
    }
  }

  // now replace capitalized letters, such as {X}
  // but since we don't want to turn "... X" into "... {X}" later when exporting
  // we need to lower-case any capitalized text after the first letter that is
  // NOT contained in braces

  QRegExp rx(QLatin1String("\\{([A-Z]+)\\}"));
  rx.setMinimal(true);
  str.replace(rx, QLatin1String("\\1"));

  return str;
}

QString BibtexHandler::exportText(const QString& text_, const QStringList& macros_) {
  if(!s_utf8LatexMap) {
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

  for(StringListHash::const_iterator it = s_utf8LatexMap->constBegin(); it != s_utf8LatexMap->constEnd(); ++it) {
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
  const QStringList tokens = text.split(QLatin1Char('#'), QString::KeepEmptyParts);
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

bool BibtexHandler::setFieldValue(Tellico::Data::EntryPtr entry_, const QString& bibtexField_, const QString& value_) {
  Data::BibtexCollection* c = static_cast<Data::BibtexCollection*>(entry_->collection().data());
  Data::FieldPtr field = c->fieldByBibtexName(bibtexField_);
  if(!field) {
    // it was the case that the default bibliography did not have a bibtex property for keywords
    // so a "keywords" field would get created in the imported collection
    // but the existing collection had a field "keyword" so the values would not get imported
    // here, check to see if the current collection has a field with the same bibtex name and
    // use it instead of creating a new one
    Data::BibtexCollection* existingColl = 0;
    if(Data::Document::self()->collection()->type() == Data::Collection::Bibtex) {
       existingColl = static_cast<Data::BibtexCollection*>(Data::Document::self()->collection().data());
    }
    Data::FieldPtr existingField;
    if(existingColl) {
       existingField = existingColl->fieldByBibtexName(bibtexField_);
    }
    if(existingField) {
      field = new Data::Field(*existingField);
    } else if(value_.length() < 100) {
      // arbitrarily say if the value has more than 100 chars, then it's a paragraph
      QString vlower = value_.toLower();
      // special case, try to detect URLs
      // In qt 3.1, QString::startsWith() is always case-sensitive
      if(bibtexField_ == QLatin1String("url")
         || vlower.startsWith(QLatin1String("http")) // may also be https
         || vlower.startsWith(QLatin1String("ftp:/"))
         || vlower.startsWith(QLatin1String("file:/"))
         || vlower.startsWith(QLatin1String("/"))) { // assume this indicates a local path
        myDebug() << "BibtexHandler::setFieldValue() - creating a URL field for " << bibtexField_ << endl;
        field = new Data::Field(bibtexField_, KStringHandler::capwords(bibtexField_), Data::Field::URL);
      } else {
        field = new Data::Field(bibtexField_, KStringHandler::capwords(bibtexField_), Data::Field::Line);
      }
      field->setCategory(i18n("Unknown"));
    } else {
      field = new Data::Field(bibtexField_, KStringHandler::capwords(bibtexField_), Data::Field::Para);
    }
    field->setProperty(QLatin1String("bibtex"), bibtexField_);
    c->addField(field);
  }
  // special case keywords, replace commas with semi-colons so they get separated
  QString value = value_;
  if(field->property(QLatin1String("bibtex")).startsWith(QLatin1String("keyword"))) {
    value.replace(QLatin1Char(','), QLatin1Char(';'));
    // special case refbase bibtex export, with multiple keywords fields
    QString oValue = entry_->field(field);
    if(!oValue.isEmpty()) {
      value = oValue + QLatin1String("; ") + value;
    }
  }
  return entry_->setField(field, value);
}

QString& BibtexHandler::cleanText(QString& text_) {
  // FIXME: need to improve this for removing all Latex entities
//  QRegExp rx(QLatin1String("(?=[^\\\\])\\\\.+\\{"));
  QRegExp rx(QLatin1String("\\\\.+\\{"));
  rx.setMinimal(true);
  text_.remove(rx);
  text_.remove(QRegExp(QLatin1String("[{}]")));
  text_.replace(QLatin1Char('~'), QLatin1Char(' '));
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
