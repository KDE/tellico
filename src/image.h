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

#ifndef IMAGE_H
#define IMAGE_H

#include <qimage.h>
#include <qstring.h>

class KPixmapIO;

namespace Tellico {
  class ImageFactory;
  class FileHandler;

  namespace Data {

/**
 * @author Robby Stephenson
 */
class Image : public QImage {

friend class Tellico::ImageFactory;
friend class Tellico::FileHandler;

public:
  ~Image();

  const QString& id() const { return m_id; };
  const QCString& format() const { return m_format; };
  QByteArray byteArray() const;
  bool isNull() const;

  QPixmap convertToPixmap() const;
  QPixmap convertToPixmap(int width, int height) const;

  static QCString outputFormat(const QCString& inputFormat);
  static QByteArray byteArray(const QImage& img, const QCString& outputFormat);

private:
  Image();
  explicit Image(const QString& filename);
  Image(const QImage& image, const QString& format);
  Image(const QByteArray& data, const QString& format, const QString& id);

  //disable copy
  Image(const Image&);
  Image& operator=(const Image&);

  void setID(const QString& id);
  void calculateID();

  QString m_id;
  QCString m_format;

  static KPixmapIO* s_pixmapIO;
  static KPixmapIO* io();
};

class ImageInfo {
public:
  ImageInfo() {}
  explicit ImageInfo(const Image& img);
  ImageInfo(const QString& id, const QCString& format, int w, int h);
  QString id;
  QCString format;
  int width;
  int height;
};

  } // end namespace
} // end namespace

inline bool operator== (const Tellico::Data::Image& img1, const Tellico::Data::Image& img2) {
  return img1.id() == img2.id();
};

#endif
