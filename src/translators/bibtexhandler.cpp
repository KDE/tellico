/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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
#include "../latin1literal.h"
#include "../tellico_debug.h"

#include <kstandarddirs.h>
#include <kurl.h>
#include <kstringhandler.h>
#include <klocale.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qdom.h>

// don't add braces around capital letters by default
#define TELLICO_BIBTEX_BRACES 0

using Tellico::BibtexHandler;

BibtexHandler::StringListMap* BibtexHandler::s_utf8LatexMap = 0;
BibtexHandler::QuoteStyle BibtexHandler::s_quoteStyle = BibtexHandler::BRACES;
const QRegExp BibtexHandler::s_badKeyChars(QString::fromLatin1("[^0-9a-zA-Z-]"));

QStringList BibtexHandler::bibtexKeys(const Data::EntryVec& entries_) {
  QStringList keys;
  for(Data::EntryVec::ConstIterator it = entries_.begin(); it != entries_.end(); ++it) {
    QString s = bibtexKey(it.data());
    if(!s.isEmpty()) {
      keys << s;
    }
  }
  return keys;
}

QString BibtexHandler::bibtexKey(Data::ConstEntryPtr entry_) {
  if(!entry_ || !entry_->collection() || entry_->collection()->type() != Data::Collection::Bibtex) {
    return QString::null;
  }

  const Data::BibtexCollection* c = static_cast<const Data::BibtexCollection*>(entry_->collection().data());
  Data::FieldPtr f = c->fieldByBibtexName(QString::fromLatin1("key"));
  if(f) {
    QString key = entry_->field(f->name());
    if(!key.isEmpty()) {
      return key;
    }
  }

  QString author;
  Data::FieldPtr authorField = c->fieldByBibtexName(QString::fromLatin1("author"));
  if(authorField) {
    if(authorField->flags() & Data::Field::AllowMultiple) {
      // grab first author only;
      QString tmp = entry_->field(authorField->name());
      author = tmp.section(';', 0, 0);
    } else {
      author = entry_->field(authorField->name());
    }
  }

  Data::FieldPtr titleField = c->fieldByBibtexName(QString::fromLatin1("title"));
  QString title;
  if(titleField) {
    title = entry_->field(titleField->name());
  }

  Data::FieldPtr yearField = c->fieldByBibtexName(QString::fromLatin1("year"));
  QString year;
  if(yearField) {
    year = entry_->field(yearField->name());
  }
  if(year.isEmpty()) {
    year = entry_->field(QString::fromLatin1("pub_year"));
    if(year.isEmpty()) {
      year = entry_->field(QString::fromLatin1("cr_year"));
    }
  }
  year = year.section(';', 0, 0);

  return bibtexKey(author, title, year);
}

QString BibtexHandler::bibtexKey(const QString& author_, const QString& title_, const QString& year_) {
  QString key;
  // if no comma, take the last word
  if(!author_.isEmpty()) {
    if(author_.find(',') == -1) {
      key += author_.section(' ', -1).lower() + '-';
    } else {
      // if there is a comma, take the string up to the first comma
      key += author_.section(',', 0, 0).lower() + '-';
    }
  }
  QStringList words = QStringList::split(' ', title_);
  for(QStringList::ConstIterator it = words.begin(); it != words.end(); ++it) {
    key += (*it).left(1).lower();
  }
  key += year_;
  // bibtex key may only contain [0-9a-zA-Z-]
  return key.replace(s_badKeyChars, QString::null);
}

void BibtexHandler::loadTranslationMaps() {
  QString mapfile = locate("appdata", QString::fromLatin1("bibtex-translation.xml"));
  if(mapfile.isEmpty()) {
    return;
  }

  s_utf8LatexMap = new StringListMap();

  KURL u;
  u.setPath(mapfile);
  // no namespace processing
  QDomDocument dom = FileHandler::readXMLFile(u, false);

  QDomNodeList keyList = dom.elementsByTagName(QString::fromLatin1("key"));

  for(unsigned i = 0; i < keyList.count(); ++i) {
    QDomNodeList strList = keyList.item(i).toElement().elementsByTagName(QString::fromLatin1("string"));
    // the strList might have more than one node since there are multiple ways
    // to represent a character in LaTex.
    QString s = keyList.item(i).toElement().attribute(QString::fromLatin1("char"));
    for(unsigned j = 0; j < strList.count(); ++j) {
      (*s_utf8LatexMap)[s].append(strList.item(j).toElement().text());
//      kdDebug() << "BibtexHandler::loadTranslationMaps - "
//       << s << " = " << strList.item(j).toElement().text() << endl;
    }
  }
}

QString BibtexHandler::importText(char* text_) {
  if(!s_utf8LatexMap) {
    loadTranslationMaps();
  }

  QString str = QString::fromUtf8(text_);
  for(StringListMap::Iterator it = s_utf8LatexMap->begin(); it != s_utf8LatexMap->end(); ++it) {
    for(QStringList::Iterator sit = it.data().begin(); sit != it.data().end(); ++sit) {
      str.replace(*sit, it.key());
    }
  }

  // now replace capitalized letters, such as {X}
  // but since we don't want to turn "... X" into "... {X}" later when exporting
  // we need to lower-case any capitalized text after the first letter that is
  // NOT contained in braces

  QRegExp rx(QString::fromLatin1("\\{([A-Z]+)\\}"));
  rx.setMinimal(true);
  str.replace(rx, QString::fromLatin1("\\1"));

  return str;
}

