/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
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

#include "dataimporter.h"
#include "../entry.h"
#include "../datavectors.h"

#include <qdom.h>

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
  enum Format { Unknown, Error, XML, Zip };

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

  /**
   * sometimes, a new document format might add data
   */
  bool modifiedOriginal() const { return m_modified; }

  /**
   */
  virtual Data::Collection* collection();
  Format format() const { return m_format; }

  /**
   */
  static bool loadImage(const KURL& url, const QString& id);

private:
  void loadXMLData(const QByteArray& data, bool loadImages);
  void loadZipData();

  void readField(unsigned syntaxVersion, const QDomElement& elem);
  void readEntry(unsigned syntaxVersion, const QDomElement& elem);
  void readImage(const QDomElement& elem);
  void readFilter(const QDomElement& elem);
  void readBorrower(const QDomElement& elem);
  void addDefaultFilters();

  Data::Collection* m_coll;
  bool m_loadAllImages;
  QString m_namespace;
  Format m_format;
  bool m_modified;
};

  } // end namespace
} // end namespace
#endif
