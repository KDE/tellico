/***************************************************************************
                            bookcasexmlimporter.h
                             -------------------
    begin                : Wed Sep 24 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BOOKCASEXMLIMPORTER_H
#define BOOKCASEXMLIMPORTER_H

#include "textimporter.h"

#include <qdom.h>

/**
 * Reading the @ref Bookcase data files, which are saved in XML format, is done by the
 * BookcaseXMLImporter.
 *
 * @author Robby Stephenson
 * @version $Id: bookcasexmlimporter.h 218 2003-10-24 01:51:48Z robby $
 */
class BookcaseXMLImporter : public TextImporter {
Q_OBJECT

public:
  /**
   * The text is parsed into a @ref QDomDocument immediately when the importer is created.
   *
   * @param url The bookcase data file.
   */
  BookcaseXMLImporter(const KURL& url);
  /**
   * Constructor used to convert arbitrary text to a @ref BCCollection
   *
   * @param text The text
   */
  BookcaseXMLImporter(const QString& text);

  /**
   */
  virtual BCCollection* collection();

private:
  void readDomDocument(const QString& text);
  void loadDomDocument();
  void readAttribute(unsigned syntaxVersion, const QDomElement& attelem);
  void readUnit(unsigned syntaxVersion, const QDomNode& unitelem);

  QDomDocument m_doc;
  BCCollection* m_coll;
};

#endif
