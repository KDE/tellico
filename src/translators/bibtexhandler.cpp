/***************************************************************************
                              bibtexhandler.cpp
                             -------------------
    begin                : Thu Aug 21 2003
    copyright            : (C) 2003 by Robby Stephenson
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
#include "../bcunit.h"
#include "../bccollection.h"
#include "../bcfilehandler.h"

#include <kglobal.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kurl.h>
#include <kstringhandler.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qdom.h>

StringListMap BibtexHandler::s_utf8LatexMap;
BibtexHandler::QuoteStyle BibtexHandler::s_quoteStyle = BibtexHandler::BRACES;
const QString BibtexHandler::s_bibtexmlNamespace = QString::fromLatin1("http://bibtexml.sf.net/");


QString BibtexHandler::bibtexKey(BCUnit* unit_) {
  QString author;
  BCAttribute* authorAtt = unit_->collection()->attributeByName(QString::fromLatin1("author"));
  if(authorAtt->flags() & BCAttribute::AllowMultiple) {
    QString tmp = unit_->attribute(authorAtt->name());
    author = tmp.section(';', 0, 0);
  } else {
    author = unit_->attribute(authorAtt->name());
  }
  author = author.lower();
  
  QString title = unit_->attribute(QString::fromLatin1("title")).lower();

  QString year = unit_->attribute(QString::fromLatin1("pub_year"));

  if(year.isEmpty()) {
    year = unit_->attribute(QString::fromLatin1("cr_year"));
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
  return key;
}

void BibtexHandler::loadTranslationMaps() {
  QString mapfile = KGlobal::dirs()->findResource("appdata", QString::fromLatin1("bibtex-translation.xml"));
  if(mapfile.isEmpty()) {
    return;
  }

  QDomDocument dom = BCFileHandler::readXMLFile(KURL(mapfile));

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
#if QT_VERSION >= 0x030100
    str.replace(*sit, it.key());
#else
    str.replace(QRegExp(*sit), it.key());
#endif
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
#if QT_VERSION >= 0x030100
    text.replace(it.key(), it.data()[0]);
#else
    text.replace(QRegExp(it.key()), it.data()[0]);
#endif
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
  text = list.join(QString::fromLatin1("#"));
#if QT_VERSION >= 0x030100
  text.replace(lquote+QString::fromLatin1("#")+rquote, QString::fromLatin1("#"));
#else
  text.replace(QRegExp(lquote+QString::fromLatin1("#")+rquote), QString::fromLatin1("#"));
#endif

  return text;
}

bool BibtexHandler::setAttributeValue(BCUnit* unit_, const QString& bibtexField_, const QString& value_) {
  BibtexCollection* c = static_cast<BibtexCollection*>(unit_->collection());
  BibtexAttribute* att = c->attributeByBibtexField(bibtexField_);
  if(!att) {
    // arbitrarily say if the value has more than 100 chars, then it's a paragraph
    if(value_.length() < 100) {
      if(bibtexField_ == QString::fromLatin1("url")) {
        att = new BibtexAttribute(bibtexField_, KStringHandler::capwords(bibtexField_), BCAttribute::URL);
      } else {
        att = new BibtexAttribute(bibtexField_, KStringHandler::capwords(bibtexField_), BCAttribute::Line);
      }
      att->setCategory(i18n("Unknown"));
    } else {
      att = new BibtexAttribute(bibtexField_, KStringHandler::capwords(bibtexField_), BCAttribute::Para);
    }
    att->setBibtexFieldName(bibtexField_);
    c->addAttribute(att);
  }
  return att ? unit_->setAttribute(att->name(), value_) : false;
}

QString& BibtexHandler::cleanText(QString& text_) {
  // TODO: need to improve this for removing all Latex entities
  QRegExp rx(QString::fromLatin1("(?=[^\\])\\.*{"));
  rx.setMinimal(true);
  text_.replace(rx, QString::null);
  text_.replace(QRegExp(QString::fromLatin1("[{}]")), QString::null);
  return text_;
}
