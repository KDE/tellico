/***************************************************************************
                          bcattribute.cpp  -  description
                             -------------------
    begin                : Sun Sep 23 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@radiojodi.com
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

// this constructor is for anything but Choice type
BCAttribute::BCAttribute(QString name_, QString title_, AttributeType type_/*=Line*/)
 : m_name(name_), m_title(title_), m_type(type_), m_flags(0) {
  m_group = i18n("General");
  if(m_type == BCAttribute::Choice) {
    kdDebug() << "BCAttribute() - A different constructor should be called for multiple choice attributes." << endl;
  }
}

// if this constructor is called, the type is necessarily Choice
BCAttribute::BCAttribute(QString name_, QString title_, QStringList allowed_)
 : m_name(name_), m_title(title_), m_allowed(allowed_), m_flags(0) {
  m_group = i18n("General");
  m_type = BCAttribute::Choice;
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

QString BCAttribute::formatTitle(const QString& title_) {
  // TODO if the title has ",the" at the end, put it at the front
  // put the articles in i18n() so they can be translated
  // prob better way to do case insensitive then repeating, but I'm lazy
  QStringList articles = QStringList::split(":", i18n("The:the"), false);
  QString text = title_;
  for(QStringList::Iterator it = articles.begin(); it != articles.end(); ++it) {
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
  return text.simplifyWhiteSpace();;
}

QString BCAttribute::formatName(const QString& name_, bool multiple_/*=true*/) {
  // TODO:: figure out how to do personal name formatting
  // such as: "Robby Stephenson" -> "Stephenson, Robby"
  QStringList entries;
  if(multiple_) {
    entries = QStringList::split(";", name_, false);
  } else {
    entries << name_;
  }

  QStringList text;
  for(QStringList::Iterator it = entries.begin(); it != entries.end(); ++it) {
    // if it contains a comma already, don't format it
    if(static_cast<QString>(*it).contains(",") > 0) {
      // arbitrarily impose rule that a space must follow every comma
      // lazy method, replace comma with comma<space> and then simplifyWhiteSpace()
      QString name = static_cast<QString>(*it);
      name.replace(QRegExp(","), ", ");
      name.simplifyWhiteSpace();
      text << name;
      break;
    }
    // otherwise split it by white space, move the last word to the front
    QStringList words = QStringList::split(QRegExp("\\s"), static_cast<QString>(*it), false);
    if(words.count() > 1) {
      // only add comma if the entry has more than one word
      words.prepend(words.last() + ",");
      words.remove(words.fromLast());
    }
    text << words.join(" ").simplifyWhiteSpace();
  }
  return text.join("; ").simplifyWhiteSpace();
}

QString BCAttribute::formatDate(const QString& date_) {
  // TODO:: format as a date
  return date_;
}
