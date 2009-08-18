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

#ifndef TELLICO_IMAGEFACTORY_H
#define TELLICO_IMAGEFACTORY_H

#include "../utils/stringset.h"

#include <kurl.h>

#include <QObject>
#include <QColor>
#include <QHash>
#include <QCache>
#include <QPixmap>

class KTempDir;
class KZip;

namespace Tellico {
  namespace Data {
    class Image;
    class ImageInfo;
  }

class StyleOptions {
public:
  QString fontFamily;
  int fontSize;
  QColor baseColor;
  QColor textColor;
  QColor highlightedBaseColor;
  QColor highlightedTextColor;
  QString imgDir;
};

/**
 * @author Robby Stephenson
 */
class ImageFactory : public QObject {
Q_OBJECT

public:
  enum CacheDir {
    TempDir,
    DataDir,
    LocalDir,
    ZipArchive
  };

  /**
   * setup some of the static members
   */
  static void init();

  /**
   * Returns the temporary directory where image files are saved
   *
   * @return The full path
   */
  static QString tempDir();
  static QString dataDir();
  static QString localDir();
  static QString imageDir();
  static CacheDir cacheDir();

  /**
   * Add an image, reading it from a URL, which is the case when adding a new image from the
   * @ref ImageWidget.
   *
   * @param url The URL of the image, anything KIO can handle
   * @param quiet If any error should not be reported.
   * @return The image id, empty if null
   */
  static QString addImage(const KUrl& url, bool quiet=false,
                          const KUrl& referrer = KUrl(), bool linkOnly=false);
  /**
   * Add an image, reading it from a regular QImage, which is the case when dragging and dropping
   * an image in the @ref ImageWidget. The format has to be included, since the QImage doesn't
   * 'know' what format it came from.
   *
   * @param image The qimage
   * @param format The image format, probably "PNG"
   * @return The image id, empty if null
   */
  static QString addImage(const QImage& image, const QString& format);
  static QString addImage(const QPixmap& image, const QString& format);
  /**
   * Add an image, reading it from data, which is the case when reading from the data file. The
   * @p id isn't strictly needed, since it can be reconstructed from the image data and format, but
   * since it's already known, go ahead and use it.
   *
   * @param data The image data
   * @param format The image format, from Qt's output format list
   * @param id The internal id of the image
   * @return The image id, empty if null
   */
  static QString addImage(const QByteArray& data, const QString& format, const QString& id);

  static bool writeCachedImage(const QString& id, CacheDir dir, bool force = false);

  /**
   * Returns an image reference given its id. If none is found, a null image
   * is returned.
   *
   * @param id The image id
   * @return The image referencenter
   */
  static const Data::Image& imageById(const QString& id);
  static Data::ImageInfo imageInfo(const QString& id);
  static void cacheImageInfo(const Data::ImageInfo& info);
  // basically returns !imageById().isNull()
  static bool validImage(const QString& id);

  static QPixmap pixmap(const QString& id, int w, int h);

  /**
   * Clear the image cache and dict
   * if deleteTempDirectory = true, then clean the temp dir and remove all temporary image files
   */
  static void clean(bool deleteTempDirectory);
  /**
   * Creates the gradient images used in the entry view.
   */
  static void createStyleImages(int collectionType, const StyleOptions& options = StyleOptions());

  static void removeImage(const QString& id_, bool deleteImage);
  static StringSet imagesNotInCache();

  static void setLocalDirectory(const KUrl& url);
  static void setZipArchive(KZip* zip);

  static ImageFactory* self();

signals:
  void imageLocationMismatch();

private:
  /**
   * Add an image, reading it from a URL, which is the case when adding a new image from the
   * @ref ImageWidget.
   *
   * @param url The URL of the image, anything KIO can handle
   * @param quiet If any error should not be reported.
   * @return The image
   */
  const Data::Image& addImageImpl(const KUrl& url, bool quiet=false,
                                  const KUrl& referrer = KUrl(), bool linkOnly = false);
  /**
   * Add an image, reading it from a regular QImage, which is the case when dragging and dropping
   * an image in the @ref ImageWidget. The format has to be included, since the QImage doesn't
   * 'know' what format it came from.
   *
   * @param image The qimage
   * @param format The image format, probably "PNG"
   * @return The image
   */
  const Data::Image& addImageImpl(const QImage& image, const QString& format);
  /**
   * Add an image, reading it from data, which is the case when reading from the data file. The
   * @p id isn't strictly needed, since it can be reconstructed from the image data and format, but
   * since it's already known, go ahead and use it.
   *
   * @param data The image data
   * @param format The image format, from Qt's output format list
   * @param id The internal id of the image
   * @return The image
   */
  const Data::Image& addImageImpl(const QByteArray& data, const QString& format, const QString& id);

  const Data::Image& addCachedImageImpl(const QString& id, CacheDir dir);

  static ImageFactory* factory;

  static QHash<QString, Data::ImageInfo> s_imageInfoMap;
  static StringSet s_imagesToRelease;

  ImageFactory();
  ~ImageFactory();

  bool hasImage(const QString& id) const;
  void releaseImages();

  class Private;
  Private* const d;
};

} // end namespace

#endif
