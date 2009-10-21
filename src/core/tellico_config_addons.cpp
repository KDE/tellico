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

namespace {
  static const QRegExp commaSplit = QRegExp(QLatin1String("\\s*,\\s*"));
}

using Tellico::Config;

QStringList Config::m_articleList;
QStringList Config::m_articleAposList;

QStringList Config::noCapitalizationList() {
  return Config::noCapitalizationString().split(commaSplit);
}

void Config::checkArticleList() {
  // I don't know of a way to update the list when the string changes
  // so just keep a cached copy
  static QString cacheValue;
  if(cacheValue != Config::articlesString()) {
    m_articleList = Config::articlesString().split(commaSplit);
    m_articleAposList.clear();
    foreach(const QString& article, m_articleList) {
      if(article.endsWith(QLatin1Char('\''))) {
        m_articleAposList += article;
      }
    }
  }
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
  return Config::nameSuffixesString().split(commaSplit);
}

QStringList Config::surnamePrefixList() {
  return Config::surnamePrefixesString().split(commaSplit);
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
