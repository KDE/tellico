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
 * @version $Id: dataimporter.h 386 2004-01-24 05:12:28Z robby $
 */
class DataImporter : public Importer {
Q_OBJECT

public:
  /**
   * @param url The URL of the file to import
   */
  DataImporter(const KURL& url) : Importer(url), m_data(FileHandler::readDataFile(url)) {}
  /**
   * @param url The URL of the file to import
   */
  DataImporter(const QString& text) : Importer(KURL()), m_data(text.utf8()) {}
  /**
   */
  virtual ~DataImporter() {}

protected:
  /**
   * Return the URL of the imported file.
   *
   * @return the file URL
   */
  const QByteArray& data() const { return m_data; }

private:
  QByteArray m_data;
};

  } // end namespace
} // end namespace
#endif
