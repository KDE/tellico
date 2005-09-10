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

#ifndef DATAIMPORTER_H
#define DATAIMPORTER_H

#include "importer.h"
#include "../filehandler.h"

namespace Tellico {
  namespace Import {

/**
 * @author Robby Stephenson
 */
class DataImporter : public Importer {
Q_OBJECT

public:
  enum Source { URL, Text };

  /**
   * @param url The URL of the file to import
   */
//  DataImporter(const KURL& url) : Importer(url), m_data(FileHandler::readDataFile(url)), m_source(URL) {}
  DataImporter(const KURL& url) : Importer(url), m_source(URL) { m_fileRef = FileHandler::fileRef(url); }
  /**
   * Since the conversion to a QCString appends a \0 character at the end, remove it.
   *
   * @param text The XML text. It MUST be in UTF-8.
   */
  DataImporter(const QString& text) : Importer(KURL()), m_data(text.utf8()), m_source(Text), m_fileRef(0)
    { m_data.truncate(m_data.size()-1); }
  /**
   */
  virtual ~DataImporter() { delete m_fileRef; m_fileRef = 0; }

  Source source() const { return m_source; }

protected:
  /**
   * Return the data in the imported file
   *
   * @return the file data
   */
  const QByteArray& data() const { return m_data; }
  FileHandler::FileRef& fileRef() const { return *m_fileRef; }

private:
  QByteArray m_data;
  const Source m_source;
  FileHandler::FileRef* m_fileRef;
};

  } // end namespace
} // end namespace
#endif
