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

#ifndef DATAIMPORTER_H
#define DATAIMPORTER_H

#include "importer.h"
#include "../filehandler.h"

namespace Bookcase {
  namespace Import {

/**
 * @author Robby Stephenson
 * @version $Id: dataimporter.h 816 2004-08-27 05:51:02Z robby $
 */
class DataImporter : public Importer {
Q_OBJECT

public:
  /**
   * @param url The URL of the file to import
   */
  DataImporter(const KURL& url) : Importer(url), m_data(FileHandler::readDataFile(url)) {}
  /**
   * Since the conversion to a QCString appends a \0 character at the end, remove it.
   *
   * @param text The XML text. It MUST be in UTF-8.
   */
  DataImporter(const QString& text) : Importer(KURL()), m_data(text.utf8()) { m_data.truncate(m_data.size()-1); }
  /**
   */
  virtual ~DataImporter() {}

protected:
  /**
   * Return the data in the imported file
   *
   * @return the file data
   */
  const QByteArray& data() const { return m_data; }

private:
  QByteArray m_data;
};

  } // end namespace
} // end namespace
#endif
