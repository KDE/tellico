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
#include "imagejob.h"
#include "../config/tellico_config.h"
#include "../utils/tellico_utils.h"
#include "../utils/gradient.h"
#include "../tellico_debug.h"

#include <KColorUtils>
#include <KZip>
#include <KIO/Global>

#include <QCache>
#include <QFileInfo>
#include <QDir>

#define RELEASE_IMAGES

using namespace Tellico;
using Tellico::ImageFactory;

// this image info map is primarily for big images that don't fit
// in the cache, so that don't have to be continually reloaded to get info
QHash<QString, Tellico::Data::ImageInfo> ImageFactory::s_imageInfoMap;
Tellico::StringSet ImageFactory::s_imagesToRelease;

Tellico::ImageFactory* ImageFactory::factory = nullptr;

class ImageFactory::Private {
public:
  Private() = default;

  QHash<QString, Data::Image*> imageDict;
  QCache<QString, Data::Image> imageCache;
  QCache<QString, QPixmap> pixmapCache;
  ImageDirectory dataImageDir; // kept in $HOME/.local/share/tellico/data/
  ImageDirectory localImageDir; // kept local to data file
  TemporaryImageDirectory tempImageDir; // kept in tmp directory
  ImageZipArchive imageZipArchive;
  StringSet nullImages;
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
  factory->d->dataImageDir.setPath(Tellico::saveLocation(QStringLiteral("data/")));
}

Tellico::ImageFactory* ImageFactory::self() {
  Q_ASSERT(factory && "ImageFactory is not initialized!");
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
  Q_ASSERT(factory);
  switch(cacheDir()) {
    case LocalDir: return localDir();
    case DataDir: return dataDir();
    case ZipArchive: return tempDir();
    case TempDir: return tempDir();
  }
  return tempDir();
}

Tellico::ImageFactory::CacheDir ImageFactory::cacheDir() {
  CacheDir dir = TempDir;
  switch(Config::imageLocation()) {
    case Config::ImagesInLocalDir: dir = LocalDir; break;
    case Config::ImagesInAppDir:   dir = DataDir;  break;
    case Config::ImagesInFile:     dir = TempDir;  break;
  }
  // special case when configured to use local dir but for a new collection when no local dir exists
  if(dir == LocalDir && factory->d->localImageDir.path().isEmpty()) {
    dir = TempDir;
  }
  return dir;
}

QString ImageFactory::addImage(const QUrl& url_, bool quiet_, const QUrl& refer_, bool link_) {
  Q_ASSERT(factory && "ImageFactory is not initialized!");
  return factory->addImageImpl(url_, quiet_, refer_, link_).id();
}

