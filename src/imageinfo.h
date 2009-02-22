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
