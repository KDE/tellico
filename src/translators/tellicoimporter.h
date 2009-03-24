/***************************************************************************
    copyright            : (C) 2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_IMPORT_TELLICOIMPORTER_H
#define TELLICO_IMPORT_TELLICOIMPORTER_H

#include "dataimporter.h"
#include "../datavectors.h"
#include "../utils/stringset.h"

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
  explicit TellicoImporter(const KUrl& url, bool loadAllImages=true);
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
  virtual Data::CollPtr collection();
  Format format() const { return m_format; }

  bool hasImages() const;
  bool loadImage(const QString& id_);

  static bool loadAllImages(const KUrl& url);

public slots:
  void slotCancel();

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

  QBuffer* m_buffer;
  KZip* m_zip;
  const KArchiveDirectory* m_imgDir;
};

  } // end namespace
} // end namespace
#endif
