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

#include "imageinfo.h"
#include "image.h"
#include "imagefactory.h"

using Tellico::Data::ImageInfo;

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
