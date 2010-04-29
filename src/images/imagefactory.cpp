/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include "imagefactory.h"
#include "image.h"
#include "imageinfo.h"
#include "imagedirectory.h"
#include "../core/filehandler.h"
#include "../core/tellico_config.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"

#include <kapplication.h>
#include <kcolorutils.h>
#include <qimageblitz/qimageblitz.h>

#define RELEASE_IMAGES

using Tellico::ImageFactory;

// this image info map is primarily for big images that don't fit
// in the cache, so that don't have to be continually reloaded to get info
QHash<QString, Tellico::Data::ImageInfo> ImageFactory::s_imageInfoMap;
Tellico::StringSet ImageFactory::s_imagesToRelease;

Tellico::ImageFactory* ImageFactory::factory = 0;

class ImageFactory::Private {
public:
  // since most images get turned into pixmaps quickly, use 10 megs
  // for images and 10 megs for pixmaps
  Private() {}

  QHash<QString, Data::Image*> imageDict;
  QCache<QString, Data::Image> imageCache;
  QCache<QString, QPixmap> pixmapCache;
  ImageDirectory dataImageDir; // kept in $KDEHOME/share/apps/tellico/data/
  ImageDirectory localImageDir; // kept local to data file
  TemporaryImageDirectory tempImageDir; // kept in tmp directory
  ImageZipArchive imageZipArchive;
};

ImageFactory::ImageFactory() : QObject(), d(new Private()) {
}

ImageFactory::~ImageFactory() {
  delete d;
}

void ImageFactory::init() {
  if(factory) {
    return;
  }
  factory = new ImageFactory();
  factory->d->imageCache.setMaxCost(Config::imageCacheSize());
  factory->d->pixmapCache.setMaxCost(Config::imageCacheSize());
  factory->d->dataImageDir.setPath(Tellico::saveLocation(QLatin1String("data/")));
}

Tellico::ImageFactory* ImageFactory::self() {
  Q_ASSERT(factory);
  return factory;
}

QString ImageFactory::tempDir() {
  return factory->d->tempImageDir.path();
}

QString ImageFactory::dataDir() {
  return factory->d->dataImageDir.path();
}

QString ImageFactory::localDir() {
  const QString dir = factory->d->localImageDir.path();
  return dir.isEmpty() ? dataDir() : dir;
}

QString ImageFactory::imageDir() {
  switch(cacheDir()) {
    case LocalDir: return localDir();
    case DataDir: return dataDir();
    case ZipArchive: return tempDir();
    case TempDir: return tempDir();
  }
  return tempDir();
}

Tellico::ImageFactory::CacheDir ImageFactory::cacheDir() {
  switch(Config::imageLocation()) {
    case Config::ImagesInLocalDir: return LocalDir;
    case Config::ImagesInAppDir: return DataDir;
    case Config::ImagesInFile: return TempDir;
  }
  return TempDir;
}

QString ImageFactory::addImage(const KUrl& url_, bool quiet_, const KUrl& refer_, bool link_) {
  Q_ASSERT(factory && "ImageFactory is not initialized!");
  return factory->addImageImpl(url_, quiet_, refer_, link_).id();
}

const Tellico::Data::Image& ImageFactory::addImageImpl(const KUrl& url_, bool quiet_, const KUrl& refer_, bool link_) {
  if(url_.isEmpty() || !url_.isValid()) {
    return Data::Image::null;
  }
//  myLog() << url_.prettyUrl();
  Data::Image* img = FileHandler::readImageFile(url_, quiet_, refer_);
  if(!img) {
    myLog() << "image not found:" << url_.prettyUrl();
    return Data::Image::null;
  }
  if(img->isNull()) {
    myLog() << "null image:" << url_.prettyUrl();
    delete img;
    return Data::Image::null;
  }

  if(link_) {
    img->setLinkOnly(true);
    img->setID(url_.url());
  }

  if(hasImage(img->id())) {
    const Data::Image& img2 = imageById(img->id());
    if(!img2.isNull()) {
      delete img;
      return img2;
    }
  }

  if(!link_) {
    d->imageDict.insert(img->id(), img);
  }
  s_imageInfoMap.insert(img->id(), Data::ImageInfo(*img));
  return *img;
}

