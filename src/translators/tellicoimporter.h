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

#ifndef TELLICO_IMPORTER_H
#define TELLICO_IMPORTER_H

class QBuffer;
class KZip;
class KArchiveDirectory;

#include "dataimporter.h"
#include "../datavectors.h"
#include "../stringset.h"

class QDomElement;

namespace Tellico {
  namespace Import {

/**
 * Reading the @ref Tellico data files is done by the TellicoImporter.
 *
 * @author Robby Stephenson
 */
class TellicoImporter : public DataImporter {
Q_OBJECT

public:
  enum Format { Unknown, Error, XML, Zip, Cancel };

  /**
   * @param url The tellico data file.
   */
  TellicoImporter(const KURL& url, bool loadAllImages=true);
  /**
   * Constructor used to convert arbitrary text to a @ref Collection
   *
   * @param text The text
   */
  TellicoImporter(const QString& text);
  virtual ~TellicoImporter();

  /**
   * sometimes, a new document format might add data
   */
  bool modifiedOriginal() const { return m_modified; }

  /**
   */
  virtual Data::CollPtr collection();
  Format format() const { return m_format; }

  bool hasImages() const { return m_hasImages; }
  bool loadImage(const QString& id_);

  static bool loadAllImages(const KURL& url);

public slots:
  void slotCancel();

private:
  static bool versionConversion(uint from, uint to);

  void loadXMLData(const QByteArray& data, bool loadImages);
  void loadZipData();

  void readField(uint syntaxVersion, const QDomElement& elem);
  void readEntry(uint syntaxVersion, const QDomElement& elem);
  void readImage(const QDomElement& elem, bool loadImage);
  void readFilter(const QDomElement& elem);
  void readBorrower(const QDomElement& elem);
  void addDefaultFilters();

  Data::CollPtr m_coll;
  bool m_loadAllImages;
  QString m_namespace;
  Format m_format;
  bool m_modified : 1;
  bool m_cancelled : 1;
  bool m_hasImages : 1;
  StringSet m_images;

  QBuffer* m_buffer;
  KZip* m_zip;
  const KArchiveDirectory* m_imgDir;
};

  } // end namespace
} // end namespace
#endif
