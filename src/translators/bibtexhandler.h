/***************************************************************************
                               bibtexhandler.h
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

#ifndef BIBTEXHANDLER_H
#define BIBTEXHANDLER_H

class BCUnit;

class QString;
class QStringList;

#include <qmap.h>

typedef QMap<QString, QStringList> StringListMap;

/**
 * @author Robby Stephenson
 * @version $Id: bibtexhandler.h 261 2003-11-06 05:15:35Z robby $
 */
class BibtexHandler {
public:
  enum QuoteStyle { BRACES=0, QUOTES=1 };
  static QString bibtexKey(BCUnit* unit);
  static QString bibtexKey(const QString& author, const QString& title, const QString& year);
  static QString importText(char* text);
  static QString exportText(const QString& text, const QStringList& macros);
  static bool setAttributeValue(BCUnit* unit, const QString& bibtexField, const QString& value);
  /**
   * Strips the text of all vestiges of Latex.
   *
   * @param text A reference to the text
   * @return A reference to the text
   */
  static QString& cleanText(QString& text);

  static const QString s_bibtexmlNamespace;

private:
  static void loadTranslationMaps();
  static StringListMap s_utf8LatexMap;
  static QuoteStyle s_quoteStyle;
};

#endif
