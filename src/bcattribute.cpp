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
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "bcattribute.h"

#include <klocale.h>
#include <kdebug.h>

#include <qstringlist.h>
#include <qregexp.h>

//these get overwritten, but are here to allow them to be static
QStringList BCAttribute::m_articles = QStringList();
QStringList BCAttribute::m_suffixes = QStringList();
bool BCAttribute::m_autoCapitalization = false;

// this constructor is for anything but Choice type
BCAttribute::BCAttribute(const QString& name_, const QString& title_, AttributeType type_/*=Line*/)
 : m_name(name_), m_title(title_), m_type(type_), m_flags(0) {
  m_group = i18n("General");
  if(m_type == BCAttribute::Choice) {
    kdDebug() << "BCAttribute() - A different constructor should be called for multiple choice attributes." << endl;
  }
}

// if this constructor is called, the type is necessarily Choice
BCAttribute::BCAttribute(const QString& name_, const QString& title_, const QStringList& allowed_)
 : m_name(name_), m_title(title_), m_allowed(allowed_), m_flags(0) {
  m_group = i18n("General");
  m_type = BCAttribute::Choice;
}

BCAttribute::BCAttribute(const BCAttribute&) {
  kdWarning() << "BCAttribute copy constructor - should not be used!!!" << endl;
}

BCAttribute BCAttribute::operator=(const BCAttribute& att_) {
  kdWarning() << "BCAttribute assignment operator - should not be used!!!" << endl;
  return BCAttribute(att_);
}

BCAttribute::~BCAttribute() {
}

BCAttribute::AttributeType BCAttribute::type() const {
  return m_type;
}

const QString& BCAttribute::name() const {
  return m_name;
}

const QString& BCAttribute::title() const {
  return m_title;
}

const QString& BCAttribute::group() const {
  return m_group;
}

const QStringList& BCAttribute::allowed() const {
  if(m_type != BCAttribute::Choice) {
    kdDebug() << "BCAttribute::allowed() - attribute is not of type Choice" << endl;
  }
  return m_allowed;
}

int BCAttribute::flags() const {
  return m_flags;
}

const QString& BCAttribute::description() const {
  return m_desc;
}

void BCAttribute::setTitle(const QString& title_) {
  m_title = title_;
}

void BCAttribute::setGroup(const QString& group_) {
  m_group = group_;
}

void BCAttribute::setAllowed(const QStringList& list_) {
  m_allowed = list_;
}

void BCAttribute::setFlags(int flags_) {
  m_flags = flags_;
}

void BCAttribute::setDescription(const QString& desc_) {
  m_desc = desc_;
}

QString BCAttribute::formatTitle(const QString& title_) {
  // yeah, this defeats purpose of const modifier in method parameter...so?
  QString text = title_;
  if(isAutoCapitalization()) {
    text = BCAttribute::capitalize(text);
  }

  // TODO if the title has ",the" at the end, put it at the front
  for(QStringList::Iterator it = m_articles.begin(); it != m_articles.end(); ++it) {
    // assume white space is already stripped
    QString article = static_cast<QString>(*it);
    if(text.startsWith(article + " ")) {
      QRegExp regexp = QRegExp("^" + article + "\\s");
      text = text.replace(regexp, "") + ", " + article;
      break;
    }
  }

  // also, arbitrarily impose rule that a space must follow every comma
  // lazy method, replace comma with comma<space> and then simplifyWhiteSpace()
  text.replace(QRegExp(","), ", ");
  return text.simplifyWhiteSpace();
}

QString BCAttribute::formatName(const QString& name_, bool multiple_/*=true*/) {
  QStringList entries;
  if(multiple_) {
    entries = QStringList::split(";", name_, false);
  } else {
    entries << name_;
  }

  QStringList text;
  QStringList::Iterator it;
  for(it = entries.begin(); it != entries.end(); ++it) {
    QString name = static_cast<QString>(*it);
    if(isAutoCapitalization()) {
      name = BCAttribute::capitalize(name);
    }

    // split the name by white space and commas
    QStringList words = QStringList::split(QRegExp("[\\s,]"), name, false);
    // if it contains a comma already and the last word is not a suffix, don't format it
    if(name.contains(",") > 0 && m_suffixes.contains(words.last().stripWhiteSpace()) == 0) {
      // arbitrarily impose rule that a space must follow every comma
      // lazy method, replace comma with comma<space> and then simplifyWhiteSpace()
      name.replace(QRegExp(","), ", ");
      text << name.simplifyWhiteSpace();
      break;
    }
    // otherwise split it by white space, move the last word to the front
    // but only if there is more than one word
    if(words.count() > 1) {
      // if the last word is a suffix, it has to be kept with last name
      if(m_suffixes.contains(words.last().stripWhiteSpace()) > 0) {
        words.prepend(words.last() + ",");
        words.remove(words.fromLast());
      }
      words.prepend(words.last() + ",");
      words.remove(words.fromLast());
      text << words.join(" ").simplifyWhiteSpace();
    } else {
      text << name;
    }
  }
  return text.join("; ").simplifyWhiteSpace();
}

QString BCAttribute::formatDate(const QString& date_) {
  // TODO:: format as a date
  return date_;
}

QString BCAttribute::capitalize(const QString& str_) {
  // nothing is done to the last character which saves a position check
  QString s = str_;
  // first letter is always capitalized
  s.replace(0, 1, s.at(0).upper());

  // regexp to split words
  QRegExp rx("[\\s,.-;]");
  int pos = s.find(rx);
  int nextPos;
  QString word;
  // put into i18n for translation
  QStringList notCap = QStringList::split(",", i18n("a,an,in,of,the,to"), false);
  while(pos != -1) {
    // also need to compare against list of non-capitalized words
    nextPos = s.find(rx, pos+1);
    if(nextPos == -1) {
      nextPos = s.length();
    }
    word = s.mid(pos+1, nextPos-pos-1);
    if(notCap.contains(word) == 0 && !word.isEmpty()) {
      s.replace(pos+1, 1, s.at(pos+1).upper());
    }
    pos = s.find(rx, pos+1);
  }
  return s;
}

QStringList BCAttribute::defaultArticleList() {
// put the articles in i18n() so they can be translated
// prob better way to do case insensitive than repeating, but I'm lazy
  return QStringList::split(",", i18n("The,the"), false);
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
  return QStringList::split(",", i18n("Jr.,Jr,jr.,jr,III,iii,IV,iv"), false);
}

void BCAttribute::setSuffixList(const QStringList& list_) {
  m_suffixes = list_;
}

const QStringList& BCAttribute::suffixList() {
  return m_suffixes;
}

void BCAttribute::setAutoCapitalization(bool auto_) {
  m_autoCapitalization = auto_;
}

bool BCAttribute::isAutoCapitalization() {
  return m_autoCapitalization;
}
