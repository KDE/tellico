/***************************************************************************
                               bcfilehandler.h
                             -------------------
    begin                : Sun Oct 12 2003
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

#ifndef BCFILEUTILS_H
#define BCFILEUTILS_H

class KURL;

class QString;
class QDomDocument;
class QFile;

/**
 * The BCFileHandler class contains some utility functions for reading files.
 *
 * @author Robby Stephenson
 * @version $Id: bcfilehandler.h 281 2003-11-09 21:17:14Z robby $
 */
class BCFileHandler {

  friend class Bookcase;

public:
  /**
   * Read contents of a file into a string.
   *
   * @param url The URL of the file
   * @return A string containing the contents of a file
   */
  static QString readFile(const KURL& url);
  /**
   * Read contents of an XML file into a QDomDocument.
   *
   * @param url The URL of the file
   * @return A QDomDocument containing the contents of a file
   */
  static QDomDocument readXMLFile(const KURL& url);
  /**
   * Writes the contents of a string to a url. If the file already exists, a "~" is appended
   * and the existing file is moved. If the file is remote, a temporary file is written and
   * then uploaded.
   *
   * @param url The url
   * @param text The text
   * @param localeEncoding Whether to use Locale encoding, or UTF-8 by default
   * @return A boolean indicating success
   */
  static bool writeURL(const KURL& url, const QString& text, bool localeEncoding);

private:
  /**
   * Writes the contents of a string to a file.
   *
   * @param file The file object
   * @param text The string
   * @param localeEncoding Whether to use Locale encoding, or UTF-8 by default
   * @return A boolean indicating success
   */
  static bool writeFile(QFile& file, const QString& text, bool localeEncoding);

  static Bookcase* s_bookcase;
};
#endif
