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
QStringList BCAttribute::m_surnamePrefixes = QStringList();
// put into i18n for translation
// and allow spaces in the regexp, someone might accidently put one there
QStringList BCAttribute::m_noCapitalize = QStringList::split(QRegExp(QString::fromLatin1("\\s*,\\s*")),
                                          i18n("a,an,and,in,of,the,to"), false);
bool BCAttribute::m_autoCapitalize = true;
bool BCAttribute::m_autoFormat = true;

// this constructor is for anything but Choice type
BCAttribute::BCAttribute(const QString& name_, const QString& title_, AttributeType type_/*=Line*/)
    : m_name(name_), m_title(title_),  m_category(i18n("&General")), m_desc(title_),
      m_type(type_), m_flags(0), m_formatFlag(FormatPlain) {

  if(m_type == Choice) {
    kdWarning() << "BCAttribute() - A different constructor should be called for multiple choice attributes." << endl;
    kdWarning() << "Constructing a BCAttribute with name = " << name_ << endl;
  }
  // a paragraph's category is always it's title
  if(m_type == Para) {
    m_category = m_title;
  }
}

// if this constructor is called, the type is necessarily Choice
BCAttribute::BCAttribute(const QString& name_, const QString& title_, const QStringList& allowed_)
    : m_name(name_), m_title(title_), m_category(i18n("&General")), m_desc(title_),
      m_type(BCAttribute::Choice), m_allowed(allowed_), m_flags(0), m_formatFlag(FormatPlain) {
}

BCAttribute::BCAttribute(const BCAttribute& att_)
    : m_name(att_.name()), m_title(att_.title()), m_category(att_.category()),
      m_desc(att_.description()), m_type(att_.type()),
      m_flags(att_.flags()), m_formatFlag(att_.formatFlag()) {
  if(m_type == Choice) {
    m_allowed = att_.allowed();
  }
}

