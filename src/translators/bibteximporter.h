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

#ifndef BIBTEXIMPORTER_H
#define BIBTEXIMPORTER_H

namespace Bookcase {
  namespace Data {
    class Collection;
    class BibtexCollection;
  }
}
class Entry;

#include "textimporter.h"
#include "btparse/btparse.h"

#include <qptrlist.h>

typedef QPtrList<AST> ASTList;
typedef QPtrListIterator<AST> ASTListIterator;

namespace Bookcase {
  namespace Import {

/**
 * Bibtex files are used for bibliographies within LaTex. The btparse library is used to
 * parse the text and generate a @ref BibtexCollection.
 *
 * @author Robby Stephenson
 * @version $Id: bibteximporter.h 386 2004-01-24 05:12:28Z robby $
 */
class BibtexImporter : public TextImporter {
Q_OBJECT

public:
  /**
   * Initializes the btparse library
   *
   * @param url The url of the bibtex file
   */
  BibtexImporter(const KURL& url);
  /*
   * Some cleanup is done for the btparse library
   */
  virtual ~BibtexImporter();

  /**
   * Returns a pointer to a @ref BibtexCollection created on the stack. All entries
   * in the bibtex file are added, including any preamble, all macro strings, and each entry.
   *
   * @return A pointer to a @ref BibtexCollection, or 0 if none can be created.
   */
  virtual Data::Collection* collection();

private:
  QPtrList<AST> parseText(const QString& text) const;

  Data::BibtexCollection* m_coll;
};

  } // end namespace
} // end namespace
#endif
