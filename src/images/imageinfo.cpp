/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#include "imageinfo.h"
#include "image.h"
#include "imagefactory.h"

using Tellico::Data::ImageInfo;

ImageInfo::ImageInfo(const Data::Image& img_)
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
