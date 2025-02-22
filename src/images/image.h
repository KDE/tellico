/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_IMAGE_H
#define TELLICO_IMAGE_H

#include <QImage>
#include <QString>
#include <QByteArray>
#include <QPixmap>

class TellicoReadTest;
namespace Tellico {
  class ImageFactory;
  class ImageDirectory;
  class ImageZipArchive;
  class FileHandler;
  class ImageJob;

  namespace Data {

/**
 * @author Robby Stephenson
 */
class Image : public QImage {

friend class ::TellicoReadTest;
friend class Tellico::ImageFactory;
friend class Tellico::ImageDirectory;
friend class Tellico::ImageZipArchive;
friend class Tellico::FileHandler;
friend class Tellico::ImageJob;

public:
  Image(const Image& image);
  Image& operator=(const Image&);
  ~Image();

  const QString& id() const { return m_id; }
  const QByteArray& format() const { return m_format; }
  QByteArray byteArray() const;
  bool isNull() const;
  bool linkOnly() const { return m_linkOnly; }
  void setLinkOnly(bool l) { m_linkOnly = l; }

  QPixmap convertToPixmap() const;
  QPixmap convertToPixmap(int width, int height) const;

  static QByteArray outputFormat(const QByteArray& inputFormat);
  static QByteArray byteArray(const QImage& img, const QByteArray& outputFormat);
  static QString idClean(const QString& id);
  static QString calculateID(const QByteArray& data, const QString& format);

  static const Image null;

private:
  Image();
  explicit Image(const QString& filename, const QString& id = QString());
  Image(const QImage& image, const QString& format);
  Image(const QByteArray& data, const QString& format, const QString& id);

  void setID(const QString& id);
  void setFormat(const QByteArray& format_) { m_format = format_; }
  void calculateID();

  QString m_id;
  QByteArray m_format;
  bool m_linkOnly : 1;

  static QList<QByteArray> s_outputFormats;
};

  } // end namespace
} // end namespace

#endif
