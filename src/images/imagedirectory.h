/***************************************************************************
    copyright            : (C) 2009 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_IMAGEDIRECTORY_H
#define TELLICO_IMAGEDIRECTORY_H

#include "../utils/stringset.h"

#include <QString>

class KTempDir;
class KZip;
class KArchiveDirectory;

namespace Tellico {
  namespace Data {
    class Image;
  }

class ImageStorage {
public:
  ImageStorage() {}
  virtual ~ImageStorage() {}

  virtual bool hasImage(const QString& id) const = 0;
  virtual Data::Image* imageById(const QString& id) = 0;
};

class ImageDirectory : public ImageStorage {
public:
  ImageDirectory();
  ImageDirectory(const QString& path);
  virtual ~ImageDirectory();

  const QString& path() const;
  virtual void setPath(const QString& path);

  bool hasImage(const QString& id) const;
  Data::Image* imageById(const QString& id);
  bool writeImage(const Data::Image& image);
  bool removeImage(const QString& id);

private:
  QString m_path;
  bool m_pathExists;
};

class TemporaryImageDirectory : public ImageDirectory {
public:
  TemporaryImageDirectory();
  virtual ~TemporaryImageDirectory();

  void purge();

private:
  void setPath(const QString& path);

  KTempDir* m_dir;
};

class ImageZipArchive : public ImageStorage {
public:
  ImageZipArchive();
  virtual ~ImageZipArchive();

  void setZip(KZip* zip);

  bool hasImage(const QString& id) const;
  Data::Image* imageById(const QString& id);

private:
  KZip* m_zip;
  const KArchiveDirectory* m_imgDir;
  StringSet m_images;
};

} // end namespace
#endif
