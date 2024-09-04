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

#include <QUrl>

#include <memory>

class QTemporaryDir;

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

private:
  Q_DISABLE_COPY(ImageStorage)
};

class ImageDirectory : public ImageStorage {
public:
  ImageDirectory();
  ImageDirectory(const QUrl& dir);
  virtual ~ImageDirectory();

  virtual QUrl dir();
  virtual void setDirectory(const QUrl& dir);

  bool hasImage(const QString& id) Q_DECL_OVERRIDE;
  Data::Image* imageById(const QString& id) Q_DECL_OVERRIDE;
  bool writeImage(const Data::Image& image);
  bool removeImage(const QString& id);

private:
  Q_DISABLE_COPY(ImageDirectory)
  QUrl m_dir;
  bool m_pathExists;
  bool m_isLocal;
  QHash<QString, bool> m_imageExists;
  // until the file gets saved, the local directory is temporary
  QTemporaryDir* m_tempDir;
};

class TemporaryImageDirectory : public ImageDirectory {
public:
  TemporaryImageDirectory();
  virtual ~TemporaryImageDirectory();

  virtual QUrl dir() Q_DECL_OVERRIDE;
  void purge();

private:
  Q_DISABLE_COPY(TemporaryImageDirectory)
  void setDirectory(const QUrl& dir) Q_DECL_OVERRIDE;

  QTemporaryDir* m_tempDir;
};

class ImageZipArchive : public ImageStorage {
public:
  ImageZipArchive();
  virtual ~ImageZipArchive();

  void setZip(std::unique_ptr<KZip> zip);

  bool hasImage(const QString& id) Q_DECL_OVERRIDE;
  Data::Image* imageById(const QString& id) Q_DECL_OVERRIDE;

private:
  Q_DISABLE_COPY(ImageZipArchive)
  std::unique_ptr<KZip> m_zip;
  const KArchiveDirectory* m_imgDir;
  StringSet m_images;
};

} // end namespace
#endif
