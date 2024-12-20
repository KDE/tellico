/***************************************************************************
    Copyright (C) 2008-2020 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_IMPORT_TELLICOIMPORTER_H
#define TELLICO_IMPORT_TELLICOIMPORTER_H

#include "dataimporter.h"
#include "../datavectors.h"
#include "../utils/stringset.h"

#include <KZip>

#include <memory>

class QBuffer;
class KZip;
class KArchiveDirectory;

namespace Tellico {
  namespace Import {

/**
 * @author Robby Stephenson
 */
class TellicoImporter : public DataImporter {
Q_OBJECT

public:
  enum Format { Unknown, Error, XML, Zip, Cancel };

  /**
   * @param url The tellico data file.
   */
  explicit TellicoImporter(const QUrl& url, bool loadAllImages=true);
  /**
   * Constructor used to convert arbitrary text to a @ref Collection
   *
   * @param text The text
   */
  explicit TellicoImporter(const QString& text);
  virtual ~TellicoImporter();

  /**
   * sometimes, a new document format might add data
   */
  bool modifiedOriginal() const { return m_modified; }

  /**
   */
  virtual Data::CollPtr collection() override;
  /**
   * The TellicoImporter can import any type known to Tellico. Obviously.
   */
  virtual bool canImport(int type) const override { Q_UNUSED(type); return true; }
  Format format() const { return m_format; }

  bool hasImages() const;
  bool loadImage(const QString& id_);

  void setBaseUrl(const QUrl& url) { m_baseUrl = url; }

  // take ownership of zip object with images
  std::unique_ptr<KZip> takeImages();

  static bool loadAllImages(const QUrl& url);

public Q_SLOTS:
  void slotCancel() override;

private:
  void loadXMLData(const QByteArray& data, bool loadImages);
  void loadZipData();

  Data::CollPtr m_coll;
  bool m_loadAllImages;
  QString m_namespace;
  Format m_format;
  bool m_modified;
  bool m_cancelled;
  bool m_hasImages;
  StringSet m_images;
  QUrl m_baseUrl;

  std::unique_ptr<QBuffer> m_buffer;
  std::unique_ptr<KZip> m_zip;
  // no ownership of this pointer
  const KArchiveDirectory* m_imgDir;
};

  } // end namespace
} // end namespace
#endif