QString BibtexHandler::exportText(const QString& text_, const QStringList& macros_) {
  if(!s_utf8LatexMap) {
    loadTranslationMaps();
  }

  QChar lquote, rquote;
  switch(s_quoteStyle) {
    case BRACES:
      lquote = '{';
      rquote = '}';
      break;
    case QUOTES:
      lquote =  '"';
      rquote =  '"';
      break;
  }

  QString text = text_;

  for(StringListMap::Iterator it = s_utf8LatexMap->begin(); it != s_utf8LatexMap->end(); ++it) {
    text.replace(it.key(), it.data()[0]);
  }

  if(macros_.isEmpty()) {
    return lquote + addBraces(text) + rquote;
  }

// Now, split the text by the character '#', and examine each token to see if it is in
// the macro list. If it is not, then add left-quote and right-quote around it. If it is, don't
// change it. Then, in case '#' occurs in a non-macro string, replace any occurrences of '}#{' with '#'

// list of new tokens
  QStringList list;

// first, split the text
  QStringList tokens = QStringList::split('#', text, true);
  for(QStringList::Iterator it = tokens.begin(); it != tokens.end(); ++it) {
    // check to see if token is a macro
    if(macros_.findIndex((*it).stripWhiteSpace()) == -1) {
      // the token is NOT a macro, add braces around whole words and also around capitals
      list << lquote + addBraces(*it) + rquote;
    } else {
      list << *it;
    }
  }

  const QChar octo = '#';
  text = list.join(octo);
  text.replace(QString(rquote)+octo+lquote, octo);

  return text;
}

bool BibtexHandler::setFieldValue(Data::EntryPtr entry_, const QString& bibtexField_, const QString& value_) {
  Data::BibtexCollection* c = static_cast<Data::BibtexCollection*>(entry_->collection().data());
  Data::FieldPtr field = c->fieldByBibtexName(bibtexField_);
  if(!field) {
    // it was the case that the default bibliography did not have a bibtex property for keywords
    // so a "keywords" field would get created in the imported collection
    // but the existing collection had a field "keyword" so the values would not get imported
    // here, check to see if the current collection has a field with the same bibtex name and
    // use it instead of creating a new one
    Data::BibtexCollection* existingColl = Data::Document::self()->collection()->type() == Data::Collection::Bibtex
                                         ? static_cast<Data::BibtexCollection*>(Data::Document::self()->collection().data())
                                         : 0;
    Data::FieldPtr existingField = existingColl ? existingColl->fieldByBibtexName(bibtexField_) : 0;
    if(existingField) {
      field = new Data::Field(*existingField);
    } else if(value_.length() < 100) {
      // arbitrarily say if the value has more than 100 chars, then it's a paragraph
      QString vlower = value_.lower();
      // special case, try to detect URLs
      // In qt 3.1, QString::startsWith() is always case-sensitive
      if(bibtexField_ == Latin1Literal("url")
         || vlower.startsWith(QString::fromLatin1("http")) // may also be https
         || vlower.startsWith(QString::fromLatin1("ftp:/"))
         || vlower.startsWith(QString::fromLatin1("file:/"))
         || vlower.startsWith(QString::fromLatin1("/"))) { // assume this indicates a local path
        myDebug() << "BibtexHandler::setFieldValue() - creating a URL field for " << bibtexField_ << endl;
        field = new Data::Field(bibtexField_, KStringHandler::capwords(bibtexField_), Data::Field::URL);
      } else {
        field = new Data::Field(bibtexField_, KStringHandler::capwords(bibtexField_), Data::Field::Line);
      }
      field->setCategory(i18n("Unknown"));
    } else {
      field = new Data::Field(bibtexField_, KStringHandler::capwords(bibtexField_), Data::Field::Para);
    }
    field->setProperty(QString::fromLatin1("bibtex"), bibtexField_);
    c->addField(field);
  }
  // special case keywords, replace commas with semi-colons so they get separated
  QString value = value_;
  if(field->property(QString::fromLatin1("bibtex")).startsWith(QString::fromLatin1("keyword"))) {
    value.replace(',', ';');
  }
  return entry_->setField(field, value);
}

QString& BibtexHandler::cleanText(QString& text_) {
  // FIXME: need to improve this for removing all Latex entities
//  QRegExp rx(QString::fromLatin1("(?=[^\\\\])\\\\.+\\{"));
  QRegExp rx(QString::fromLatin1("\\\\.+\\{"));
  rx.setMinimal(true);
  text_.replace(rx, QString::null);
  text_.replace(QRegExp(QString::fromLatin1("[{}]")), QString::null);
  text_.replace('~', ' ');
  return text_;
}

// add braces around capital letters
QString& BibtexHandler::addBraces(QString& text) {
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
