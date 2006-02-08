/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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

class QString;
class QStringList;
class QRegExp;

#include "../datavectors.h"

#include <qmap.h>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class BibtexHandler {
public:
  enum QuoteStyle { BRACES=0, QUOTES=1 };
  static QStringList bibtexKeys(const Data::EntryVec& entries);
  static QString bibtexKey(Data::ConstEntryPtr entry);
  static QString importText(char* text);
  static QString exportText(const QString& text, const QStringList& macros);
  static bool setFieldValue(Data::EntryPtr entry, const QString& bibtexField, const QString& value);
  /**
   * Strips the text of all vestiges of LaTeX.
   *
   * @param text A reference to the text
   * @return A reference to the text
   */
  static QString& cleanText(QString& text);

  static QuoteStyle s_quoteStyle;

private:
  typedef QMap<QString, QStringList> StringListMap;

  static QString bibtexKey(const QString& author, const QString& title, const QString& year);
  static void loadTranslationMaps();
  static QString& addBraces(QString& string);

  static StringListMap* s_utf8LatexMap;
  static const QRegExp s_badKeyChars;
};

} // end namespace
#endif
