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

#ifndef TEXTEXPORTER_H
#define TEXTEXPORTER_H

#include "exporter.h"

namespace Bookcase {
  namespace Export {

/**
 * The TextExporter class is meant as an abstract class for any exporter which operates on text files,
 * whether it be XML, Bibtex, or whatever.
 *
 * @author Robby Stephenson
 * @version $Id: textexporter.h 386 2004-01-24 05:12:28Z robby $
 */
class TextExporter : public Exporter {
public:
  TextExporter(const Data::Collection* coll, const Data::EntryList& list) : Exporter(coll, list) {}

  virtual bool isText() const { return true; }
  /**
   * This should never get called.
   */
  virtual QByteArray data(bool) { return QByteArray(); }
};

  } //end namespace
} //end namespace
#endif