QString ImageFactory::addImage(const QImage& image_, const QString& format_) {
  Q_ASSERT(factory && "ImageFactory is not initialized!");
  return factory->addImageImpl(image_, format_).id();
}

QString ImageFactory::addImage(const QPixmap& pix_, const QString& format_) {
  Q_ASSERT(factory && "ImageFactory is not initialized!");
  return factory->addImageImpl(pix_.toImage(), format_).id();
}

const Tellico::Data::Image& ImageFactory::addImageImpl(const QImage& image_, const QString& format_) {
  Data::Image* img = new Data::Image(image_, format_);
  if(hasImage(img->id())) {
    const Data::Image& img2 = imageById(img->id());
    if(!img2.isNull()) {
      delete img;
      return img2;
    }
  }
  if(img->isNull()) {
    delete img;
    return Data::Image::null;
  }
  d->imageDict.insert(img->id(), img);
  s_imageInfoMap.insert(img->id(), Data::ImageInfo(*img));
  return *img;
}

QString ImageFactory::addImage(const QByteArray& data_, const QString& format_, const QString& id_) {
  Q_ASSERT(factory && "ImageFactory is not initialized!");
  return factory->addImageImpl(data_, format_, id_).id();
}

const Tellico::Data::Image& ImageFactory::addImageImpl(const QByteArray& data_, const QString& format_,
                                                       const QString& id_) {
  if(id_.isEmpty()) {
    return Data::Image::null;
  }

  // do not call imageById(), it causes infinite looping with Document::loadImage()
  Data::Image* img = d->imageCache.object(id_);
  if(img) {
    myLog() << "already exists in cache: " << id_;
    return *img;
  }

  img = d->imageDict.value(id_);
  if(img) {
    myLog() << "already exists in dict: " << id_;
    return *img;
  }

  img = new Data::Image(data_, format_, id_);
  if(img->isNull()) {
    myDebug() << "NULL IMAGE!!!!!";
    delete img;
    return Data::Image::null;
  }

//  myLog() << "//          << " bytes, format = " << format_
//          << ", id = "<< img->id();

  d->imageDict.insert(img->id(), img);
  s_imageInfoMap.insert(img->id(), Data::ImageInfo(*img));
  return *img;
}

const Tellico::Data::Image& ImageFactory::addCachedImageImpl(const QString& id_, CacheDir dir_) {
//  myLog() << "dir =" << (dir_ == DataDir ? "DataDir" : "TmpDir" ) << "; id =" << id_;
  Data::Image* img = 0;
  switch(dir_) {
    case DataDir:
      img = d->dataImageDir.imageById(id_);
      break;
    case LocalDir:
      img = d->localImageDir.imageById(id_);
      break;
    case TempDir:
      img = d->tempImageDir.imageById(id_);
      break;
    case ZipArchive:
      img = d->imageZipArchive.imageById(id_);
      break;
  }
  if(!img) {
    myWarning() << "image not found:" << id_;
    return Data::Image::null;
  }

  s_imageInfoMap.insert(img->id(), Data::ImageInfo(*img));

  // if numBytes() is greater than maxCost, then trying and failing to insert it would
  // mean the image gets deleted
  if(img->numBytes() > d->imageCache.maxCost()) {
    // can't hold it in the cache
    myWarning() << "Image cache is unable to hold the image, it's too big!";
    myWarning() << "Image name is " << img->id();
    myWarning() << "Image size is " << img->numBytes();
    myWarning() << "Max cache size is " << d->imageCache.maxCost();

    // add it back to the dict, but add the image to the list of
    // images to release later. Necessary to avoid a memory leak since new Image()
    // was called, we need to keep the pointer
    d->imageDict.insert(img->id(), img);
    s_imagesToRelease.add(img->id());
  } else if(!d->imageCache.insert(img->id(), img, img->numBytes())) {
    // at this point, img has been deleted!
    myWarning() << "Unable to insert into image cache";
    return Data::Image::null;
  }
  return *img;
}

