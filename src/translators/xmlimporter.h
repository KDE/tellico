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

#ifndef XMLIMPORTER_H
#define XMLIMPORTER_H

#include "importer.h"

#include <qdom.h>

namespace Bookcase {
  namespace Import {

/**
 * The XMLImporter class is meant as an abstract class for any importer which reads xml files.
 *
 * @author Robby Stephenson
 * @version $Id: xmlimporter.h 386 2004-01-24 05:12:28Z robby $
 */
class XMLImporter : public Importer {
Q_OBJECT

public:
  /**
   * In the constructor, the contents of the file are read.
   *
   * @param url The file to be imported
   */
  XMLImporter(const KURL& url);
  /**
   * Imports xml text.
   *
   * @param text The text
   */
  XMLImporter(const QString& text);
  /**
   * Imports xml text from a byte array.
   *
   * @param data The Data
   */
  XMLImporter(const QByteArray& data);

protected:
  /**
   * Returns the contents of the imported file.
   *
   * @return The file contents
   */
  const QDomDocument& domDocument() const { return m_dom; }

private:
  QDomDocument m_dom;
};

  } // end namespace
} // end namespace
#endif
