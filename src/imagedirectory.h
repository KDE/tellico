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

#include <QString>

class KTempDir;

namespace Tellico {
  namespace Data {
    class Image;
  }

class ImageDirectory {
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

} // end namespace
#endif
