/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
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
#include "../collection.h"
#include "../filehandler.h"

#include <kglobal.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kurl.h>
#include <kstringhandler.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qdom.h>

using Bookcase::BibtexHandler;

BibtexHandler::StringListMap BibtexHandler::s_utf8LatexMap;
BibtexHandler::QuoteStyle BibtexHandler::s_quoteStyle = BibtexHandler::BRACES;
const QString BibtexHandler::s_bibtexmlNamespace = QString::fromLatin1("http://bibtexml.sf.net/");
const QRegExp BibtexHandler::s_badKeyChars(QString::fromLatin1("[^0-9a-zA-Z-]"));

QString BibtexHandler::bibtexKey(Data::Entry* entry_) {
  QString author;
  const Data::BibtexCollection* c = dynamic_cast<const Data::BibtexCollection*>(entry_->collection());
  Data::Field* authorField = c->fieldByBibtexName(QString::fromLatin1("author"));
  if(authorField->flags() & Data::Field::AllowMultiple) {
    QString tmp = entry_->field(authorField->name());
    author = tmp.section(';', 0, 0);
  } else {
    author = entry_->field(authorField->name());
  }
  author = author.lower();

  Data::Field* f = c->fieldByBibtexName(QString::fromLatin1("title"));
  QString title;
  if(f) {
    title = entry_->field(f->name()).lower();
  }

  f = c->fieldByBibtexName(QString::fromLatin1("year"));
  QString year;
  if(f) {
    year = entry_->field(f->name());
  }
  if(year.isEmpty()) {
    year = entry_->field(QString::fromLatin1("pub_year"));
    if(year.isEmpty()) {
      year = entry_->field(QString::fromLatin1("cr_year"));
    }
  }
  year = year.section(QString::fromLatin1("; "), 0, 0);

  return bibtexKey(author, title, year);
}

QString BibtexHandler::bibtexKey(const QString& author_, const QString& title_, const QString& year_) {
  QString key;
  if(author_.find(',') == -1) {
    key += author_.section(' ', -1);
  } else {
    key += author_.section(',', 0, 0);
  }
  key += QString::fromLatin1("-");
  QStringList words = QStringList::split(' ', title_);
  for(QStringList::ConstIterator it = words.begin(); it != words.end(); ++it) {
    key += (*it).left(1);
  }
  key += year_;
  // bibtex key may only contain [0-9a-zA-Z-]
  return key.replace(s_badKeyChars, QString::null);
}

void BibtexHandler::loadTranslationMaps() {
  QString mapfile = KGlobal::dirs()->findResource("appdata", QString::fromLatin1("bibtex-translation.xml"));
  if(mapfile.isEmpty()) {
    return;
  }

  QDomDocument dom = FileHandler::readXMLFile(KURL(mapfile));

  QDomNodeList keyList = dom.elementsByTagName(QString::fromLatin1("key"));

  for(unsigned i = 0; i < keyList.count(); ++i) {
    QDomNodeList strList = keyList.item(i).toElement().elementsByTagName(QString::fromLatin1("string"));
    // the strList might have more than one node since there are multiple ways
    // to represent a character in LaTex. Iterate backwards so that the first one listed
    // becomes the 'primary' key
    QString s = keyList.item(i).toElement().attribute(QString::fromLatin1("char"));
    for(int j = strList.count()-1; j > -1; --j) {
      s_utf8LatexMap[s].append(strList.item(j).toElement().text());
//      kdDebug() << "BibtexHandler::loadTranslationMaps - "
//       << s << " = " << strList.item(j).toElement().text() << endl;
    }
  }
}

QString BibtexHandler::importText(char* text_) {
  if(s_utf8LatexMap.empty()) {
    loadTranslationMaps();
  }

  QString str = QString::fromLatin1(text_);
  for(StringListMap::Iterator it = s_utf8LatexMap.begin(); it != s_utf8LatexMap.end(); ++it) {
    for(QStringList::Iterator sit = it.data().begin(); sit != it.data().end(); ++sit) {
    str.replace(*sit, it.key());
}
  }
  return str;
}

QString BibtexHandler::exportText(const QString& text_, const QStringList& macros_) {
  if(s_utf8LatexMap.empty()) {
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

  for(StringListMap::Iterator it = s_utf8LatexMap.begin(); it != s_utf8LatexMap.end(); ++it) {
    text.replace(it.key(), it.data()[0]);
  }

  if(macros_.isEmpty()) {
    return lquote + text + rquote;
  }

// Now, split the text by the character '#', and examine each token to see if it is in
// the macro list. If it is not, then add left-quote and right-quote around it. If it is, don't
// change it. Then, in case '#' occurs in a non-macro string, replace any occurances of '}#{' with '#'

// list of new tokens
  QStringList list;

// first, split the text
  QStringList tokens = QStringList::split('#', text, true);
  for(QStringList::ConstIterator it = tokens.begin(); it != tokens.end(); ++it) {
    // check to see if token is a macro
    if(macros_.findIndex((*it).stripWhiteSpace()) == -1) {
      // the token is NOT a macro, add braces
      list << lquote + *it + rquote;
    } else {
      list << *it;
    }
  }

  const QString octo = QString::fromLatin1("#");
  text = list.join(octo);
  text.replace(rquote+octo+lquote, octo);

  return text;
}

bool BibtexHandler::setFieldValue(Data::Entry* entry_, const QString& bibtexField_, const QString& value_) {
  Data::BibtexCollection* c = static_cast<Data::BibtexCollection*>(entry_->collection());
  Data::Field* field = c->fieldByBibtexName(bibtexField_);
  if(!field) {
    // arbitrarily say if the value has more than 100 chars, then it's a paragraph
    if(value_.length() < 100) {
      // special case, try to detect URLs
      // In qt 3.1, QString::startsWith() is always case-sensitive
      if(bibtexField_ == QString::fromLatin1("url")
         || value_.lower().startsWith(QString::fromLatin1("http"))
         || value_.lower().startsWith(QString::fromLatin1("file:/"))
         || value_.lower().startsWith(QString::fromLatin1("/"))) { // assume this indicates a local path
        kdDebug() << "BibtexHandler::setFieldValue() - creating a URL field for " << bibtexField_ << endl;
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
  return field ? entry_->setField(field->name(), value_) : false;
}

QString& BibtexHandler::cleanText(QString& text_) {
  // FIXME: need to improve this for removing all Latex entities
  QRegExp rx(QString::fromLatin1("(?=[^\\])\\.*{"));
  rx.setMinimal(true);
  text_.replace(rx, QString::null);
  text_.replace(QRegExp(QString::fromLatin1("[{}]")), QString::null);
  text_.replace('~', ' ');
  return text_;
}
