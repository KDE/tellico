/***************************************************************************
    copyright            : (C) 2007-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_GRIFFITHIMPORTER_H
#define TELLICO_GRIFFITHIMPORTER_H

#include "importer.h"

class KProcess;

namespace Tellico {
  namespace Import {

/**
 * An importer for importing collections used by Griffith, a movie collection manager.
 *
 * The database is assumed to be $HOME/.griffith/griffith.db. The file format is sqlite3,
 * and a python script, depending on pysqlite, is used to import the database
 *
 * @author Robby Stephenson
 */
class GriffithImporter : public Importer {
Q_OBJECT

public:
  /**
   */
  GriffithImporter() : Importer(), m_process(0) {}
  /**
   */
  virtual ~GriffithImporter();

  /**
   */
  virtual Data::CollPtr collection();
  virtual bool canImport(int type) const;

private slots:
  void slotData();
  void slotError();
  void slotProcessExited();

private:
  Data::CollPtr m_coll;

  KProcess* m_process;
  QByteArray m_data;
};

  } // end namespace
} // end namespace
#endif
