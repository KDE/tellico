/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

// this file gets included by tellico_config.h

public:
  static void deleteAndReset();

  static QStringList noCapitalizationList();
  static QStringList articleList();
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
