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

#ifndef ALEXANDRIAEXPORTER_H
#define ALEXANDRIAEXPORTER_H

class QDir;

#include "exporter.h"

namespace Tellico {
  namespace Data {
    class Entry;
  }
  namespace Export {

/**
 * @author Robby Stephenson
 * @version $Id: alexandriaexporter.h 862 2004-09-15 01:49:51Z robby $
 */
class AlexandriaExporter : public Exporter {
public:
  AlexandriaExporter(const Data::Collection* coll) : Exporter(coll) {}

  virtual QString formatString() const;
  virtual QWidget* widget(QWidget*, const char*) { return 0; }
  virtual bool exportEntries(bool formatFields) const;

  /**
   * This should never get called.
   */
  virtual void readOptions(KConfig*) {}
  /**
   * This should never get called.
   */
  virtual void saveOptions(KConfig*) {}
  /**
   * This should never get called.
   */
  virtual QString fileFilter() const { return QString::null; }
  /**
   * This should never get called.
   */
  virtual bool isText() const { return false; }
  /**
   * This should never get called.
   */
  virtual QString text(bool, bool) { return QString::null; }
  /**
   * This should never get called.
   */
  virtual QByteArray data(bool) { return QByteArray(); }

private:
  bool writeFile(const QDir& dir, Data::Entry* entry, bool formatFields) const;
};

  } // end namespace
} // end namespace
#endif
