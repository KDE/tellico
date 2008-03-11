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

#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include <qstring.h>
#include <qcstring.h> // needed for QByteArray
#include <qptrlist.h>

class KURL;
class KSaveFile;
class KFileItem;
namespace KIO {
  class Job;
}

class QDomDocument;
class QIODevice;

namespace Tellico {
  class ImageFactory;
  namespace Data {
    class Image;
  }

/**
 * The FileHandler class contains some utility functions for reading files.
 *
 * @author Robby Stephenson
 */
class FileHandler {

friend class ImageFactory;

public:
  /**
   * An internal class to handle KIO stuff. Exposed so a FileRef pointer
   * can be returned from FileHandler.
   */
  class FileRef {
  public:
    bool open(bool quiet=false);
    QIODevice* file() const { return m_device; }
    const QString& fileName() const { return m_filename; }
    bool isValid() const { return m_isValid; }
    ~FileRef();

  private:
    friend class FileHandler;
    FileRef(const KURL& url, bool quiet=false, bool allowCompressed=false);
    QIODevice* m_device;
    QString m_filename;
    bool m_isValid;
  };
  friend class FileRef;

  /**
   * Creates a FileRef for a given url. It's not meant to be used by methods in the class,
   * Rather by a class wanting direct access to a file. The caller takes ownership of the pointer.
   *
   * @param url The url
   * @param quiet Whether error messages should be shown
   * @return The fileref
   */
  static FileRef* fileRef(const KURL& url, bool quiet=false);
  /**
   * Read contents of a file into a string.
   *
   * @param url The URL of the file
   * @param quiet whether the importer should report errors or not
   * @param useUTF8 Whether the file should be read as UTF8 or use user locale
   * @param allowCompressed Whether to check if the file is compressed or not
   * @return A string containing the contents of a file
   */
  static QString readTextFile(const KURL& url, bool quiet=false, bool useUTF8=false, bool allowCompressed=false);
  /**
   * Read contents of an XML file into a QDomDocument.
   *
   * @param url The URL of the file
   * @param processNamespace Whether to process the namespace of the XML file
   * @param quiet Whether error messages should be shown
   * @return A QDomDocument containing the contents of a file
   */
  static QDomDocument readXMLFile(const KURL& url, bool processNamespace, bool quiet=false);
  /**
   * Read contents of a data file into a QByteArray.
   *
   * @param url The URL of the file
   * @param quiet Whether error messages should be shown
   * @return A QByteArray of the file's contents
   */
  static QByteArray readDataFile(const KURL& url, bool quiet=false);
  /**
   * Writes the contents of a string to a url. If the file already exists, a "~" is appended
   * and the existing file is moved. If the file is remote, a temporary file is written and
   * then uploaded.
   *
   * @param url The url
   * @param text The text
   * @param encodeUTF8 Whether to use UTF-8 encoding, or Locale
   * @param force Whether to force the write
   * @return A boolean indicating success
   */
  static bool writeTextURL(const KURL& url, const QString& text, bool encodeUTF8, bool force=false, bool quiet=false);
  /**
   * Writes data to a url. If the file already exists, a "~" is appended
   * and the existing file is moved. If the file is remote, a temporary file is written and
   * then uploaded.
   *
   * @param url The url
   * @param data The data
   * @param force Whether to force the write
   * @return A boolean indicating success
   */
  static bool writeDataURL(const KURL& url, const QByteArray& data, bool force=false, bool quiet=false);
  /**
   * Checks to see if a URL exists already, and if so, queries the user.
   *
   * @param url The target URL
   * @return True if it is ok to continue, false otherwise.
   */
  static bool queryExists(const KURL& url);
  static void clean();

private:
  class ItemDeleter;
  friend class ItemDeleter;
  static QPtrList<ItemDeleter> s_deleterList;

  /**
   * Read contents of a file into an image. It's private since everything should use the
   * ImageFactory methods.
   *
   * @param url The URL of the file
   * @param quiet If errors should be quiet
   * @return The image
   */
  static Data::Image* readImageFile(const KURL& url, bool quiet=false);
  static Data::Image* readImageFile(const KURL& url, bool quiet, const KURL& referrer);
  /**
   * Writes the contents of a string to a file.
   *
   * @param file The file object
   * @param text The string
   * @param encodeUTF8 Whether to use UTF-8 encoding, or Locale
   * @return A boolean indicating success
   */
  static bool writeTextFile(KSaveFile& file, const QString& text, bool encodeUTF8);
  /**
   * Writes data to a file.
   *
   * @param file The file object
   * @param data The data
   * @return A boolean indicating success
   */
  static bool writeDataFile(KSaveFile& file, const QByteArray& data);
};

} // end namespace
#endif
