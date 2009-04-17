/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_BIBTEXHANDLER_H
#define TELLICO_BIBTEXHANDLER_H

#include "../datavectors.h"

#include <QStringList>
#include <QHash>
#include <QRegExp>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class BibtexHandler {
public:
  enum QuoteStyle { BRACES=0, QUOTES=1 };
  static QStringList bibtexKeys(const Data::EntryList& entries);
  static QString bibtexKey(Data::EntryPtr entry);
  static QString importText(char* text);
  static QString exportText(const QString& text, const QStringList& macros);
  /**
   * Strips the text of all vestiges of LaTeX.
   *
   * @param text A reference to the text
   * @return A reference to the text
   */
  static QString& cleanText(QString& text);

  static QuoteStyle s_quoteStyle;

private:
  typedef QHash<QString, QStringList> StringListHash;

  static QString bibtexKey(const QString& author, const QString& title, const QString& year);
  static void loadTranslationMaps();
  static QString addBraces(const QString& string);

  static StringListHash* s_utf8LatexMap;
  static const QRegExp s_badKeyChars;
};

} // end namespace
#endif
