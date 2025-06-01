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
#include "imagejob.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <KZip>
#include <KIO/StatJob>
#include <KIO/MkdirJob>
#include <KIO/DeleteJob>

#include <QFile>
#include <QDir>
#include <QUrl>
#include <QTemporaryDir>

using namespace Tellico;
using Tellico::ImageStorage;
using Tellico::ImageDirectory;
using Tellico::TemporaryImageDirectory;
using Tellico::ImageZipArchive;

ImageDirectory::ImageDirectory() : ImageStorage(), m_pathExists(false), m_isLocal(true), m_tempDir(nullptr) {
}

ImageDirectory::ImageDirectory(const QUrl& dir_) : ImageStorage(), m_tempDir(nullptr) {
  setDirectory(dir_);
}

ImageDirectory::~ImageDirectory() {
  delete m_tempDir;
  m_tempDir = nullptr;
}

QUrl ImageDirectory::dir() {
  return m_dir;
}

void ImageDirectory::setDirectory(const QUrl& dir_) {
  if(dir_ != m_dir) {
    m_dir = dir_;
    m_isLocal = m_dir.isLocalFile();
    // for now, only check existence of local directories
    m_pathExists = m_isLocal && QFileInfo::exists(m_dir.toLocalFile());
  }
}

bool ImageDirectory::hasImage(const QString& id_) {
  // dir() is virtual
  const bool localExists = m_pathExists && m_isLocal && QFile::exists(dir().toLocalFile() + id_);
  if(localExists) return true;

  // now we're looking for a remote file. First, check if we know already
  if(m_imageExists.contains(id_)) {
    return m_imageExists[id_];
  }
  QUrl imageUrl = dir();
  imageUrl.setPath(imageUrl.path() + id_);
  auto statJob = KIO::stat(imageUrl, KIO::StatJob::SourceSide, KIO::StatNoDetails, KIO::HideProgressInfo);
  // if no error, then file exists
  const bool noError = statJob->exec();
  m_imageExists.insert(id_, noError);
  return noError;
}

Tellico::Data::Image* ImageDirectory::imageById(const QString& id_) {
  if(!hasImage(id_)) {
    return nullptr;
  }

  Data::Image* img = nullptr;
  if(m_isLocal) {
    img = new Data::Image(dir().toLocalFile() + id_, id_);
  } else {
    QUrl imageUrl = dir();
    imageUrl.setPath(imageUrl.path() + id_);
    auto imgJob = new ImageJob(imageUrl, id_);
    if(imgJob->exec()) {
      img = new Data::Image(imgJob->image());
      m_pathExists = true;
    } else {
      return nullptr;
    }
  }
  if(img->isNull()) {
    myLog() << "Image found but null:" << id_;
    delete img;
    return nullptr;
  }
  return img;
}

bool ImageDirectory::writeImage(const Data::Image& img_) {
  if(dir().isEmpty()) {
    // an empty directory means the data file itself hasn't been saved yet
    if(!m_tempDir) {
      m_tempDir = new QTemporaryDir(); // default is to auto-delete, aka autoRemove()
    }
    // It's a directory, so be sure to end with a slash
    ImageDirectory::setDirectory(QUrl::fromLocalFile(m_tempDir->path() + QLatin1Char('/')));
    return writeImage(img_);
  }

  QUrl target = dir();
  if(!m_pathExists) {
    if(m_isLocal) {
      const QString localPath = dir().toLocalFile();
      Q_ASSERT(!localPath.isEmpty());
      QDir dir;
      if(dir.mkdir(localPath)) {
        myLog() << "Created local directory:" << localPath;
      } else {
        myWarning() << "Unable to create dir:" << localPath;
      }
      m_pathExists = true;
    } else {
      auto statJob = KIO::stat(target, KIO::StatJob::SourceSide, KIO::StatNoDetails, KIO::HideProgressInfo);
      // if no error, then dir exists
      if(statJob->exec()) {
        m_pathExists = true;
      } else {
        auto mkdirJob = KIO::mkdir(target);
        myLog() << "Creating directory:" << target.toDisplayString();
        m_pathExists = mkdirJob->exec();
      }
    }
  }
  target.setPath(target.path() + img_.id());
  return FileHandler::writeDataURL(target, img_.byteArray(), true /* force */);
}

bool ImageDirectory::removeImage(const QString& id_) {
  if(!m_pathExists) return false;
  if(m_isLocal) {
    return QFile::remove(dir().toLocalFile() + id_);
  }
  QUrl imgUrl(dir());
  imgUrl.setPath(imgUrl.path() + id_);
  auto delJob = KIO::del(imgUrl, KIO::HideProgressInfo);
  if(delJob->exec()) {
    return true;
  } else {
    myDebug() << "Failed to remove" << id_;
    myDebug() << delJob->errorString();
    return false;
  }
}

TemporaryImageDirectory::TemporaryImageDirectory() : ImageDirectory(), m_tempDir(nullptr) {
}

TemporaryImageDirectory::~TemporaryImageDirectory() {
  purge();
}

void TemporaryImageDirectory::purge() {
  delete m_tempDir;
  m_tempDir = nullptr;
}

QUrl TemporaryImageDirectory::dir() {
  if(!m_tempDir) {
    m_tempDir = new QTemporaryDir(); // default is to auto-delete, aka autoRemove()
    // in KDE4, the way this worked included the final slash.
    ImageDirectory::setDirectory(QUrl::fromLocalFile(m_tempDir->path() + QLatin1Char('/')));
    myLog() << "Setting temp image dir:" << dir().url(QUrl::PreferLocalFile);
  }
  return ImageDirectory::dir();
}

ImageZipArchive::ImageZipArchive() : ImageStorage(), m_imgDir(nullptr) {
}

ImageZipArchive::~ImageZipArchive() {
}

void ImageZipArchive::setZip(std::unique_ptr<KZip> zip_) {
  m_images.clear();
  m_zip = std::move(zip_);
  m_imgDir = nullptr;

  const KArchiveDirectory* dir = m_zip->directory();
  if(!dir) {
    m_zip.reset();
    return;
  }
  const KArchiveEntry* imgDirEntry = dir->entry(QStringLiteral("images"));
  if(!imgDirEntry || !imgDirEntry->isDirectory()) {
    m_zip.reset();
    return;
  }
  m_imgDir = static_cast<const KArchiveDirectory*>(imgDirEntry);
  m_images.add(m_imgDir->entries());
}

bool ImageZipArchive::hasImage(const QString& id_) {
  return m_images.contains(id_);
}

Tellico::Data::Image* ImageZipArchive::imageById(const QString& id_) {
  if(!hasImage(id_)) {
    return nullptr;
  }
  Data::Image* img = nullptr;
  const KArchiveEntry* file = m_imgDir->entry(id_);
  if(file && file->isFile()) {
    img = new Data::Image(static_cast<const KArchiveFile*>(file)->data(),
                          id_.section(QLatin1Char('.'), -1).toUpper(), id_);
  }
  // might be unexpected behavior, but in order to delete the zip object after
  // all images are read, we need to consider the image gone now
  m_images.remove(id_);
  if(m_images.isEmpty()) {
    m_zip.reset();
    m_imgDir = nullptr;
  }
  if(!img) {
    myLog() << "image not found:" << id_;
    return nullptr;
  }
  if(img->isNull()) {
    myLog() << "image found but null:" << id_;
    delete img;
    return nullptr;
  }
  return img;
}