bool ImageFactory::writeCachedImage(const QString& id_, CacheDir dir_, bool force_ /*=false*/) {
  if(id_.isEmpty()) {
    return false;
  }
//  myLog() << "dir =" << (dir_ == DataDir ? "DataDir" : "TmpDir" ) << "; id =" << id_;
  ImageDirectory* imgDir = dir_ == DataDir ? &factory->d->dataImageDir :
                          (dir_ == TempDir ? &factory->d->tempImageDir :
                                             &factory->d->localImageDir);
  Q_ASSERT(imgDir);
  bool exists = imgDir->hasImage(id_);
  // only write if it doesn't exist
  bool success = (!force_ && exists);
  if(!success) {
    const Data::Image& img = imageById(id_);
    if(!img.isNull()) {
      success = imgDir->writeImage(img);
    }
  }

  if(success) {
    // remove from dict and add to cache
    // it might not be in dict though
    if(factory->d->imageDict.contains(id_)) {
      Data::Image* img = factory->d->imageDict.take(id_);
      Q_ASSERT(img);
      if(factory->d->imageCache.insert(img->id(), img, img->numBytes())) {
        s_imageInfoMap.remove(id_);
      } else {
//      myLog() << "failed writing image to cache:" << id_;
//      myLog() << "removed from dict, deleting:" << img->id();
//      myLog() << "current dict size:" << d->imageDict.count();
      // can't insert it in the cache, so put it back in the dict
      // No, it's written to disk now, so we're safe
//      d->imageDict.insert(img->id(), img);
        delete img;
      }
    }
  }
  return success;
}

const Tellico::Data::Image& ImageFactory::imageById(const QString& id_) {
  if(id_.isEmpty()) {
    return Data::Image::null;
  }
//  myLog() << id_;

  // can't think of a better place to regularly check for images to release
  // but don't release image that just got asked for
  s_imagesToRelease.remove(id_);
  factory->releaseImages();

 // first check the cache, used for images that are in the data file, or are only temporary
 // then the dict, used for images downloaded, but not yet saved anywhere
  Data::Image* img = factory->d->imageCache.object(id_);
  if(img) {
//    myLog() << "found in cache";
    return *img;
  }

  img = factory->d->imageDict.value(id_);
  if(img) {
//    myLog() << "found in dict";
    return *img;
  }

  // if the image is link only, we need to load it
  // but can't call imageInfo() since that might recurse into imageById()
  // also, the image info cache might not have it so check if the
  // id is a valid absolute url
  // yeah, it's probably slow
  if((s_imageInfoMap.contains(id_) && s_imageInfoMap[id_].linkOnly) || !KUrl::isRelativeUrl(id_)) {
    KUrl u(id_);
    if(u.isValid()) {
      return factory->addImageImpl(u, true, KUrl(), true);
    }
  }

  // the document does a delayed loading of the images, sometimes
  // so an image could be in the tmp dir and not be in the cache
  // or it could be too big for the cache
  if(factory->d->tempImageDir.hasImage(id_)) {
    const Data::Image& img2 = factory->addCachedImageImpl(id_, TempDir);
    if(!img2.isNull()) {
//      myLog() << "found in tmp dir";
      return img2;
    }
  }

  // try to do a delayed loading of the image
  if(factory->d->imageZipArchive.hasImage(id_)) {
    const Data::Image& img2 = factory->addCachedImageImpl(id_, ZipArchive);
    if(!img2.isNull()) {
//      myLog() << "found in zip archive";
      // go ahead and write image to disk so we don't have to keep it in memory
      // calling pixmap() could be loading all the covers, and we don't want one
      // to get pushed out of the cache yet
      writeCachedImage(id_, TempDir);
      return img2;
    }
  }

  CacheDir dir1 = TempDir;
  CacheDir dir2 = TempDir;
  ImageDirectory* imgDir1 = 0;
  ImageDirectory* imgDir2 = 0;
  if(Config::imageLocation() == Config::ImagesInLocalDir) {
    dir1 = LocalDir;
    dir2 = DataDir;
    imgDir1 = &factory->d->localImageDir;
    imgDir2 = &factory->d->dataImageDir;
  } else if(Config::imageLocation() == Config::ImagesInAppDir) {
    dir1 = DataDir;
    dir2 = LocalDir;
    imgDir1 = &factory->d->dataImageDir;
    imgDir2 = &factory->d->localImageDir;
  }

  if(imgDir1 && imgDir1->hasImage(id_)) {
    const Data::Image& img2 = factory->addCachedImageImpl(id_, dir1);
    if(!img2.isNull()) {
      return img2;
    } else {
      myDebug() << "tried to add" << id_ << "from" << imgDir1->path() << "but failed";
    }
  } else if(imgDir2 && imgDir2->hasImage(id_)) {
    const Data::Image& img2 = factory->addCachedImageImpl(id_, dir2);
    if(!img2.isNull()) {
      // the img is in the other location
      factory->emitImageMismatch();
      return img2;
    } else {
      myDebug() << "tried to add" << id_ << "from" << imgDir2->path() << "but failed";
    }
  }
  // at this point, there's a possibility that the user has changed settings so that the images
  // are currently in a local or data directory, but Config::imageLocation() doesn't match that location
  if(factory->d->dataImageDir.hasImage(id_)) {
    const Data::Image& img2 = factory->addCachedImageImpl(id_, DataDir);
    if(!img2.isNull()) {
      return img2;
    }
  }
  if(factory->d->localImageDir.hasImage(id_)) {
    const Data::Image& img2 = factory->addCachedImageImpl(id_, LocalDir);
    if(!img2.isNull()) {
      return img2;
    }
  }
  myDebug() << "***not found:" << id_;
  return Data::Image::null;
}

