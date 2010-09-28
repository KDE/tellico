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

// this file gets included by tellico_config.h
#ifndef TELLICO_CONFIG_ADDONS_H
#define TELLICO_CONFIG_ADDONS_H

public:
  static QStringList noCapitalizationList();
  static QStringList articleList();
  static QStringList articleAposList();
  static QStringList nameSuffixList();
  static QStringList surnamePrefixList();

  static QString templateName(int type);
  static QFont templateFont(int type);
  static QColor templateBaseColor(int type);
  static QColor templateTextColor(int type);
  static QColor templateHighlightedBaseColor(int type);
  static QColor templateHighlightedTextColor(int type);

  static void setTemplateName(int type, const QString& name);
  static void setTemplateFont(int type, const QFont& font);
  static void setTemplateBaseColor(int type, const QColor& color);
  static void setTemplateTextColor(int type, const QColor& color);
  static void setTemplateHighlightedBaseColor(int type, const QColor& color);
  static void setTemplateHighlightedTextColor(int type, const QColor& color);

private:
  static void checkArticleList();

  static QStringList m_articleList;
  // need to remember articles with apostrophes for capitalization
  static QStringList m_articleAposList;

#endif
