/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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
#include "tellico_debug.h"
#include "tellico_utils.h"
#include "tellico_kernel.h"

#include <ktempdir.h>
#include <kapplication.h>
#include <kimageeffect.h>

#include <qfile.h>

using Tellico::ImageFactory;

const Tellico::Data::Image ImageFactory::s_null;
bool ImageFactory::s_imagesCreated = false;

QDict<Tellico::Data::Image> ImageFactory::s_imageDict;
// since most images get turned into pixmaps quickly, use 2 megs
// for images and 5 megs for pixmaps
QCache<Tellico::Data::Image> ImageFactory::s_imageCache(2 * 1024 * 1024);
QCache<QPixmap> ImageFactory::s_pixmapCache(5 * 1024 * 1024);
Tellico::StringSet ImageFactory::s_imagesInTmpDir;
KTempDir* ImageFactory::s_tmpDir = 0;

QString ImageFactory::tempDir() {
  if(!s_tmpDir) {
    s_tmpDir = new KTempDir();
    s_tmpDir->setAutoDelete(true);
  }
  return s_tmpDir->name();
}

QString ImageFactory::dataDir() {
  static const QString dataDir = Tellico::saveLocation(QString::fromLatin1("data/"));
  return dataDir;
}

const Tellico::Data::Image& ImageFactory::addImage(const KURL& url_, bool quiet_) {
  if(url_.isEmpty() || !url_.isValid()) {
    return s_null;
  }
//  myLog() << "ImageFactory::addImage() - " << url_.prettyURL() << endl;
  Data::Image* img = FileHandler::readImageFile(url_, quiet_);
  if(!img) {
//    myDebug() << "ImageFactory::addImage() - image not found: " << url_.prettyURL() << endl;
    return s_null;
  }
  if(img->isNull()) {
    delete img;
    return s_null;
  }
  s_imageDict.setAutoDelete(true);
  s_imageDict.remove(img->id());
  s_imageDict.setAutoDelete(false);
  s_imageDict.insert(img->id(), img);
  return *img;
}

const Tellico::Data::Image& ImageFactory::addImage(const QImage& image_, const QString& format_) {
  Data::Image* img = new Data::Image(image_, format_);
  const Data::Image& img2 = imageById(img->id());
  if(!img2.isNull()) {
    delete img;
    return img2;
  }
  if(img->isNull()) {
    delete img;
    return s_null;
  }
  s_imageDict.insert(img->id(), img);
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

//  myDebug() << "ImageFactory::addImage() - " << data_.size()
//            << " bytes, format = " << format_
//            << ", id = "<< id_ << endl;
  img = new Data::Image(data_, format_, id_);
  if(img->isNull()) {
    delete img;
    return s_null;
  }
  s_imageDict.insert(img->id(), img);
  return *img;
}

const Tellico::Data::Image& ImageFactory::addCachedImage(const QString& id_, CacheDir dir_) {
//  myLog() << "ImageFactory::addCachedImage() - id = " << id_ << endl;
  KURL u;
  if(dir_ == DataDir) {
    u.setPath(dataDir() + id_);
  } else { // Temp
    u.setPath(tempDir() + id_);
  }

  QString newID = addImage(u, true).id();
  if(newID.isEmpty()) {
    myDebug() << "ImageFactory::addCachedImage() - null image loaded" << endl;
    return s_null;
  }

  // the id probably got changed, so reset it
  // addImage() already inserted it in the dict
  Data::Image* img = s_imageDict.take(newID);
  if(!img) {
    myDebug() << "ImageFactory::addCachedImage() - no image in dict - very bad!" << endl;
    return s_null;
  }
  if(img->isNull()) {
    myDebug() << "ImageFactory::addCachedImage() - null image in dict, should never happen!" << endl;
    delete img;
    return s_null;
  }
  img->m_id = id_;
  if(!s_imageCache.insert(img->id(), img, img->numBytes())) {
    // can't hold it in the cache
    kdWarning() << "Tellico's image cache is unable to hold the image, it might be too big!" << endl;
    kdWarning() << "Image name is " << img->id() << endl;
    kdWarning() << "Image size is " << img->numBytes() << endl;
    kdWarning() << "Current cache size is " << s_imageCache.totalCost() << endl;
    kdWarning() << "Max cache size is " << s_imageCache.maxCost() << endl;
    s_imageDict.insert(img->id(), img);
  }
  return *img;
}

bool ImageFactory::copyImage(const QString& id_, const KURL& targetDir_, bool force_) {
//  myLog() << "ImageFactory::copyImage() - target = " << targetDir_.url() << id_ << endl;
  if(targetDir_.isEmpty()) {
    myDebug() << "ImageFactory::copyImage() - empty target dir!" << endl;
    return false;
  }

  const Data::Image& img = imageById(id_);
  if(img.isNull()) {
    myDebug() << "ImageFactory::copyImage() - null image: " << id_ << endl;
    return false;
  }

  KURL target = targetDir_;
  target.addPath(id_);

  return FileHandler::writeDataURL(target, img.byteArray(), force_);
}

