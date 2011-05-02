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

#include "imagedirectory.h"
#include "image.h"
#include "filehandler.h"
#include "../tellico_debug.h"

#include <kurl.h>
#include <ktempdir.h>
#include <kzip.h>

#include <QFile>
#include <QDir>

using namespace Tellico;
using Tellico::ImageStorage;
using Tellico::ImageDirectory;
using Tellico::TemporaryImageDirectory;
using Tellico::ImageZipArchive;

ImageDirectory::ImageDirectory() : ImageStorage(), m_pathExists(false) {
}

ImageDirectory::ImageDirectory(const QString& path_) : ImageStorage() {
  setPath(path_);
}

ImageDirectory::~ImageDirectory() {
}

QString ImageDirectory::path() {
  return m_path;
}

void ImageDirectory::setPath(const QString& path_) {
  m_path = path_;
  QDir dir(m_path);
  m_pathExists = dir.exists();
}

bool ImageDirectory::hasImage(const QString& id_) {
  return QFile::exists(path() + id_);
}

Tellico::Data::Image* ImageDirectory::imageById(const QString& id_) {
  if(!hasImage(id_)) {
    return 0;
  }

  KUrl imgUrl;
  imgUrl.setPath(path() + id_);
  Data::Image* img = FileHandler::readImageFile(imgUrl, id_, true /* quiet */);
  if(!img) {
    myLog() << "image not found:" << imgUrl;
    return 0;
  }
  if(img->isNull()) {
    myLog() << "image found but null:" << imgUrl;
    delete img;
    return 0;
  }
  return img;
}

bool ImageDirectory::writeImage(const Data::Image& img_) {
  const QString path = this->path(); // virtual function, so don't assume m_path is correct
  if(!m_pathExists) {
    if(path.isEmpty()) {
      myWarning() << "trying to write to empty path:" << img_.id();
      return false;
    }
    QDir dir(path);
    if(dir.mkdir(path)) {
      myLog() << "created" << path;
    } else {
      myWarning() << "unable to create dir:" << path;
    }
    m_pathExists = true;
  }
  KUrl target;
  target.setPath(path);
  target.setFileName(img_.id());
  return FileHandler::writeDataURL(target, img_.byteArray(), true /* force */);
}

bool ImageDirectory::removeImage(const QString& id_) {
  return QFile::remove(path() + id_);
}

TemporaryImageDirectory::TemporaryImageDirectory() : ImageDirectory(), m_dir(0) {
}

TemporaryImageDirectory::~TemporaryImageDirectory() {
  purge();
}

void TemporaryImageDirectory::purge() {
  delete m_dir;
  m_dir = 0;
}

QString TemporaryImageDirectory::path() {
  if(!m_dir) {
    m_dir = new KTempDir(); // default is to auto-delete, aka autoRemove()
    ImageDirectory::setPath(m_dir->name());
  }
  return ImageDirectory::path();
}

void TemporaryImageDirectory::setPath(const QString& path) {
  Q_UNUSED(path);
  Q_ASSERT(path.isEmpty()); // should never be called, that's why it's private
}

ImageZipArchive::ImageZipArchive() : ImageStorage(), m_zip(0) {
}

ImageZipArchive::~ImageZipArchive() {
  delete m_zip;
  m_zip = 0;
}

void ImageZipArchive::setZip(KZip* zip_) {
  m_images.clear();
  delete m_zip;
  m_zip = zip_;
  m_imgDir = 0;

  const KArchiveDirectory* dir = m_zip->directory();
  if(!dir) {
    delete m_zip;
    m_zip = 0;
    return;
  }
  const KArchiveEntry* imgDirEntry = dir->entry(QLatin1String("images"));
  if(!imgDirEntry || !imgDirEntry->isDirectory()) {
    delete m_zip;
    m_zip = 0;
    return;
  }
  m_imgDir = static_cast<const KArchiveDirectory*>(imgDirEntry);
  m_images.add(m_imgDir->entries());
}

bool ImageZipArchive::hasImage(const QString& id_) {
  return m_images.has(id_);
}

Tellico::Data::Image* ImageZipArchive::imageById(const QString& id_) {
  if(!hasImage(id_)) {
    return 0;
  }
  Data::Image* img = 0;
  const KArchiveEntry* file = m_imgDir->entry(id_);
  if(file && file->isFile()) {
    img = new Data::Image(static_cast<const KArchiveFile*>(file)->data(),
                          id_.section(QLatin1Char('.'), -1).toUpper(), id_);
  }
  // might be unexpected behavior, but in order to delete the zip object after
  // all images are read, we need to consider the image gone now
  m_images.remove(id_);
  if(m_images.isEmpty()) {
    delete m_zip;
    m_zip = 0;
    m_imgDir = 0;
  }
  if(!img) {
    myLog() << "image not found:" << id_;
    return 0;
  }
  if(img->isNull()) {
    myLog() << "image found but null:" << id_;
    delete img;
    return 0;
  }
  return img;
}
