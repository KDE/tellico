/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
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

namespace Tellico {
  class ImageFactory;

  namespace Data {

/**
 * @author Robby Stephenson
 * @version $Id: image.h 862 2004-09-15 01:49:51Z robby $
 */
class Image : public QImage {

friend class ImageFactory;
friend class FileHandler;

public:
  Image() : QImage(), m_id(QString::null) {}

  const QString& id() const { return m_id; };
  const QCString& format() const { return m_format; };
  QByteArray byteArray() const;

  QPixmap convertToPixmap() const;
  QPixmap convertToPixmap(int width, int height) const;

  static QCString outputFormat(const QCString& inputFormat);

private:
  Image(const QString& filename);
  Image(const QImage& image, const QString& format);
  Image(const QByteArray& data, const QString& format, const QString& id);

  QString m_id;
  QCString m_format;
};

  } // end namespace
} // end namespace

inline bool operator== (const Tellico::Data::Image& img1, const Tellico::Data::Image& img2) {
  return img1.id() == img2.id();
};

#endif
