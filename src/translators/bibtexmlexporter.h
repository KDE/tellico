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

#ifndef BIBTEXMLEXPORTER_H
#define BIBTEXMLEXPORTER_H

#include "textexporter.h"

namespace Bookcase {
  namespace Export {

/**
 * @author Robby Stephenson
 * @version $Id: bibtexmlexporter.h 386 2004-01-24 05:12:28Z robby $
 */
class BibtexmlExporter : public TextExporter {
public: 
  BibtexmlExporter(const Data::Collection* coll, const Data::EntryList& list) : TextExporter(coll, list) {}

  virtual QWidget* widget(QWidget*, const char*) { return 0; }
  virtual QString formatString() const;
  virtual QString text(bool format, bool encodeUTF8);
  virtual QString fileFilter() const;
  virtual void readOptions(KConfig*) {}
  virtual void saveOptions(KConfig*) {}
};

  } // end namespace
} // end namespace
#endif
