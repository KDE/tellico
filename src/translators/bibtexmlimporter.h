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

#ifndef BIBTEXMLIMPORTER_H
#define BIBTEXMLIMPORTER_H

#include "xmlimporter.h"

#include <qdom.h>

namespace Tellico {
  namespace Import {

/**
 *@author Robby Stephenson
 */
class BibtexmlImporter : public XMLImporter {
Q_OBJECT

public:
  /**
   */
  BibtexmlImporter(const KURL& url) : Import::XMLImporter(url), m_coll(0) {}

  /**
   */
  virtual Data::Collection* collection();
  virtual bool canImport(int type) const;

private:
  void loadDomDocument();
  void readEntry(const QDomNode& entryNode);

  Data::Collection* m_coll;
};

  } // end namespace
} // end namespace
#endif
