/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>
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

#include "tellico_config.h"
#include "../collection.h"

#define COLL Data::Collection::
#define CLASS Config::
#define P1 (
#define P2 )
#define CASE1(a,b) case COLL b: return CLASS a ## b P1 P2;
#define CASE2(a,b,c) case COLL b: CLASS a ## b P1 c P2; break;
#define GET(name,type) \
  CASE1(name,type)
#define SET(name,type,value) \
  CASE2(name,type,value)
#define ALL_GET(name) \
  GET(name, Base) \
  GET(name, Book) \
  GET(name, Video) \
  GET(name, Album) \
  GET(name, Bibtex) \
  GET(name, ComicBook) \
  GET(name, Wine) \
  GET(name, Coin) \
  GET(name, Stamp) \
  GET(name, Card) \
  GET(name, Game) \
  GET(name, File) \
  GET(name, BoardGame)
#define ALL_SET(name,value) \
  SET(name, Base, value) \
  SET(name, Book, value) \
  SET(name, Video, value) \
  SET(name, Album, value) \
  SET(name, Bibtex, value) \
  SET(name, ComicBook, value) \
  SET(name, Wine, value) \
  SET(name, Coin, value) \
  SET(name, Stamp, value) \
  SET(name, Card, value) \
  SET(name, Game, value) \
  SET(name, File, value) \
  SET(name, BoardGame, value)

using Tellico::Config;

QStringList Config::m_noCapitalizationList;
QStringList Config::m_articleList;
QStringList Config::m_articleAposList;
QStringList Config::m_nameSuffixList;
QStringList Config::m_surnamePrefixList;
QStringList Config::m_surnamePrefixTokens;

QRegularExpression Config::commaSplit() {
  static const QRegularExpression rx(QLatin1String("\\s*,\\s*"));
  return rx;
}

void Config::checkArticleList() {
  // I don't know of a way to update the list when the string changes
  // so just keep a cached copy
  static QString cacheValue;
  if(cacheValue != Config::articlesString()) {
    cacheValue = Config::articlesString();
    m_articleList = cacheValue.split(commaSplit());
    m_articleAposList.clear();
    foreach(const QString& article, m_articleList) {
      if(article.endsWith(QLatin1Char('\''))) {
        m_articleAposList += article;
      }
    }
  }
}

QStringList Config::noCapitalizationList() {
  static QString cacheValue;
  if(cacheValue != Config::noCapitalizationString()) {
    cacheValue = Config::noCapitalizationString();
    m_noCapitalizationList = cacheValue.split(commaSplit());
  }
  return m_noCapitalizationList;
}

QStringList Config::articleList() {
  // articles should all be in lower-case
  checkArticleList();
  return m_articleList;
}

QStringList Config::articleAposList() {
  checkArticleList();
  return m_articleAposList;
}

QStringList Config::nameSuffixList() {
  static QString cacheValue;
  if(cacheValue != Config::nameSuffixesString()) {
    cacheValue = Config::nameSuffixesString();
    m_nameSuffixList = cacheValue.split(commaSplit());
  }
  return m_nameSuffixList;
}

QStringList Config::surnamePrefixList() {
  static QString cacheValue;
  if(cacheValue != Config::surnamePrefixesString()) {
    cacheValue = Config::surnamePrefixesString();
    m_surnamePrefixList = cacheValue.split(commaSplit());
  }
  return m_surnamePrefixList;
}

// In a previous version of Tellico, using a prefix such as "van der" (with a space) would work
// because QStringList::contains did substring matching, but now need to add a function for tokenizing
// the list with whitespace as well as comma
QStringList Config::surnamePrefixTokens() {
  static QString cacheValue;
  if(cacheValue != Config::surnamePrefixesString()) {
    cacheValue = Config::surnamePrefixesString();
    static const QRegularExpression commaSpaceSplit(QLatin1String("\\s*[, ]\\s*"));
    m_surnamePrefixTokens = cacheValue.split(commaSpaceSplit);
  }
  return m_surnamePrefixTokens;
}

QString Config::templateName(int type_) {
  switch(type_) {
    ALL_GET(template);
  }
  return QString();
}

QFont Config::templateFont(int type_) {
  switch(type_) {
    ALL_GET(font);
  }
  return QFont();
}

QColor Config::templateBaseColor(int type_) {
  switch(type_) {
    ALL_GET(baseColor)
  }
  return QColor();
}

QColor Config::templateTextColor(int type_) {
  switch(type_) {
    ALL_GET(textColor)
  }
  return QColor();
}

QColor Config::templateHighlightedBaseColor(int type_) {
  switch(type_) {
    ALL_GET(highlightedBaseColor)
  }
  return QColor();
}

QColor Config::templateHighlightedTextColor(int type_) {
  switch(type_) {
    ALL_GET(highlightedTextColor)
  }
  return QColor();
}

QColor Config::templateLinkColor(int type_) {
  switch(type_) {
    ALL_GET(linkColor)
  }
  return QColor();
}

void Config::setTemplateName(int type_, const QString& name_) {
  switch(type_) {
    ALL_SET(setTemplate,name_)
  }
}

void Config::setTemplateFont(int type_, const QFont& font_) {
  switch(type_) {
    ALL_SET(setFont,font_)
  }
}

void Config::setTemplateBaseColor(int type_, const QColor& color_) {
  switch(type_) {
    ALL_SET(setBaseColor,color_)
  }
}

void Config::setTemplateTextColor(int type_, const QColor& color_) {
  switch(type_) {
    ALL_SET(setTextColor,color_)
  }
}

void Config::setTemplateHighlightedBaseColor(int type_, const QColor& color_) {
  switch(type_) {
    ALL_SET(setHighlightedBaseColor,color_)
  }
}

void Config::setTemplateHighlightedTextColor(int type_, const QColor& color_) {
  switch(type_) {
    ALL_SET(setHighlightedTextColor,color_)
  }
}

void Config::setTemplateLinkColor(int type_, const QColor& color_) {
  switch(type_) {
    ALL_SET(setLinkColor,color_)
  }
}

#undef COLL
#undef CLASS
#undef P1
#undef P2
#undef CASE1
#undef CASE2
#undef GET
#undef SET
#undef ALL_GET
#undef ALL_SET
