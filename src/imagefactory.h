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

#ifndef IMAGEFACTORY_H
#define IMAGEFACTORY_H

#include "image.h"
#include "filehandler.h"

#include <kurl.h>

#include <qdict.h>

namespace Bookcase {

/**
 * @author Robby Stephenson
 * @version $Id: imagefactory.h 657 2004-05-13 04:52:31Z robby $
 */
class ImageFactory {
public:
  /**
   * Add an image, reading it from a URL, which is the case when adding a new image from the
   * @ref ImageWidget.
   *
   * @param url The URL of the image, anything KIO can handle
   * @return The image
   */
  static const Data::Image& addImage(const KURL& url);
  /**
   * Add an image, reading it from a regular QImage, which is the case when dragging and dropping
   * an image in the @ref ImageWidget. The format has to be included, since the QImage doesn't
   * 'know' what format it came from.
   *
   * @param image The qimage
   * @param format The image format, probably "PNG"
   * @return The image
   */
  static const Data::Image& addImage(const QImage& image, const QString& format);
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
  static const Data::Image& addImage(const QByteArray& data, const QString& format, const QString& id);
  /**
   * Returns an image reference given its id. If none is found, a null image
   * is returned.
   *
   * @param id The image id
   * @return The image referencenter
   */
  static const Data::Image& imageById(const QString& id);
  /**
   * Returns the temporary directory where image files are saved
   *
   * @return The full path
   */
  static const QString& tempDir() { if(s_tempDir.isEmpty()) createTempDir(); return s_tempDir; }
  /**
   * Writes an image to a file. ImageFactory keeps track of which images were already written
   * if the location is the same as the tempdir.
   *
   * @param id The ID of the image to be written
   * @param targetDir The directory to write the image to, if empty, the tempdir is used.
   * @param force Force the image to be written, even if it already has been
   * @return Whether the save was successful
   */
  static bool writeImage(const QString& id, const KURL& targetDir=KURL(), bool force=false);
  /**
   * Clean the temp dir and remove all temporary image files
   */
  static void clean();
  /**
   * Are there any images in the collection?
   */
  static bool hasImages() { return !s_imageDict.isEmpty(); }

private:
  /**
   * Create a temp dir for images to be written to disk.
   */
  static void createTempDir();

  static QDict<Data::Image> s_imageDict;
  // use a dict for fast random access to keep track of which images were written to disk
  static QDict<int> s_imageFileDict;
  static QString s_tempDir;
  static const Data::Image s_null;
};

} // end namespace

#endif