const Tellico::Data::Image& ImageFactory::addImageImpl(const QUrl& url_, bool quiet_, const QUrl& refer_, bool link_) {
  if(url_.isEmpty() || !url_.isValid() || d->nullImages.contains(url_.url())) {
    myDebug() << "Returning null image";
    return Data::Image::null;
  }
  ImageJob* job = new ImageJob(url_, QString(), quiet_);
  job->setLinkOnly(link_);
  job->setReferrer(refer_);

  if(!job->exec()) {
    myDebug() << "ImageJob failed to exec:" << job->errorString();
    // ERR_UNKNOWN is used when the returned image is truly null
    // rather than network error or some such
    if(job->error() == KIO::ERR_UNKNOWN) {
      d->nullImages.add(url_.url());
    }
    return Data::Image::null;
  }

  const Data::Image& img = job->image();
  if(img.isNull()) {
    myDebug() << "Null image for" << url_.toDisplayString();
    return Data::Image::null;
  }

  // hold the image in memory since it probably isn't written locally to disk yet
  if(!url_.isLocalFile() && !d->imageDict.contains(img.id())) {
    d->imageDict.insert(img.id(), new Data::Image(img));
    s_imageInfoMap.insert(img.id(), Data::ImageInfo(img));
  }
  return img;
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
  if(hasImageInMemory(img->id())) {
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
  Data::Image* img;
  if(!id_.isEmpty()) {
    // do not call imageById(), it causes infinite looping with Document::loadImage()
    img = d->imageCache.object(id_);
    if(img) {
      myLog() << "already exists in cache: " << id_;
      return *img;
    }
    img = d->imageDict.value(id_);
    if(img) {
      myLog() << "already exists in dict: " << id_;
      return *img;
    }
  }
  img = new Data::Image(data_, format_, id_);
  if(img->isNull()) {
    myDebug() << "NULL IMAGE!!!!!";
    delete img;
    return Data::Image::null;
  }

//  myLog() << "format = " << format_ << ", id = "<< img->id();

  d->imageDict.insert(img->id(), img);
  s_imageInfoMap.insert(img->id(), Data::ImageInfo(*img));
  return *img;
}

const Tellico::Data::Image& ImageFactory::addCachedImageImpl(const QString& id_, CacheDir dir_) {
//  myLog() << "dir =" << (dir_ == DataDir ? "DataDir" : "TmpDir" ) << "; id =" << id_;
  Data::Image* img = nullptr;
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

  // if byteCount() is greater than maxCost, then trying and failing to insert it would
  // mean the image gets deleted
  if(img->sizeInBytes() > d->imageCache.maxCost()) {
    // can't hold it in the cache
    myWarning() << "Image cache is unable to hold the image, it's too big!";
    myWarning() << "Image name is " << img->id();
    myWarning() << "Image size is " << img->sizeInBytes();
    myWarning() << "Max cache size is " << d->imageCache.maxCost();

    // add it back to the dict, but add the image to the list of
    // images to release later. Necessary to avoid a memory leak since new Image()
    // was called, we need to keep the pointer
    d->imageDict.insert(img->id(), img);
    s_imagesToRelease.add(img->id());
  } else if(!d->imageCache.insert(img->id(), img, img->sizeInBytes())) {
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
  ImageDirectory* imgDir = nullptr;
  switch(dir_) {
    case DataDir:
      imgDir = &factory->d->dataImageDir;
      break;
    case TempDir:
      imgDir = &factory->d->tempImageDir;
      break;
    case LocalDir:
      // special case when configured to use local dir but for a new collection when no local dir exists
      imgDir = factory->d->localImageDir.path().isEmpty() ?
                 &factory->d->tempImageDir :
                 &factory->d->localImageDir;
      break;
    case ZipArchive:
      myDebug() << "writeCachedImage() - ZipArchive - should never be called";
      imgDir = &factory->d->tempImageDir;
      break;
  }

  Q_ASSERT(imgDir);
  bool success = writeCachedImage(id_, imgDir, force_);

  if(success) {
    // remove from dict and add to cache
    // it might not be in dict though
    if(factory->d->imageDict.contains(id_)) {
      Data::Image* img = factory->d->imageDict.take(id_);
      Q_ASSERT(img);
      // imageCache.insert will delete the image by itself if the cost exceeds the cache size
      if(factory->d->imageCache.insert(img->id(), img, img->sizeInBytes())) {
        s_imageInfoMap.remove(id_);
      }
    }
  }
  return success;
}

bool ImageFactory::writeCachedImage(const QString& id_, ImageDirectory* imgDir_, bool force_ /*=false*/) {
  if(id_.isEmpty() || !imgDir_) {
    return false;
  }
//  myLog() << "ImageFactory::writeCachedImage() - dir =" << imgDir_->path() << "; id =" << id_;
  const bool exists = imgDir_->hasImage(id_);
  // only write if it doesn't exist
  bool success = (!force_ && exists);
  if(!success) {
    const Data::Image& img = imageById(id_);
    if(!img.isNull()) {
      success = imgDir_->writeImage(img);
    }
  }
  return success;
}

const Tellico::Data::Image& ImageFactory::imageById(const QString& id_) {
  Q_ASSERT(factory && "ImageFactory is not initialized!");
  if(id_.isEmpty() || !factory || factory->d->nullImages.contains(id_)) {
    return Data::Image::null;
  }
//  myLog() << "imageById" << id_;

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
  if((s_imageInfoMap.contains(id_) && s_imageInfoMap[id_].linkOnly) || !QUrl(id_).isRelative()) {
    QUrl u(id_);
    if(u.isValid()) {
      return factory->addImageImpl(u, true, QUrl(), true);
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

  // This section uses the config image setting plus the fallback location
  // to provide confidence that the user's image can be found
  CacheDir configLoc = TempDir;
  CacheDir fallbackLoc = TempDir;
  ImageDirectory* configImgDir = nullptr;
  ImageDirectory* fallbackImgDir = nullptr;
  if(Config::imageLocation() == Config::ImagesInLocalDir) {
    configLoc = LocalDir;
    fallbackLoc = DataDir;
    configImgDir = &factory->d->localImageDir;
    fallbackImgDir = &factory->d->dataImageDir;
  } else if(Config::imageLocation() == Config::ImagesInAppDir) {
    configLoc = DataDir;
    fallbackLoc = LocalDir;
    configImgDir = &factory->d->dataImageDir;
    fallbackImgDir = &factory->d->localImageDir;
  }

  // check the configured location first
  if(configImgDir && configImgDir->hasImage(id_)) {
    const Data::Image& img2 = factory->addCachedImageImpl(id_, configLoc);
    if(!img2.isNull()) {
//      myLog() << "found image in configured location" << configImgDir->path();
      return img2;
    } else {
      myDebug() << "tried to add" << id_ << "from" << configImgDir->path() << "but failed";
    }
  } else if(fallbackImgDir && fallbackImgDir->hasImage(id_)) {
    const Data::Image& img2 = factory->addCachedImageImpl(id_, fallbackLoc);
    if(!img2.isNull()) {
//      myLog() << "found image in fallback location" << fallbackImgDir->path() << id_;
      // the img is in the other location
      factory->emitImageMismatch();
      return img2;
    } else {
      myDebug() << "tried to add" << id_ << "from" << fallbackImgDir->path() << "but failed";
    }
  }
  // at this point, there's a possibility that the user has changed settings so that the images
  // are currently in a local or data directory, but Config::imageLocation() doesn't match that location
  if(factory->d->dataImageDir.hasImage(id_)) {
    const Data::Image& img2 = factory->addCachedImageImpl(id_, DataDir);
    if(!img2.isNull()) {
//      myLog() << "found image in data dir location";
      return img2;
    }
  }
  if(factory->d->localImageDir.hasImage(id_)) {
    const Data::Image& img2 = factory->addCachedImageImpl(id_, LocalDir);
    if(!img2.isNull()) {
//      myLog() << "found image in local dir location";
      return img2;
    }
  }
  // now, it appears the image doesn't exist. The only remaining possibility
  // is that the file name has multiple periods and as a result of the fix for bug 348088,
  // the image is lurking in a local image directory without the multiple periods
  if(Config::imageLocation() == Config::ImagesInLocalDir && QDir(localDir()).dirName().contains(QLatin1Char('.'))) {
    QString realImageDir = localDir();
    QDir d(realImageDir);
    // try to cd up and into the other old directory
    QString cdString = QLatin1String("../") + d.dirName().section(QLatin1Char('.'), 0, 0) + QLatin1String("_files/");
    if(d.cd(cdString)) {
      factory->d->localImageDir.setPath(d.path() + QDir::separator());
      if(factory->d->localImageDir.hasImage(id_)) {
//        myDebug() << "Reading image from old local directory" << (cdString+id_);
        const Data::Image& img2 = factory->addCachedImageImpl(id_, LocalDir);
        // Be sure to reset the image directory location!!
        factory->d->localImageDir.setPath(realImageDir);
        factory->d->localImageDir.writeImage(img2);
        return img2;
      }
      factory->d->localImageDir.setPath(realImageDir);
    }
  }

  myDebug() << "***ImageFactory::imageById() - not found:" << id_;
  return Data::Image::null;
}

bool ImageFactory::hasLocalImage(const QString& id_) {
  Q_ASSERT(factory && "ImageFactory is not initialized!");
  if(id_.isEmpty() || !factory) {
    return false;
  }
  bool ret = (Config::imageLocation() == Config::ImagesInLocalDir &&
                           factory->d->localImageDir.hasImage(id_)) ||
             (Config::imageLocation() == Config::ImagesInAppDir &&
                           factory->d->dataImageDir.hasImage(id_)) ||
             factory->d->imageCache.contains(id_) ||
             factory->d->imageDict.contains(id_) ||
             factory->d->tempImageDir.hasImage(id_) ||
             factory->d->imageZipArchive.hasImage(id_);
  if(ret) return true;

  const QUrl u(id_);
  return u.isValid() && !u.isRelative() && u.isLocalFile();
}

void ImageFactory::requestImageById(const QString& id_) {
  Q_ASSERT(factory && "ImageFactory is not initialized!");
  if(hasLocalImage(id_)) {
    emit factory->imageAvailable(id_);
    return;
  }
  if(factory->d->nullImages.contains(id_)) {
    // don't emit anything
    return;
  }
  const QUrl u(id_);
  // what does it mean when the Id is an absolute Url and yet the image is not link only?
  // Probably a heritage image id before the bugs were fixed.
  const bool linkOnly = (s_imageInfoMap.contains(id_) && s_imageInfoMap[id_].linkOnly);
  if(linkOnly || !u.isRelative()) {
    if(u.isValid()) {
      factory->requestImageByUrlImpl(u, true /* quiet */, QUrl() /* referrer */, linkOnly);
      if(!linkOnly) {
        myDebug() << "Loading an image url that is not link only. The image id will get updated.";
      }
    }
  }
}

void ImageFactory::requestImageByUrlImpl(const QUrl& url_, bool quiet_, const QUrl& refer_, bool link_) {
  ImageJob* job = new ImageJob(url_, QString() /* id, use calculated one */, quiet_);
  job->setLinkOnly(link_);
  job->setReferrer(refer_);
  connect(job, &ImageJob::result,
          this, &ImageFactory::slotImageJobResult);
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
  return s_imageInfoMap.contains(id_) || factory->hasImageInMemory(id_) || !imageById(id_).isNull();
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

  QPixmap pix2(*pix); // retain a copy of pix in case it doesn't go into the cache
  // pixmap size is w x h x d, divided by 8 bits
  const int size = (pix->width()*pix->height()*pix->depth()/8);
  if(!factory->d->pixmapCache.insert(key, pix, pix->width()*pix->height()*pix->depth()/8)) {
    // at this point, pix might be deleted
    myWarning() << "can't save in cache: " << id_;
    myWarning() << "### Current pixmap size is " << size;
    myWarning() << "### Max pixmap cache size is " << factory->d->pixmapCache.maxCost();
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
    myLog() << "Purging images from the temporary directory";
    factory->d->tempImageDir.purge();
    // just to make sure all the image locations clean themselves up
    // delete the factory (which deletes the storage objects) and
    // then recreate the factory, in case anything else needs it
    // be sure to save local image directory if it's not a temp dir
    // or the data dir, avoid setLocalDirectory
    const QString localDirName = factory->d->localImageDir.path();
    delete factory;
    factory = nullptr;
    ImageFactory::init();
    if(!localDirName.isEmpty() && QDir(localDirName).exists()) {
      factory->d->localImageDir.setPath(localDirName);
    }
  }
}

void ImageFactory::createStyleImages(int collectionType_, const Tellico::StyleOptions& opt_) {
  const QColor& baseColor = opt_.baseColor.isValid()
                          ? opt_.baseColor
                          : Config::templateBaseColor(collectionType_);
  const QColor& highColor = opt_.highlightedBaseColor.isValid()
                          ? opt_.highlightedBaseColor
                          : Config::templateHighlightedBaseColor(collectionType_);

  const QString bgname(QStringLiteral("gradient_bg.png"));
  const QColor& bgc1 = KColorUtils::mix(baseColor, highColor, 0.3);
  QImage bgImage = Tellico::gradient(QSize(600, 1), bgc1, baseColor,
                                     Tellico::PipeCrossGradient);
  bgImage = bgImage.transformed(QTransform().rotate(90));

  const QString hdrname(QStringLiteral("gradient_header.png"));
  const QColor& bgc2 = KColorUtils::mix(baseColor, highColor, 0.5);
  QImage hdrImage = Tellico::unbalancedGradient(QSize(1, 10), highColor, bgc2,
                                                Tellico::VerticalGradient, 100, -100);

  if(opt_.imgDir.isEmpty()) {
    ImageFactory::removeImage(bgname, true /*delete */);
    factory->addImageImpl(Data::Image::byteArray(bgImage, "PNG"), QStringLiteral("PNG"), bgname);
    ImageFactory::writeCachedImage(bgname, cacheDir(), true /*force*/);

    ImageFactory::removeImage(hdrname, true /*delete */);
    factory->addImageImpl(Data::Image::byteArray(hdrImage, "PNG"), QStringLiteral("PNG"), hdrname);
    ImageFactory::writeCachedImage(hdrname, cacheDir(), true /*force*/);
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

bool ImageFactory::hasImageInMemory(const QString& id_) const {
  return d->imageCache.contains(id_) || d->imageDict.contains(id_);
}

bool ImageFactory::hasNullImage(const QString& id_) const {
  return d->nullImages.contains(id_);
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
    if(!d->imageDict.contains(id)) {
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

QString ImageFactory::localDirectory(const QUrl& url_) {
  if(url_.isEmpty()) {
    return QString();
  }
  if(!url_.isLocalFile()) {
    myWarning() << "Tellico can only save images to local disk";
    myWarning() << "Unable to save images local to" << url_.toDisplayString();
    return QString();
  }
  QString dir = url_.adjusted(QUrl::RemoveFilename).toLocalFile();
  // could have already been set once
  if(!dir.endsWith(QLatin1String("_files/"))) {
    QFileInfo fi(url_.fileName());
    dir += fi.completeBaseName() + QLatin1String("_files/");
  }
  return dir;
}

void ImageFactory::setLocalDirectory(const QUrl& url_) {
  Q_ASSERT(factory && "ImageFactory is not initialized!");
  const QString localDirName = localDirectory(url_);
  if(!localDirName.isEmpty()) {
    factory->d->localImageDir.setPath(localDirName);
  }
}

void ImageFactory::setZipArchive(std::unique_ptr<KZip> zip_) {
  Q_ASSERT(factory && "ImageFactory is not initialized!");
  if(!zip_) {
    return;
  }
  factory->d->imageZipArchive.setZip(std::move(zip_));
}

void ImageFactory::slotImageJobResult(KJob* job_) {
  ImageJob* imageJob = qobject_cast<ImageJob*>(job_);
  Q_ASSERT(imageJob);
  if(!imageJob) {
    myWarning() << "No image job";
    return;
  }
  const Data::Image& img = imageJob->image();
  if(img.isNull()) {
     myDebug() << "null image for" << imageJob->url();
     d->nullImages.add(imageJob->url().url());
     // don't emit anything
     return;
  }

  // hold the image in memory since it probably isn't written locally to disk yet
  if(!d->imageDict.contains(img.id())) {
    d->imageDict.insert(img.id(), new Data::Image(img));
    s_imageInfoMap.insert(img.id(), Data::ImageInfo(img));
  }
  emit factory->imageAvailable(img.id());
}

#undef RELEASE_IMAGES
