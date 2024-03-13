/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FILEHANDLER_H
#define TELLICO_FILEHANDLER_H

#include <QString>
#include <QByteArray>

class QUrl;

namespace KIO {
  class Job;
}

class QDomDocument;
class QIODevice;
class QSaveFile;
class QTextStream;

namespace Tellico {
  class ImageFactory;
  class ImageDirectory;
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
friend class ImageDirectory;

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
    explicit FileRef(const QUrl& url, bool quiet=false);
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
  static FileRef* fileRef(const QUrl& url, bool quiet=false);
  /**
   * Read contents of a file into a string.
   *
   * @param url The URL of the file
   * @param quiet whether the importer should report errors or not
   * @param useUTF8 Whether the file should be read as UTF8 or use user locale
   * @return A string containing the contents of a file
   */
  static QString readTextFile(const QUrl& url, bool quiet=false, bool useUTF8=false);
  /**
   * Read contents of an XML file into a string, checking for encoding.
   *
   * @param url The URL of the file
   * @param quiet whether the importer should report errors or not
   * @return A string containing the contents of a file
   */
  static QString readXMLFile(const QUrl& url, bool quiet=false);
  /**
   * Read contents of an XML file into a QDomDocument.
   *
   * @param url The URL of the file
   * @param processNamespace Whether to process the namespace of the XML file
   * @param quiet Whether error messages should be shown
   * @return A QDomDocument containing the contents of a file
   */
  static QDomDocument readXMLDocument(const QUrl& url, bool processNamespace, bool quiet=false);
  /**
   * Read contents of a data file into a QByteArray.
   *
   * @param url The URL of the file
   * @param quiet Whether error messages should be shown
   * @return A QByteArray of the file's contents
   */
  static QByteArray readDataFile(const QUrl& url, bool quiet=false);
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
  static bool writeTextURL(const QUrl& url, const QString& text, bool encodeUTF8, bool force=false, bool quiet=false);
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
  static bool writeDataURL(const QUrl& url, const QByteArray& data, bool force=false, bool quiet=false);
  /**
   * Checks to see if a URL exists already, and if so, queries the user.
   *
   * @param url The target URL
   * @return True if it is ok to continue, false otherwise.
   */
  static bool queryExists(const QUrl& url);
  /**
   * Write a backup file with '~' extension
   *
   * Returns true on success
   */
  static bool writeBackupFile(const QUrl& url);

private:
  /**
   * Writes the contents of a string to a file.
   *
   * @param file The file object
   * @param text The string
   * @param encodeUTF8 Whether to use UTF-8 encoding, or Locale
   * @return A boolean indicating success
   */
  static bool writeTextFile(QSaveFile& file, const QString& text, bool encodeUTF8);
  static void writeTextStream(QTextStream& ts, const QString& text, bool encodeUTF8);
  /**
   * Writes data to a file.
   *
   * @param file The file object
   * @param data The data
   * @return A boolean indicating success
   */
  static bool writeDataFile(QSaveFile& file, const QByteArray& data);
};

} // end namespace
#endif
