/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
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

#include <qmap.h>

namespace Tellico {
  namespace Data {
    class Entry;
  }

/**
 * @author Robby Stephenson
 * @version $Id: bibtexhandler.h 1063 2005-02-02 01:59:11Z robby $
 */
class BibtexHandler {
public:
  enum QuoteStyle { BRACES=0, QUOTES=1 };
  static QString bibtexKey(Data::Entry* entry);
  static QString importText(char* text);
  static QString exportText(const QString& text, const QStringList& macros);
  static bool setFieldValue(Data::Entry* entry, const QString& bibtexField, const QString& value);
  /**
   * Strips the text of all vestiges of Latex.
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
  static StringListMap* s_utf8LatexMap;
  static const QRegExp s_badKeyChars;
};

} // end namespace
#endif
