/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
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
#include "document.h"

#include <kstandarddirs.h>
#include <kapplication.h>
#include <kdebug.h>

#include <qdir.h>

using Tellico::ImageFactory;

const Tellico::Data::Image ImageFactory::s_null;

QDict<Tellico::Data::Image> ImageFactory::s_imageDict;
Tellico::StringSet ImageFactory::s_cachedImages;
QString ImageFactory::s_tempDir;

const Tellico::Data::Image& ImageFactory::addImage(const KURL& url_, bool quiet_) {
  if(url_.isEmpty() || !url_.isValid()) {
    return s_null;
  }
//  kdDebug() << "ImageFactory::addImage() - " << url_.prettyURL() << endl;
  Data::Image* img = FileHandler::readImageFile(url_, quiet_);
  if(!img) {
    return s_null;
  }
  if(!img->isNull()) {
    s_imageDict.setAutoDelete(true);
    s_imageDict.remove(img->id());
    s_imageDict.setAutoDelete(false);
    s_imageDict.insert(img->id(), img);
  }
  return *img;
}

const Tellico::Data::Image& ImageFactory::addImage(const QImage& image_, const QString& format_) {
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

const Tellico::Data::Image& ImageFactory::addImage(const QByteArray& data_, const QString& format_,
                                                   const QString& id_) {
  if(id_.isEmpty()) {
    return s_null;
  }

  Data::Image* img = s_imageDict.find(id_);
  if(img) {
//    myDebug() << "ImageFactory::addImage() - image already exists in dict" << endl;
    return *img;
  }

//  kdDebug() << "ImageFactory::addImage() - " << data_.size()
//            << " bytes, format = " << format_
//            << ", id = "<< id_ << endl;
  img = new Data::Image(data_, format_, id_);
  if(!img->isNull()) {
    s_imageDict.insert(img->id(), img);
  }
  return *img;
}

const Tellico::Data::Image& ImageFactory::imageById(const QString& id_) {
  if(id_.isEmpty()) {
    return s_null;
  }
  Data::Image* img = s_imageDict.find(id_);
  if(img) {
    return *img;
  }
  // try to do a delayed loading of the image
  if(Data::Document::self()->loadImage(id_)) {
    img = s_imageDict.find(id_);
    if(img) {
      return *img;
    }
  }
  return s_null;
}

void ImageFactory::clean() {
//  kdDebug() << "ImageFactory::clean()" << endl;

  s_imageDict.setAutoDelete(true);
  s_imageDict.clear();
  s_cachedImages.clear();

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
  s_tempDir = locateLocal("tmp", QString::fromLatin1("tellico")
              + kapp->randomString(6)
              + QString::fromLatin1(".tmp")
              + QString::fromLatin1("/"));
//  kdDebug() << "ImageFactory::createTempDir() - created " << s_tempDir << endl;
}

bool ImageFactory::writeImage(const QString& id_, const KURL& targetDir_, bool force_) {
  const Data::Image& img = imageById(id_);
  if(img.isNull()) {
//    kdDebug() << "ImageFactory::writeImage() - null image: " << id_ << endl;
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
  if(force_ || !track || !s_cachedImages.has(id_)) {
//    kdDebug() << "writing " << target.prettyURL() << endl;
    success = FileHandler::writeDataURL(target, img.byteArray(), force_);
    // only keep track for images saved in default tmp dir
    if(success && track) {
      s_cachedImages.add(id_);
    }
  }
  return success;
}
