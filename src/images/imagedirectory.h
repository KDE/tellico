/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

  virtual bool hasImage(const QString& id) = 0;
  virtual Data::Image* imageById(const QString& id) = 0;
};

class ImageDirectory : public ImageStorage {
public:
  ImageDirectory();
  ImageDirectory(const QString& path);
  virtual ~ImageDirectory();

  virtual QString path();
  virtual void setPath(const QString& path);

  bool hasImage(const QString& id);
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

  virtual QString path();
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

  bool hasImage(const QString& id);
  Data::Image* imageById(const QString& id);

private:
  KZip* m_zip;
  const KArchiveDirectory* m_imgDir;
  StringSet m_images;
};

} // end namespace
#endif
