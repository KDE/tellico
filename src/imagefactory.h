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

class KURL;

#include "image.h"

#include <qdict.h>

namespace Bookcase {

/**
 * @author Robby Stephenson
 * @version $Id: imagefactory.h 386 2004-01-24 05:12:28Z robby $
 */
class ImageFactory {
public:
  static const Data::Image& addImage(const KURL& url);
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
   * Writes an image to a tempfile. ImageFactory keeps track of which images were already written.
   *
   * @param id The ID of the image to be written
   * @param force Force the image to be written, even if it already has been
   * @return Whether the save was successful
   */
  static bool writeImage(const QString& id, bool force=false);
  /**
   * Clean the temp dir and remove all temporary image files
   */
  static void clean();

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
