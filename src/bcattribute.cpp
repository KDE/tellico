/***************************************************************************
                               bcattribute.cpp
                             -------------------
    begin                : Sun Sep 23 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bcattribute.h"

#include <klocale.h>
#include <kdebug.h>

#include <qstringlist.h>
#include <qregexp.h>

//these get overwritten, but are here since they're static
QStringList BCAttribute::m_articles = QStringList();
QStringList BCAttribute::m_suffixes = QStringList();
bool BCAttribute::m_autoCapitalize = true;
bool BCAttribute::m_autoFormat = true;

// this constructor is for anything but Choice type
BCAttribute::BCAttribute(const QString& name_, const QString& title_, AttributeType type_/*=Line*/)
    : m_name(name_), m_title(title_),  m_category(i18n("General")),
      m_type(type_), m_flags(0) {

  if(m_type == BCAttribute::Choice) {
    kdWarning() << "BCAttribute() - A different constructor should be called for multiple choice attributes." << endl;
    kdWarning() << "Constructing a BCAttribute with name = " << name_ << endl;
  }
}

// if this constructor is called, the type is necessarily Choice
BCAttribute::BCAttribute(const QString& name_, const QString& title_, const QStringList& allowed_)
    : m_name(name_), m_title(title_), m_category(i18n("General")),
      m_type(BCAttribute::Choice), m_allowed(allowed_), m_flags(0) {
}

BCAttribute::BCAttribute(const BCAttribute&) {
  kdWarning() << "BCAttribute copy constructor - should not be used!!!" << endl;
}

BCAttribute BCAttribute::operator=(const BCAttribute& att_) {
  kdWarning() << "BCAttribute assignment operator - should not be used!!!" << endl;
  return BCAttribute(att_);
}

BCAttribute::AttributeType BCAttribute::type() const{
  return m_type;
}

const QString& BCAttribute::name() const {
  return m_name;
}

const QString& BCAttribute::title() const {
  return m_title;
}

const QString& BCAttribute::category() const {
  return m_category;
}

const QStringList& BCAttribute::allowed() const {
  if(m_type != BCAttribute::Choice) {
    kdWarning() << "BCAttribute::allowed() - attribute is not of type Choice" << endl;
  }
  return m_allowed;
}

int BCAttribute::flags() const {
  return m_flags;
}

const QString& BCAttribute::description() const {
  return m_desc;
}

// changing the name is not a good idea.  I need this because
// between document version 2 and 3, I changed the name of "keywords" to "keyword"
//void BCAttribute::setName(const QString& name_) {
//  m_name = name_;
//}

void BCAttribute::setTitle(const QString& title_) {
  m_title = title_;
}

void BCAttribute::setCategory(const QString& category_) {
  m_category = category_;
}

void BCAttribute::setFlags(int flags_) {
  m_flags = flags_;
}

void BCAttribute::setDescription(const QString& desc_) {
  m_desc = desc_;
}

QString BCAttribute::format(const QString& value_, int flags_) {
  if(value_.isEmpty()) {
    return value_;
  }
  
  QString text;

  if(flags_ & FormatTitle) {
    text = formatTitle(value_);

  } else if(flags_ & FormatName) {
    text = formatName(value_);

  } else if(flags_ & FormatDate) {
    text = formatDate(value_);

  } else {
    text = value_.simplifyWhiteSpace();
  }
  return text;
}

QString BCAttribute::formatTitle(const QString& title_) {
  QString newTitle = title_;
  if(autoCapitalize()) {
    newTitle = capitalize(newTitle);
  }

  // TODO if the title has ",the" at the end, put it at the front
  for(QStringList::Iterator it = m_articles.begin(); it != m_articles.end(); ++it) {
    // assume white space is already stripped
    QString article = static_cast<QString>(*it);
    if(newTitle.startsWith(article + QString::fromLatin1(" "))) {
      QString rx = article;
      rx.prepend(QString::fromLatin1("^")).append(QString::fromLatin1("\\s"));
      QRegExp regexp(rx);
      newTitle = newTitle.replace(regexp, QString()).append(QString::fromLatin1(", ")).append(article);
      break;
    }
  }

  // also, arbitrarily impose rule that a space must follow every comma
  newTitle.replace(QRegExp(QString::fromLatin1("\\s*,\\s*")), QString::fromLatin1(", "));
  return newTitle;
}

