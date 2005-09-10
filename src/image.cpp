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

#include "image.h"
#include "tellico_debug.h"

#include <kmdcodec.h>
#include <kpixmapio.h>
#include <kdebug.h>

#include <qbuffer.h>

using Tellico::Data::Image;

// I'm using the MD5 hash as the id. I consider it rather unlikely that two images in one
// collection could ever have the same hash, and this lets me do a fast comparison of two images
// simply by comparing their ids.
Image::Image(const QString& filename_) : QImage(filename_) {
  m_format = QImage::imageFormat(filename_);

  KMD5 md5(byteArray());
  // the id will eventually be used as a filename
  m_id = QString::fromLatin1(md5.hexDigest()) + QString::fromLatin1(".") + QString::fromLatin1(m_format).lower();
}

Image::Image(const QImage& img_, const QString& format_) : QImage(img_), m_format(format_) {
  KMD5 md5(byteArray());
  // the id will eventually be used as a filename
  m_id = QString::fromLatin1(md5.hexDigest()) + QString::fromLatin1(".") + QString::fromLatin1(m_format).lower();
}

Image::Image(const QByteArray& data_, const QString& format_, const QString& id_)
    : QImage(data_), m_id(id_), m_format(format_) {
}

QByteArray Image::byteArray() const {
  QByteArray ba;
  QBuffer buf(ba);
  buf.open(IO_WriteOnly);
  QImageIO iio(&buf, outputFormat(m_format));
  iio.setImage(*this);
  iio.write();
  buf.close();
  return ba;
}

QPixmap Image::convertToPixmap() const {
  static KPixmapIO io;
  return io.convertToPixmap(*this);
}

QPixmap Image::convertToPixmap(int w_, int h_) const {
  static KPixmapIO io;
  return io.convertToPixmap(this->smoothScale(w_, h_, ScaleMin));
}

QCString Image::outputFormat(const QCString& inputFormat) {
  QStrList list = QImage::outputFormats();
  for(QStrListIterator it(list); it.current(); ++it) {
    if(inputFormat == it.current()) {
      return inputFormat;
    }
  }
  return "PNG";
}
