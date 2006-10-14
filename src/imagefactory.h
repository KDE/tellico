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

#ifndef IMAGEFACTORY_H
#define IMAGEFACTORY_H

#include "filehandler.h"
#include "stringset.h"

#include <kurl.h>

#include <qcolor.h>
#include <qcache.h>

class KTempDir;

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
class ImageFactory {
public:
  enum CacheDir {
    TempDir=0, /* used as 0 in Document::slotWriteAllImages() */
    DataDir
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

  /**
   * Add an image, reading it from a URL, which is the case when adding a new image from the
   * @ref ImageWidget.
   *
   * @param url The URL of the image, anything KIO can handle
   * @param quiet If any error should not be reported.
   * @return The image id, empty if null
   */
  static QString addImage(const KURL& url, bool quiet=false, const KURL& referrer = KURL());
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

  /**
   * Writes an image to a file. ImageFactory keeps track of which images were already written
   * if the location is the same as the tempdir.
   *
   * @param id The ID of the image to be written
   * @param targetDir The directory to write the image to, if empty, the tempdir is used.
   * @param force Force the image to be written, even if it already has been
   * @return Whether the save was successful
   */
  static bool writeImage(const QString& id, const KURL& targetDir, bool force=false);
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
  static void createStyleImages(const StyleOptions& options = StyleOptions());

  static void removeImage(const QString& id_, bool deleteImage);
  static StringSet imagesNotInCache();

private:
  /**
   * Add an image, reading it from a URL, which is the case when adding a new image from the
   * @ref ImageWidget.
   *
   * @param url The URL of the image, anything KIO can handle
   * @param quiet If any error should not be reported.
   * @return The image
   */
  static const Data::Image& addImageImpl(const KURL& url, bool quiet=false, const KURL& referrer = KURL());
  /**
   * Add an image, reading it from a regular QImage, which is the case when dragging and dropping
   * an image in the @ref ImageWidget. The format has to be included, since the QImage doesn't
   * 'know' what format it came from.
   *
   * @param image The qimage
   * @param format The image format, probably "PNG"
   * @return The image
   */
  static const Data::Image& addImageImpl(const QImage& image, const QString& format);
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
  static const Data::Image& addImageImpl(const QByteArray& data, const QString& format, const QString& id);

  static const Data::Image& addCachedImageImpl(const QString& id, CacheDir dir);
  static bool hasImage(const QString& id);

  static bool s_needInit;
  static QDict<Data::Image> s_imageDict;
  static QCache<Data::Image> s_imageCache;
  static QCache<QPixmap> s_pixmapCache;
  static QMap<QString, Data::ImageInfo> s_imageInfoMap;
  static StringSet s_imagesInTmpDir; // the id's of the images written to tmp directory
  static KTempDir* s_tmpDir;
  static const Data::Image s_null;
};

} // end namespace

#endif
