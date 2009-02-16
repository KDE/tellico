/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
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
#include "imagefactory.h"
#include "tellico_debug.h"
#include "tellico_utils.h"

#include <kcodecs.h>

#include <QBuffer>
#include <QRegExp>
#include <QImageReader>
#include <QImageWriter>

using Tellico::Data::Image;
using Tellico::Data::ImageInfo;

Image::Image() : QImage(), m_id(QString::null), m_linkOnly(false) {
}

// I'm using the MD5 hash as the id. I consider it rather unlikely that two images in one
// collection could ever have the same hash, and this lets me do a fast comparison of two images
// simply by comparing their ids.
Image::Image(const QString& filename_) : QImage(filename_), m_linkOnly(false) {
  m_format = QImageReader::imageFormat(filename_);
  calculateID();
}

Image::Image(const QImage& img_, const QString& format_) : QImage(img_), m_format(format_.toLatin1()), m_linkOnly(false) {
  calculateID();
}

Image::Image(const QByteArray& data_, const QString& format_, const QString& id_)
    : QImage(QImage::fromData(data_)), m_id(idClean(id_)), m_format(format_.toLatin1()), m_linkOnly(false) {
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
  return QPixmap::fromImage(*this);
}

QPixmap Image::convertToPixmap(int w_, int h_) const {
  if(w_ < width() || h_ < height()) {
    return QPixmap::fromImage(this->scaled(w_, h_, Qt::KeepAspectRatio));
  } else {
    return QPixmap::fromImage(*this);
  }
}

QByteArray Image::outputFormat(const QByteArray& inputFormat) {
  QList<QByteArray> list = QImageWriter::supportedImageFormats();
  if(list.contains(inputFormat)) {
    return inputFormat;
  }
//  myDebug() << "Image::outputFormat() - writing " << inputFormat << " as PNG" << endl;
  return "PNG";
}

QByteArray Image::byteArray(const QImage& img_, const QByteArray& outputFormat_) {
  QByteArray ba;
  QBuffer buf(&ba);
  buf.open(QIODevice::WriteOnly);
  QImageWriter writer(&buf, outputFormat_);
  writer.write(img_);
  buf.close();
  return ba;
}

QString Image::idClean(const QString& id_) {
  static const QRegExp rx('[' + QRegExp::escape(QString::fromLatin1("/@<>#\"&%?={}|^~[]'`\\:+")) + ']');
  QString clean = id_;
  return shareString(clean.remove(rx));
}

void Image::setID(const QString& id_) {
  m_id = id_;
}

void Image::calculateID() {
  // the id will eventually be used as a filename
  if(!isNull()) {
    KMD5 md5(byteArray());
    m_id = QString::fromLatin1(md5.hexDigest()) + QString::fromLatin1(".") + QString::fromLatin1(m_format).toLower();
    m_id = idClean(m_id);
  }
}

/******************************************************/

ImageInfo::ImageInfo(const Image& img_)
    : id(img_.id())
    , format(img_.format())
    , linkOnly(img_.linkOnly())
    , m_width(img_.width())
    , m_height(img_.height()) {
}

ImageInfo::ImageInfo(const QString& id_, const QByteArray& format_, int w_, int h_, bool l_)
    : id(id_)
    , format(format_)
    , linkOnly(l_)
    , m_width(w_)
    , m_height(h_) {
}

int ImageInfo::width(bool loadIfNecessary) const {
  if(m_width < 1 && loadIfNecessary) {
    const Image& img = ImageFactory::imageById(id);
    if(!img.isNull()) {
      m_width = img.width();
      m_height = img.height();
    }
  }
  return m_width;
}

int ImageInfo::height(bool loadIfNecessary) const {
  if(m_height < 1 && loadIfNecessary) {
    const Image& img = ImageFactory::imageById(id);
    if(!img.isNull()) {
      m_width = img.width();
      m_height = img.height();
    }
  }
  return m_height;
}
