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

#ifndef TEXTIMPORTER_H
#define TEXTIMPORTER_H

#include "importer.h"

namespace Tellico {
  namespace Import {

/**
 * The TextImporter class is meant as an abstract class for any importer which reads text files.
 *
 * @author Robby Stephenson
 */
class TextImporter : public Importer {
Q_OBJECT

public:
  /**
   * In the constructor, the contents of the file are read.
   *
   * @param url The file to be imported
   */
  TextImporter(const KURL& url);

protected:
  /**
   * Returns the contents of the imported file.
   *
   * @return The file contents
   */
  const QString& text() const { return m_text; }

private:
  QString m_text;
};

  } // end namespace
} // end namespace
#endif