bool ImageFactory::writeImage(const QString& id_, CacheDir dir_) {
  if(id_.isEmpty()) {
    return false;
  }
//  myLog() << "ImageFactory::writeImage() - id = " << id_ << endl;

//  myDebug() << "ImageFactory::writeCachedImage() - " << id_ << endl;

  QString path = ( dir_ == DataDir ? dataDir() : tempDir() );

  // images in the temp directory are erased every session, so we can track
  // whether they've already been written with a simple string set.
  // images in the data directory are persistent, so we have to check the
  // actual file existence
  bool exists = ( dir_ == DataDir ? QFile::exists(path + id_) : s_imagesInTmpDir.has(id_) );

  // only write if it doesn't exists
  bool success = exists || copyImage(id_, path, true /* force */);

  if(success) {
    if(dir_ == TempDir) {
      s_imagesInTmpDir.add(id_);
    }

    // remove from dict and add to cache
    // it might not be in dict though
    Data::Image* img = s_imageDict.take(id_);
    if(img && !s_imageCache.insert(img->id(), img, img->numBytes())) {
      myDebug() << "ImageFactory::writeImage() - failed writing image to cache: " << id_ << endl;
      // can't insert it in the cache, so put it back in the dict
      // No, it's written to disk now, so we're safe
//      s_imageDict.insert(img->id(), img);
    }
  }
  return success;
}

const Tellico::Data::Image& ImageFactory::imageById(const QString& id_) {
  if(id_.isEmpty()) {
    myDebug() << "ImageFactory::imageById() - empty id" << endl;
    return s_null;
  }

 // first check the cache, used for images that are in the data file, or are only temporary
 // then the dict, used for images downloaded, but not yet saved anywhere
  Data::Image* img = s_imageCache.find(id_);
  if(img) {
    return *img;
  }

  img = s_imageDict.find(id_);
  if(img) {
    return *img;
  }

  // the document does a delayed loading of the images, sometimes
  // so an image could be in the tmp dir and not be in the cache
  if(s_imagesInTmpDir.has(id_)) {
    const Data::Image& img2 = addCachedImage(id_, TempDir);
    if(!img2.isNull()) {
      return img2;
    }
  }

  // don't check Kernel::self()->writeImagesInFile(), someday we might have problems
  // and the image will exist in the data dir, but the app thinks everything should
  // be in the zip file instead
  bool exists = QFile::exists(dataDir() + id_);
  if(exists) {
    // if we're loading from the data dir, but images are not being saved in the
    // data file, then consider the document to be modified since it needs the image saved
    if(!Kernel::self()->writeImagesInFile()) {
      Data::Document::self()->slotSetModified(true);
    }
    const Data::Image& img2 = addCachedImage(id_, DataDir);
    if(!img2.isNull()) {
      return img2;
    }
  }

  // try to do a delayed loading of the image
  if(Data::Document::self()->loadImage(id_)) {
    // loadImage() could insert in either the cache or the dict!
    img = s_imageCache.find(id_);
    if(!img) {
      img = s_imageDict.find(id_);
    }
    if(img) {
      // go ahead and write image to disk so we don't have to keep it in memory
      writeImage(id_, TempDir);
      return *img;
    }
  }
  myDebug() << "ImageFactory::imageById() - not found: " << id_ << endl;
  return s_null;
}

QPixmap ImageFactory::pixmap(const QString& id_, int width_, int height_) {
  if(id_.isEmpty()) {
    return QPixmap();
  }

//  myDebug() << "ImageFactory::pixmap() - " <<id_ << endl;
  QString key = id_ + '|' + QString::number(width_) + '|' + QString::number(height_);
  QPixmap* pix = s_pixmapCache.find(key);
  if(pix) {
    return *pix;
  }

  const Data::Image& img = imageById(id_);
  if(img.isNull()) {
    return QPixmap();
  }

  if(width_ > 0 && height_ > 0) {
    pix = new QPixmap(img.convertToPixmap(width_, height_));
  } else {
    pix = new QPixmap(img.convertToPixmap());
  }

  // pixmap size is w x h x d, divided by 8 bits
  if(!s_pixmapCache.insert(key, pix, pix->width()*pix->height()*pix->depth()/8)) {
    myDebug() << "ImageFactory::pixmap() - can't save in cache: " << id_ << endl;
    QPixmap pix2(*pix);
    delete pix;
    return pix2;
  }
  return *pix;
}

void ImageFactory::clean() {
  LOG_FUNC;

  s_imageDict.setAutoDelete(true);
  s_imageDict.clear();
  s_imageCache.setAutoDelete(true);
  s_imageCache.clear();
  s_pixmapCache.setAutoDelete(true);
  s_pixmapCache.clear();
  s_imagesInTmpDir.clear();

  delete s_tmpDir;
  s_tmpDir = 0;
  return;
}

void ImageFactory::createStyleImages() {
  if(s_imagesCreated) {
    return;
  }

  const QColorGroup& cg = kapp->palette().active();

  QColor bgc1 = Tellico::blendColors(cg.base(), cg.highlight(), 30);
  QColor bgc2 = Tellico::blendColors(cg.base(), cg.highlight(), 50);

  QString bgname = QString::fromLatin1("gradient_bg.png");
  QImage bgImage = KImageEffect::gradient(QSize(400, 1), bgc1, cg.base(),
                                          KImageEffect::PipeCrossGradient);
  bgImage = KImageEffect::rotate(bgImage, KImageEffect::Rotate90);
  ImageFactory::addImage(Data::Image::byteArray(bgImage, "PNG"), QString::fromLatin1("PNG"), bgname);
  ImageFactory::writeImage(bgname,  ImageFactory::TempDir);

  QString hdrname = QString::fromLatin1("gradient_header.png");
  QImage hdrImage = KImageEffect::unbalancedGradient(QSize(1, 10), cg.highlight(), bgc2,
                                                     KImageEffect::VerticalGradient, 100, -100);
  ImageFactory::addImage(Data::Image::byteArray(hdrImage, "PNG"), QString::fromLatin1("PNG"), hdrname);
  ImageFactory::writeImage(hdrname, ImageFactory::TempDir);

  s_imagesCreated = true;
}