BCAttribute& BCAttribute::operator=(const BCAttribute& att_) {
  if(this != &att_) {
    m_name = att_.name();
    m_title = att_.title();
    m_category = att_.category();
    m_desc = att_.description();
    m_type = att_.type();
    if(m_type == Choice) {
      m_allowed = att_.allowed();
    }
    m_flags = att_.flags();
    m_formatFlag = att_.formatFlag();
  }
  return *this;
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

BCAttribute::FormatFlag BCAttribute::formatFlag() const {
  return m_formatFlag;
}

const QString& BCAttribute::description() const {
  return m_desc;
}

// changing the name is not a good idea. Only do it before the attribute is in a collection
void BCAttribute::setName(const QString& name_) {
  m_name = name_;
}

void BCAttribute::setTitle(const QString& title_) {
  m_title = title_;
}

void BCAttribute::setType(BCAttribute::AttributeType type_) {
  m_type = type_;
  if(m_type != BCAttribute::Choice) {
    m_allowed = QString::null;
  }
}

void BCAttribute::setAllowed(const QStringList& allowed_) {
  m_allowed = allowed_;
}

void BCAttribute::setCategory(const QString& category_) {
  // a paragraph's category should be it's title, but this isn't enforced here
  m_category = category_;
}

void BCAttribute::setFlags(int flags_) {
  m_flags = flags_;
}

void BCAttribute::setFormatFlag(FormatFlag flag_) {
  m_formatFlag = flag_;
}

void BCAttribute::setDescription(const QString& desc_) {
  m_desc = desc_;
}

QString BCAttribute::format(const QString& value_, FormatFlag flag_) {
  if(value_.isEmpty()) {
    return value_;
  }
  
  QString text;

  if(flag_ == FormatTitle) {
    text = formatTitle(value_);

  } else if(flag_ == FormatName) {
    text = formatName(value_);

  } else if(flag_ == FormatDate) {
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
  QStringList::ConstIterator it;
  for(it = m_articles.begin(); it != m_articles.end(); ++it) {
    // assume white space is already stripped
    if(newTitle.startsWith(*it + QString::fromLatin1(" "))) {
      QString rx(*it);
      rx.prepend(QString::fromLatin1("^")).append(QString::fromLatin1("\\s"));
      QRegExp regexp(rx);
      newTitle = newTitle.replace(regexp, QString()).append(QString::fromLatin1(", ")).append(*it);
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
    // split by semi-color, optionally preceded or followed by white spacee
    entries = QStringList::split(QRegExp(QString::fromLatin1("\\s*;\\s*")), name_, false);
  } else {
    entries << name_;
  }

  QStringList names;
  QStringList::ConstIterator it;
  for(it = entries.begin(); it != entries.end(); ++it) {
    QString name = *it;
    if(autoCapitalize()) {
      name = capitalize(name);
    }

    // split the name by white space and commas
    QStringList words = QStringList::split(QRegExp(QString::fromLatin1("[\\s,]")), name, false);
    // if it contains a comma already and the last word is not a suffix, don't format it
    if(name.contains(QString::fromLatin1(",")) > 0
       && m_suffixes.grep(words.last(), false).isEmpty()) {
      // arbitrarily impose rule that no spaces before a comma and
      // a single space after every comma
      QRegExp spaceComma(QString::fromLatin1("\\s*,\\s*"));
      name.replace(spaceComma, QString::fromLatin1(", "));
      names << name;
      break;
    }
    // otherwise split it by white space, move the last word to the front
    // but only if there is more than one word
    if(words.count() > 1) {
      // if the last word is a suffix, it has to be kept with last name
      if(m_suffixes.grep(words.last(), false).count() > 0) {
        words.prepend(words.last().append(QString::fromLatin1(",")));
        words.remove(words.fromLast());
      }
      
      // now move the word
      words.prepend(words.last().append(QString::fromLatin1(",")));
      words.remove(words.fromLast());

      // this is probably just soemthing for me, limited to english
      while(m_surnamePrefixes.grep(words.last(), false).count() > 0) {
        words.prepend(words.last());
        words.remove(words.fromLast());
      }
            
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

  QStringList notCap = m_noCapitalize;
  // don't capitalize the surname prefixes
  // does this hold true everywhere other than english?
  notCap += BCAttribute::surnamePrefixList();
  
  while(pos != -1) {
    // also need to compare against list of non-capitalized words
    nextPos = str_.find(rx, pos+1);
    if(nextPos == -1) {
      nextPos = str_.length();
    }
    word = str_.mid(pos+1, nextPos-pos-1);
    if(notCap.grep(word, false).isEmpty() && !word.isEmpty()) {
      str_.replace(pos+1, 1, str_.at(pos+1).upper());
    }
    pos = str_.find(rx, pos+1);
  }
  return str_;
}

QStringList BCAttribute::defaultArticleList() {
// put the articles in i18n() so they can be translated
  return QStringList::split(QRegExp(QString::fromLatin1("\\s*,\\s*")),
                            i18n("the"), false);
}

void BCAttribute::setArticleList(const QStringList& list_) {
  m_articles = list_;
}

const QStringList& BCAttribute::articleList() {
  return m_articles;
}

QStringList BCAttribute::defaultSuffixList() {
// put the suffixes in i18n() so they can be translated
  return QStringList::split(QRegExp(QString::fromLatin1("\\s*,\\s*")),
                            i18n("jr.,jr,iii,iv"), false);
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

QStringList BCAttribute::defaultSurnamePrefixList() {
// put the articles in i18n() so they can be translated
  return QStringList::split(QRegExp(QString::fromLatin1("\\s*,\\s*")),
                            i18n("de,van der,von"), false);
}

void BCAttribute::setSurnamePrefixList(const QStringList& list_) {
  m_surnamePrefixes = list_;
}

const QStringList& BCAttribute::surnamePrefixList() {
  return m_surnamePrefixes;
}

// if these are changed, then BCCollectionPropDialog should be checked since it
// checks for equality against some of these strings
QMap<BCAttribute::AttributeType, QString> BCAttribute::typeMap() {
  QMap<BCAttribute::AttributeType, QString> map;
  map[BCAttribute::Line] = i18n("Simple Text");
  map[BCAttribute::Para] = i18n("Paragraph");
  map[BCAttribute::Choice] = i18n("List");
  map[BCAttribute::Bool] = i18n("Checkbox"); 
  map[BCAttribute::Year] = i18n("Year");
  map[BCAttribute::URL] = i18n("URL");
  return map;
}

