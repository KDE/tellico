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

#include "imagefactory.h"

#include <kstandarddirs.h>
#include <kapplication.h>
#include <kdebug.h>

#include <qdir.h>

using Bookcase::ImageFactory;

const Bookcase::Data::Image ImageFactory::s_null;

QDict<Bookcase::Data::Image> ImageFactory::s_imageDict;
QDict<int> ImageFactory::s_imageFileDict;
QString ImageFactory::s_tempDir;

const Bookcase::Data::Image& ImageFactory::addImage(const KURL& url_) {
  if(url_.isEmpty() || !url_.isValid()) {
    return s_null;
  }
  Data::Image* img = FileHandler::readImageFile(url_);
  if(!img) {
    return s_null;
  }
  const Data::Image& img2 = imageById(img->id());
  if(!img2.isNull()) {
    delete img;
    return img2;
  }
  if(!img->isNull()) {
    s_imageDict.insert(img->id(), img);
  }
  return *img;
}

const Bookcase::Data::Image& ImageFactory::addImage(const QImage& image_, const QString& format_) {
  Data::Image* img = new Data::Image(image_, format_);
  const Data::Image& img2 = imageById(img->id());
  if(!img2.isNull()) {
    delete img;
    return img2;
  }
  if(!img->isNull()) {
    s_imageDict.insert(img->id(), img);
  }
  return *img;
}

const Bookcase::Data::Image& ImageFactory::addImage(const QByteArray& data_, const QString& format_,
                                                    const QString& id_) {
  const Data::Image& image = imageById(id_);
  if(!image.isNull()) {
    return image;
  }

//  kdDebug() << "ImageFactory::addImage() - " << data_.size()
//            << " bytes, format = " << format_
//            << ", id = "<< id_ << endl;
  Data::Image* img = new Data::Image(data_, format_, id_);
  if(!img->isNull()) {
    s_imageDict.insert(img->id(), img);
  }
  return *img;
}

const Bookcase::Data::Image& ImageFactory::imageById(const QString& id_) {
  if(s_imageDict.isEmpty() || id_.isEmpty()) {
    return s_null;
  }
  Data::Image* img = s_imageDict.find(id_);
  if(img) {
    return *img;
  }
  return s_null;
}

void ImageFactory::clean() {
//  kdDebug() << "ImageFactory::clean()" << endl;

  s_imageDict.setAutoDelete(true);
  s_imageDict.clear();
  s_imageFileDict.clear();

  if(s_tempDir.isEmpty()) {
    return;
  }

  QDir dir(s_tempDir);
  dir.setFilter(QDir::Files | QDir::Writable);
  for(uint i = 0; i < dir.count(); ++i) {
//    kdDebug() << "ImageFactory::clean() - removing " << dir[i] << endl;
    dir.remove(dir[i]);
  }
  dir.rmdir(s_tempDir);
//  kdDebug() << "ImageFactory::clean() - removed " << s_tempDir << ": " << (b?"true":"false") << endl;
  s_tempDir.truncate(0);
}

void ImageFactory::createTempDir() {
  /* From KDE documentation for locateLocal()
       This function is much like locate. However it returns a filename suitable for writing to.
       No check is made if the specified filename actually exists. Missing directories are created.
       If filename is only a directory, without a specific file, filename must have a trailing slash.
  */
  s_tempDir = locateLocal("tmp", QString::fromLatin1("bookcase")
              + kapp->randomString(6)
              + QString::fromLatin1(".tmp")
              + QString::fromLatin1("/"));
//  kdDebug() << "ImageFactory::createTempDir() - created " << s_tempDir << endl;
}

bool ImageFactory::writeImage(const QString& id_, const KURL& targetDir_, bool force_) {
  const Data::Image& img = imageById(id_);
  if(img.isNull()) {
    kdDebug() << "ImageFactory::writeImage() - null image: " << id_ << endl;
    return false;
  }

  bool track = true; // whether to track saving
  KURL target;
  if(targetDir_.isEmpty()) {
    target.setPath(tempDir());
  } else {
    target = targetDir_;
    if(targetDir_.path() != tempDir()) {
      track = false;
    }
  }
  target.addPath(id_);

  bool success = true;
  // three cases when the image should definitely be saved
  // - when forced
  // - when the save dir is not the ImageFactory() temp dir
  // - when the save dir _is_ the ImageFactory() but it has not been written yet
  if(force_ || !track || !s_imageFileDict[id_]) {
//    kdDebug() << "writing " << target.prettyURL() << endl;
    success = FileHandler::writeDataURL(target, img.byteArray(), force_);
    // only keep track for images saved in default tmp dir
    if(success && track) {
      s_imageFileDict.insert(id_, reinterpret_cast<const int *>(1));
    }
  }
  return success;
}