Tellico::Data::ImageInfo ImageFactory::imageInfo(const QString& id_) {
  if(s_imageInfoMap.contains(id_)) {
    return s_imageInfoMap[id_];
  }

  const Data::Image& img = imageById(id_);
  if(img.isNull()) {
    return Data::ImageInfo();
  }
  return Data::ImageInfo(img);
}

void ImageFactory::cacheImageInfo(const Tellico::Data::ImageInfo& info) {
  s_imageInfoMap.insert(info.id, info);
}

bool ImageFactory::hasImageInfo(const QString& id_) {
  return s_imageInfoMap.contains(id_);
}

bool ImageFactory::validImage(const QString& id_) {
  // don't try s_imageInfoMap[id_] cause it inserts an empty image info
  return s_imageInfoMap.contains(id_) || factory->hasImage(id_) || !imageById(id_).isNull();
}

QPixmap ImageFactory::pixmap(const QString& id_, int width_, int height_) {
  if(id_.isEmpty()) {
    return QPixmap();
  }

  const QString key = id_ + QLatin1Char('|') + QString::number(width_) + QLatin1Char('|') + QString::number(height_);
  QPixmap* pix = factory->d->pixmapCache.object(key);
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
  if(!factory->d->pixmapCache.insert(key, pix, pix->width()*pix->height()*pix->depth()/8)) {
    myWarning() << "can't save in cache: " << id_;
    myWarning() << "### Current pixmap size is " << (pix->width()*pix->height()*pix->depth()/8);
    myWarning() << "### Max pixmap cache size is " << factory->d->pixmapCache.maxCost();
    QPixmap pix2(*pix);
    delete pix;
    return pix2;
  }
  return *pix;
}

void ImageFactory::clean(bool purgeTempDirectory_) {
  // the caches all auto-delete
  s_imagesToRelease.clear();
  qDeleteAll(factory->d->imageDict);
  factory->d->imageDict.clear();
  s_imageInfoMap.clear();
  factory->d->imageCache.clear();
  factory->d->pixmapCache.clear();
  if(purgeTempDirectory_) {
    factory->d->tempImageDir.purge();
  }
}

