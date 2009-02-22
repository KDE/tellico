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

#include "imagedirectory.h"
#include "image.h"
#include "filehandler.h"
#include "tellico_debug.h"

#include <kurl.h>
#include <ktempdir.h>

#include <QFile>
#include <QDir>

using Tellico::ImageDirectory;
using Tellico::TemporaryImageDirectory;

ImageDirectory::ImageDirectory() : m_pathExists(false) {
}

ImageDirectory::ImageDirectory(const QString& path_) {
  setPath(path_);
}

ImageDirectory::~ImageDirectory() {
}

const QString& ImageDirectory::path() const {
  return m_path;
}

void ImageDirectory::setPath(const QString& path_) {
  m_path = path_;
  QDir dir(m_path);
  m_pathExists = dir.exists();
}

bool ImageDirectory::hasImage(const QString& id_) const {
  return QFile::exists(m_path + id_);
}

Tellico::Data::Image* ImageDirectory::imageById(const QString& id_) {
  if(!hasImage(id_)) {
    return 0;
  }

  KUrl imgUrl;
  imgUrl.setPath(m_path + id_);
  Data::Image* img = FileHandler::readImageFile(imgUrl, true /* quiet */);
  if(!img) {
    myLog() << "image not found:" << imgUrl;
    return 0;
  }
  if(img->isNull()) {
    delete img;
    return 0;
  }
  // the image id gets calculated from the image has
  // but we want to reset it to the id that was asked for
  img->setID(id_);
  return img;
}

bool ImageDirectory::writeImage(const Data::Image& img_) {
  if(!m_pathExists) {
    if(m_path.isEmpty()) {
      myWarning() << "writing to empty path!";
    }
    QDir dir(m_path);
    if(!dir.exists() && !dir.mkdir(m_path)) {
      myWarning() << "unable to creatd dir:" << m_path;
    }
  }
  KUrl target;
  target.setPath(m_path);
  target.setFileName(img_.id());
  return FileHandler::writeDataURL(target, img_.byteArray(), true /* force */);
}

bool ImageDirectory::removeImage(const QString& id_) {
  return QFile::remove(m_path + id_);
}

TemporaryImageDirectory::TemporaryImageDirectory() : ImageDirectory() {
  m_dir = new KTempDir(); // default is to auto-delete, aka autoRemove()
  setPath(m_dir->name());
}

TemporaryImageDirectory::~TemporaryImageDirectory() {
  delete m_dir;
  m_dir = 0;
}

void TemporaryImageDirectory::purge() {
  delete m_dir;
  m_dir = new KTempDir();
  setPath(m_dir->name());
}

void TemporaryImageDirectory::setPath(const QString& path_) {
  ImageDirectory::setPath(path_);
}