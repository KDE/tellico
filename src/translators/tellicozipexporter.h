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

#ifndef TELLICOZIPEXPORTER_H
#define TELLICOZIPEXPORTER_H

#include "dataexporter.h"

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 * @version $Id: tellicozipexporter.h 867 2004-09-15 03:04:49Z robby $
 */
class TellicoZipExporter : public DataExporter {
public:
  TellicoZipExporter(const Data::Collection* coll) : DataExporter(coll) {};

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
