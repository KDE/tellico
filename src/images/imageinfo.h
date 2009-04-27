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

#ifndef TELLICO_IMAGEINFO_H
#define TELLICO_IMAGEINFO_H

#include <QString>
#include <QByteArray>

namespace Tellico {
  namespace Data {

class Image;

class ImageInfo {
public:
  ImageInfo() {}
  explicit ImageInfo(const Image& img);
  ImageInfo(const QString& id, const QByteArray& format, int w, int h, bool link);
  bool isNull() const { return id.isEmpty(); }
  QString id;
  QByteArray format;
  bool linkOnly : 1;

  int width(bool loadIfNecessary=true) const;
  int height(bool loadIfNecessary=true) const;

private:
  mutable int m_width;
  mutable int m_height;
};

  } // end namespace
} // end namespace

#endif
