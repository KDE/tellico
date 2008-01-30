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

#include "image.h"
#include "tellico_debug.h"

#include <kmdcodec.h>
#include <kpixmapio.h>
#include <kstaticdeleter.h>

#include <qbuffer.h>
#include <qregexp.h>

using Tellico::Data::Image;
using Tellico::Data::ImageInfo;

KPixmapIO* Image::s_pixmapIO = 0;
static KStaticDeleter<KPixmapIO> staticKPixmapIODeleter;
KPixmapIO* Image::io() {
  if(!s_pixmapIO) {
    staticKPixmapIODeleter.setObject(s_pixmapIO, new KPixmapIO());
  }
  return s_pixmapIO;
}

Image::Image() : QImage(), m_id(QString::null), m_linkOnly(false) {
}

// I'm using the MD5 hash as the id. I consider it rather unlikely that two images in one
// collection could ever have the same hash, and this lets me do a fast comparison of two images
// simply by comparing their ids.
Image::Image(const QString& filename_) : QImage(filename_), m_linkOnly(false) {
  m_format = QImage::imageFormat(filename_);
  calculateID();
}

Image::Image(const QImage& img_, const QString& format_) : QImage(img_), m_format(format_), m_linkOnly(false) {
  calculateID();
}

Image::Image(const QByteArray& data_, const QString& format_, const QString& id_)
    : QImage(data_), m_id(idClean(id_)), m_format(format_), m_linkOnly(false) {
  if(isNull()) {
    m_id = QString();
  }
}

Image::~Image() {
}

QByteArray Image::byteArray() const {
  return byteArray(*this, outputFormat(m_format));
}

bool Image::isNull() const {
  // 1x1 images are considered null for Tellico. Amazon returns some like that.
  return QImage::isNull() || (width() < 2 && height() < 2);
}

QPixmap Image::convertToPixmap() const {
  return io()->convertToPixmap(*this);
}

QPixmap Image::convertToPixmap(int w_, int h_) const {
  if(w_ < width() || h_ < height()) {
    return io()->convertToPixmap(this->smoothScale(w_, h_, ScaleMin));
  } else {
    return io()->convertToPixmap(*this);
  }
}

QCString Image::outputFormat(const QCString& inputFormat) {
  QStrList list = QImage::outputFormats();
  for(QStrListIterator it(list); it.current(); ++it) {
    if(inputFormat == it.current()) {
      return inputFormat;
    }
  }
//  myDebug() << "Image::outputFormat() - writing " << inputFormat << " as PNG" << endl;
  return "PNG";
}

QByteArray Image::byteArray(const QImage& img_, const QCString& outputFormat_) {
  QByteArray ba;
  QBuffer buf(ba);
  buf.open(IO_WriteOnly);
  QImageIO iio(&buf, outputFormat_);
  iio.setImage(img_);
  iio.write();
  buf.close();
  return ba;
}

QString Image::idClean(const QString& id_) {
  static const QRegExp rx('[' + QRegExp::escape(QString::fromLatin1("/@<>#\"&%?={}|^~[]'`\\:+")) + ']');
  QString clean = id_;
  return clean.remove(rx);
}

void Image::setID(const QString& id_) {
  m_id = id_;
}

void Image::calculateID() {
  // the id will eventually be used as a filename
  if(!isNull()) {
    KMD5 md5(byteArray());
    m_id = QString::fromLatin1(md5.hexDigest()) + QString::fromLatin1(".") + QString::fromLatin1(m_format).lower();
    m_id = idClean(m_id);
  }
}

/******************************************************/

ImageInfo::ImageInfo(const Image& img_)
    : id(img_.id())
    , format(img_.format())
    , width(img_.width())
    , height(img_.height())
    , linkOnly(img_.linkOnly()) {
}

ImageInfo::ImageInfo(const QString& id_, const QCString& format_, int w_, int h_, bool l_)
    : id(id_)
    , format(format_)
    , width(w_)
    , height(h_)
    , linkOnly(l_) {
}