QString BCAttribute::formatName(const QString& name_, bool multiple_/*=true*/) {
  QStringList entries;
  if(multiple_) {
    // split by semi-color, optionally precedded or followed by white spacee
    entries = QStringList::split(QRegExp(QString::fromLatin1("\\s*;\\s*")), name_, false);
  } else {
    entries << name_;
  }

  QStringList names;
  QStringList::Iterator it;
  for(it = entries.begin(); it != entries.end(); ++it) {
    QString name = static_cast<QString>(*it);
    if(autoCapitalize()) {
      name = capitalize(name);
    }

    // split the name by white space and commas
    QStringList words = QStringList::split(QRegExp(QString::fromLatin1("[\\s,]")), name, false);
    // if it contains a comma already and the last word is not a suffix, don't format it
    if(name.contains(QString::fromLatin1(",")) > 0
       && m_suffixes.contains(words.last().stripWhiteSpace()) == 0) {
      // arbitrarily impose rule that no spacecs before a comma and
      // a single space after every comma
      name.replace(QRegExp(QString::fromLatin1("\\s*,\\s*")), QString::fromLatin1(", "));
      names << name;
      break;
    }
    // otherwise split it by white space, move the last word to the front
    // but only if there is more than one word
    if(words.count() > 1) {
      // if the last word is a suffix, it has to be kept with last name
      if(m_suffixes.contains(words.last()) > 0) {
        words.prepend(words.last().append(QString::fromLatin1(",")));
        words.remove(words.fromLast());
      }
      words.prepend(words.last().append(QString::fromLatin1(",")));
      words.remove(words.fromLast());
      names << words.join(QString::fromLatin1(" "));
    } else {
      names << name;
    }
  }
  return names.join(QString::fromLatin1("; "));
}

QString BCAttribute::formatDate(const QString& date_) {
  // TODO:: format as a date
  return date_;
}

QString BCAttribute::capitalize(QString str_) {
  if(str_.isEmpty()) {
    return str_;
  }
  // nothing is done to the last character which saves a position check
  // first letter is always capitalized
  str_.replace(0, 1, str_.at(0).upper());

  // regexp to split words
  QRegExp rx(QString::fromLatin1("[\\s,.-;]"));
  int pos = str_.find(rx);
  int nextPos;
  QString word;
  // put into i18n for translation
  // and allow spaces in the regexp, someone might accidently put one there
  QStringList notCap = QStringList::split(QRegExp(QString::fromLatin1("\\s*,\\s*")),
                                          i18n("a,an,and,in,of,the,to"), false);
  while(pos != -1) {
    // also need to compare against list of non-capitalized words
    nextPos = str_.find(rx, pos+1);
    if(nextPos == -1) {
      nextPos = str_.length();
    }
    word = str_.mid(pos+1, nextPos-pos-1);
    if(notCap.contains(word) == 0 && !word.isEmpty()) {
      str_.replace(pos+1, 1, str_.at(pos+1).upper());
    }
    pos = str_.find(rx, pos+1);
  }
  return str_;
}

QStringList BCAttribute::defaultArticleList() {
// put the articles in i18n() so they can be translated
// prob better way to do case insensitive than repeating, but I'm lazy
  return QStringList::split(QRegExp(QString::fromLatin1("\\s*,\\s*")),
                            i18n("The,the"), false);
}

void BCAttribute::setArticleList(const QStringList& list_) {
  m_articles = list_;
}

const QStringList& BCAttribute::articleList() {
  return m_articles;
}

QStringList BCAttribute::defaultSuffixList() {
// put the suffixes in i18n() so they can be translated
// prob better way to do case insensitive than repeating, but I'm lazy
  return QStringList::split(QRegExp(QString::fromLatin1("\\s*,\\s*")),
                            i18n("Jr.,Jr,jr.,jr,III,iii,IV,iv"), false);
}

void BCAttribute::setSuffixList(const QStringList& list_) {
  m_suffixes = list_;
}

const QStringList& BCAttribute::suffixList() {
  return m_suffixes;
}

void BCAttribute::setAutoCapitalize(bool auto_) {
  m_autoCapitalize = auto_;
}

bool BCAttribute::autoCapitalize() {
  return m_autoCapitalize;
}

void BCAttribute::setAutoFormat(bool auto_) {
  m_autoFormat = auto_;
}

bool BCAttribute::autoFormat() {
  return m_autoFormat;
}

