/***************************************************************************
                             bibtexmlexporter.h
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

#ifndef BIBTEXMLEXPORTER_H
#define BIBTEXMLEXPORTER_H

#include "exporter.h"

/**
 * @author Robby Stephenson
 * @version $Id: bibtexmlexporter.h 261 2003-11-06 05:15:35Z robby $
 */
class BibtexmlExporter : public Exporter {
public: 
  BibtexmlExporter(const BCCollection* coll, const BCUnitList& list) : Exporter(coll, list) {}

  virtual QWidget* widget(QWidget*, const char*) { return 0; }
  virtual QString formatString() const;
  virtual QString text(bool format, bool encodeUTF8);
  virtual QString fileFilter() const;
  virtual void readOptions(KConfig*) {}
  virtual void saveOptions(KConfig*) {}
};

#endif
