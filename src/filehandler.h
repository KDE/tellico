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

#ifndef BCFILEUTILS_H
#define BCFILEUTILS_H

class KURL;
class KSaveFile;

class QString;
class QDomDocument;
class QFile;

#include <qstring.h>
#include <qcstring.h> // needed for QByteArray

namespace Bookcase {
  namespace Data {
    class Image;
  }

/**
 * The FileHandler class contains some utility functions for reading files.
 *
 * @author Robby Stephenson
 * @version $Id: filehandler.h 386 2004-01-24 05:12:28Z robby $
 */
class FileHandler {

friend class MainWindow;
friend const Data::Image& addImage(const KURL& url);

public:
  /**
   * Read contents of a file into a string.
   *
   * @param url The URL of the file
   * @return A string containing the contents of a file
   */
  static QString readTextFile(const KURL& url);
  /**
   * Read contents of an XML file into a QDomDocument.
   *
   * @param url The URL of the file
   * @return A QDomDocument containing the contents of a file
   */
  static QDomDocument readXMLFile(const KURL& url);
  /**
   * Read contents of a data file into a QByteArray.
   *
   * @param url The URL of the file
   * @return A QByteArray of the file's contents
   */
  static QByteArray readDataFile(const KURL& url);
  /**
   * Writes the contents of a string to a url. If the file already exists, a "~" is appended
   * and the existing file is moved. If the file is remote, a temporary file is written and
   * then uploaded.
   *
   * @param url The url
   * @param text The text
   * @param localeEncoding Whether to use Locale encoding, or UTF-8
   * @return A boolean indicating success
   */
  static bool writeTextURL(const KURL& url, const QString& text, bool localeEncoding);
  /**
   * Writes data to a url. If the file already exists, a "~" is appended
   * and the existing file is moved. If the file is remote, a temporary file is written and
   * then uploaded.
   *
   * @param url The url
   * @param data The data
   * @return A boolean indicating success
   */
  static bool writeDataURL(const KURL& url, const QByteArray& data);

private:
  /**
   * An internal class to handle KIO stuff.
   */
  class FileRef {
    friend class FileHandler;
    FileRef(const KURL& url);
    ~FileRef();
    QFile* file;
    QString filename;
    bool isValid;
  };

  /**
   * Read contents of a file into an image. It's private since everything should use the
   * ImageFactory methods.
   *
   * @param url The URL of the file
   * @return The image
   */
  static Data::Image* readImageFile(const KURL& url);
  /**
   * Checks to see if a URL exists already, and if so, queries the user.
   *
   * @param url The target URL
   * @return True if it is ok to continue, false otherwise.
   */
  static bool queryExists(const KURL& url);
  /**
   * Writes the contents of a string to a file.
   *
   * @param file The file object
   * @param text The string
   * @param localeEncoding Whether to use Locale encoding, or UTF-8 by default
   * @return A boolean indicating success
   */
  static bool writeTextFile(KSaveFile& file, const QString& text, bool localeEncoding);
  /**
   * Writes data to a file.
   *
   * @param file The file object
   * @param data The data
   * @return A boolean indicating success
   */
  static bool writeDataFile(KSaveFile& file, const QByteArray& data);

  static MainWindow* s_mainWindow;
};

} // end namespace
#endif
