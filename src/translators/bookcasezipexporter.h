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

#ifndef BOOKCASEZIPEXPORTER_H
#define BOOKCASEZIPEXPORTER_H

#include "dataexporter.h"

namespace Bookcase {
  namespace Export {

/**
 * @author Robby Stephenson
 * @version $Id: bookcasezipexporter.h 580 2004-03-27 03:34:33Z robby $
 */
class BookcaseZipExporter : public DataExporter {
public:
  BookcaseZipExporter(const Data::Collection* coll, const Data::EntryList& list);

  virtual QWidget* widget(QWidget*, const char*) { return 0; }
  virtual QString formatString() const;
  virtual QByteArray data(bool formatFields);
  virtual QString fileFilter() const;
  virtual void readOptions(KConfig*) {}
  virtual void saveOptions(KConfig*) {}
};

  } // end namespace
} // end namespace
#endif