void ImageFactory::createStyleImages(int collectionType_, const Tellico::StyleOptions& opt_) {
  const QColor& baseColor = opt_.baseColor.isValid()
                          ? opt_.baseColor
                          : Config::templateBaseColor(collectionType_);
  const QColor& highColor = opt_.highlightedBaseColor.isValid()
                          ? opt_.highlightedBaseColor
                          : Config::templateHighlightedBaseColor(collectionType_);

  const QColor& bgc1 = KColorUtils::mix(baseColor, highColor, 0.3);
  const QColor& bgc2 = KColorUtils::mix(baseColor, highColor, 0.5);

  const QString bgname = QLatin1String("gradient_bg.png");
  QImage bgImage = Blitz::gradient(QSize(400, 1), bgc1, baseColor,
                                   Blitz::PipeCrossGradient);
  bgImage = bgImage.transformed(QTransform().rotate(90));

  const QString hdrname = QLatin1String("gradient_header.png");
  QImage hdrImage = Blitz::unbalancedGradient(QSize(1, 10), highColor, bgc2,
                                              Blitz::VerticalGradient, 100, -100);

  if(opt_.imgDir.isEmpty()) {
    // write the style images both to the tmp dir and the cache dir
    // doesn't really hurt and lets the user switch back and forth
    ImageFactory::removeImage(bgname, true /*delete */);
    factory->addImageImpl(Data::Image::byteArray(bgImage, "PNG"), QLatin1String("PNG"), bgname);
    ImageFactory::writeCachedImage(bgname, cacheDir(), true /*force*/);
    ImageFactory::writeCachedImage(bgname, TempDir, true /*force*/);

    ImageFactory::removeImage(hdrname, true /*delete */);
    factory->addImageImpl(Data::Image::byteArray(hdrImage, "PNG"), QLatin1String("PNG"), hdrname);
    ImageFactory::writeCachedImage(hdrname, cacheDir(), true /*force*/);
    ImageFactory::writeCachedImage(hdrname, TempDir, true /*force*/);
  } else {
    bgImage.save(opt_.imgDir + bgname, "PNG");
    hdrImage.save(opt_.imgDir + hdrname, "PNG");
  }
}

void ImageFactory::removeImage(const QString& id_, bool deleteImage_) {
  // be careful using this
  delete factory->d->imageDict.take(id_);
  factory->d->imageCache.remove(id_);

  if(deleteImage_) {
    // remove from everywhere
    factory->d->dataImageDir.removeImage(id_);
    factory->d->localImageDir.removeImage(id_);
    factory->d->tempImageDir.removeImage(id_);
  }
}

Tellico::StringSet ImageFactory::imagesNotInCache() {
  StringSet set;
  QHash<QString, Tellico::Data::Image*>::const_iterator end = factory->d->imageDict.constEnd();
  for(QHash<QString, Tellico::Data::Image*>::const_iterator it = factory->d->imageDict.constBegin(); it != end; ++it) {
    if(!factory->d->imageCache.contains(it.key())) {
      set.add(it.key());
    }
  }
  return set;
}

bool ImageFactory::hasImage(const QString& id_) const {
  return d->imageCache.contains(id_) || d->imageDict.contains(id_);
}

// the purpose here is to remove images from the dict if they're is on the disk somewhere,
// either in tempDir() or in dataDir(). The use for this is for calling pixmap() on an
// image too big to stay in the cache. Then it stays in the dict forever.
void ImageFactory::releaseImages() {
#ifdef RELEASE_IMAGES
  if(s_imagesToRelease.isEmpty()) {
    return;
  }

  foreach(const QString& id, s_imagesToRelease) {
    if(!d->imageDict.value(id)) {
      continue;
    }
    if(d->dataImageDir.hasImage(id) ||
       d->localImageDir.hasImage(id) ||
       d->tempImageDir.hasImage(id)) {
      delete d->imageDict.take(id);
    }
  }
  s_imagesToRelease.clear();
#endif
}

void ImageFactory::emitImageMismatch() {
  emit imageLocationMismatch();
}

void ImageFactory::setLocalDirectory(const KUrl& url_) {
  if(url_.isEmpty()) {
    return;
  }
  if(!url_.isLocalFile()) {
    myWarning() << "Tellico can only save images to local disk";
    myWarning() << "unable to save to " << url_;
  } else {
    QString dir = url_.directory(KUrl::AppendTrailingSlash);
    // could have already been set once
    if(!url_.fileName().contains(QLatin1String("_files"))) {
      dir += url_.fileName().section(QLatin1Char('.'), 0, 0) + QLatin1String("_files/");
    }
    factory->d->localImageDir.setPath(dir);
  }
}

void ImageFactory::setZipArchive(KZip* zip_) {
  if(!zip_) {
    return;
  }
  factory->d->imageZipArchive.setZip(zip_);
}

#undef RELEASE_IMAGES

#include "imagefactory.moc"
