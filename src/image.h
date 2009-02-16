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

#ifndef TELLICO_IMAGE_H
#define TELLICO_IMAGE_H

#include <QImage>
#include <QString>
#include <QByteArray>
#include <QPixmap>

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
  const QByteArray& format() const { return m_format; };
  QByteArray byteArray() const;
  bool isNull() const;
  bool linkOnly() const { return m_linkOnly; }
  void setLinkOnly(bool l) { m_linkOnly = l; }

  QPixmap convertToPixmap() const;
  QPixmap convertToPixmap(int width, int height) const;

  static QByteArray outputFormat(const QByteArray& inputFormat);
  static QByteArray byteArray(const QImage& img, const QByteArray& outputFormat);
  static QString idClean(const QString& id);

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
  QByteArray m_format;
  bool m_linkOnly : 1;
};

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

inline bool operator== (const Tellico::Data::Image& img1, const Tellico::Data::Image& img2) {
  return img1.id() == img2.id();
}

#endif
