/***************************************************************************
                             bibtexmlimporter.h
                             -------------------
    begin                : Sat Aug 2 2003
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

#ifndef BIBTEXMLIMPORTER_H
#define BIBTEXMLIMPORTER_H

class BCCollection;
class BCUnit;

#include "textimporter.h"

#include <qdom.h>

/**
 *@author Robby Stephenson
 */
class BibtexmlImporter : public TextImporter {
Q_OBJECT

public: 
  BibtexmlImporter(const KURL& url);

  /**
   */
  virtual BCCollection* collection();

private:
  void readDomDocument(const QString& text);
  void loadDomDocument();
  void readEntry(const QDomNode& entryNode);

  QDomDocument m_doc;
  BCCollection* m_coll;
};

#endif
