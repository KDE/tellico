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

#include <QUrl>
#include <QObject>
#include <QColor>
#include <QHash>
#include <QPixmap>

#include <memory>

class KZip;
class KJob;

namespace Tellico {
  namespace Data {
    class Image;
    class ImageInfo;
  }
  class ImageDirectory;

class StyleOptions {
public:
  QString fontFamily;
  int fontSize;
  QColor baseColor;
  QColor textColor;
  QColor highlightedBaseColor;
  QColor highlightedTextColor;
  QColor linkColor;
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
  static QUrl tempDir();
  static QUrl dataDir();
  static QUrl localDir();
  static QUrl imageDir();
  static CacheDir cacheDir();

  /**
   * Add an image, reading it from a URL, which is the case when adding a new image from the
   * @ref ImageWidget.
   *
   * @param url The URL of the image, anything KIO can handle
   * @param quiet If any error should not be reported.
   * @return The image id, empty if null
   */
  static QString addImage(const QUrl& url, bool quiet=false,
                          const QUrl& referrer = QUrl(), bool linkOnly=false);
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
   * Add an image, reading it from data, which is the case when reading from the data file, and
   * using the @p format and @p id as the image id. The image id is checked in the image cache
   * image dict first, and then if it is not found, a new Image is constructed. The new image is
   * inserted in the dict, and the image info is cached.
   *
   * @param data The image data
   * @param format The image format, from Qt's output format list
   * @param id The internal id of the image
   * @return The image id, empty if null
   */
  static QString addImage(const QByteArray& data, const QString& format, const QString& id=QString());

  static bool writeCachedImage(const QString& id, CacheDir dir, bool force = false);
  static bool writeCachedImage(const QString& id, ImageDirectory* dir, bool force = false);

  /**
   * Returns an image reference given its id. If none is found, a null image
   * is returned.
   *
   * @param id The image id
   * @return The image reference
   */
  static const Data::Image& imageById(const QString& id);
  static bool hasImageInDir(const QString& id);
  static bool hasImageInDirOrMemory(const QString& id);
  bool hasImageInMemory(const QString& id) const;
  // just used for testing
  bool hasNullImage(const QString& id) const;
  /**
   * Requests an image to be made available. Images already in the cache or available locally are
   * considered to be instantly available. Otherwise, the id is assumed to be a URL and is downloaded
   * The imageAvailable() signal is used to indicate completion and availability of the image.
   *
   * @param id The image id
   */
  static void requestImageById(const QString& id);
  static Data::ImageInfo imageInfo(const QString& id);
  static void cacheImageInfo(const Data::ImageInfo& info);
  static bool hasImageInfo(const QString& id);
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

  static QUrl localDirectory(const QUrl& url);
  static void setLocalDirectory(const QUrl& url);
  static void setZipArchive(std::unique_ptr<KZip> zip);

  static ImageFactory* self();

Q_SIGNALS:
  void imageAvailable(const QString& id);
  void imageLocationMismatch();

private Q_SLOTS:
  void slotImageJobResult(KJob* job);
  void releaseImages();

private:
  /**
   * Add an image, reading it from a URL, which is the case when adding a new image from the
   * @ref ImageWidget.
   *
   * @param url The URL of the image, anything KIO can handle
   * @param quiet If any error should not be reported.
   * @return The image
   */
  const Data::Image& addImageImpl(const QUrl& url, bool quiet=false,
                                  const QUrl& referrer = QUrl(), bool linkOnly = false);
  void requestImageByUrlImpl(const QUrl& url, bool quiet=false,
                             const QUrl& referrer = QUrl(), bool linkOnly = false);
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

  void emitImageMismatch();

  class Private;
  Private* const d;
};

} // end namespace

#endif
