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

#ifndef BOOKCASEIMPORTER_H
#define BOOKCASEIMPORTER_H

#include "dataimporter.h"

#include <qdom.h>

namespace Bookcase {
  namespace Import {

/**
 * Reading the @ref Bookcase data files is done by the BookcaseImporter.
 *
 * @author Robby Stephenson
 * @version $Id: bookcaseimporter.h 386 2004-01-24 05:12:28Z robby $
 */
class BookcaseImporter : public DataImporter {
Q_OBJECT

public:
  /**
   * @param url The bookcase data file.
   */
  BookcaseImporter(const KURL& url);
  /**
   * Constructor used to convert arbitrary text to a @ref Collection
   *
   * @param text The text
   */
  BookcaseImporter(const QString& text);

  /**
   */
  virtual Data::Collection* collection();

private:
  void loadXMLData(const QByteArray& data, bool loadImages);
  void loadZipData(const QByteArray& data);

  void readField(unsigned syntaxVersion, const QDomElement& elem);
  void readEntry(unsigned syntaxVersion, const QDomNode&elem);
  void readImage(const QDomElement& elem);

  Data::Collection* m_coll;
  float m_fraction;
};

  } // end namespace
} // end namespace
#endif
