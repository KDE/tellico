/***************************************************************************
    copyright            : (C) 2007 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef GRIFFITHIMPORTER_H
#define GRIFFITHIMPORTER_H

#include "importer.h"
#include "../datavectors.h"

class KProcess;

namespace Tellico {
  namespace Import {

/**
 * An importer for importing collections used by Griffith, a movie colleciton manager.
 *
 * The database is assumed to be $HOME/.griffith/griffith.db. The file format is sqlite3,
 * and a python script, depending on pysqlite, i sused to import the database
 *
 * @author Robby Stephenson
 */
class GriffithImporter : public Importer {
Q_OBJECT

public:
  /**
   */
  GriffithImporter() : Importer(), m_coll(0), m_process(0) {}
  /**
   */
  virtual ~GriffithImporter();

  /**
   */
  virtual Data::CollPtr collection();
  virtual bool canImport(int type) const;

private slots:
  void slotData(KProcess* proc, char* buffer, int len);
  void slotError(KProcess* proc, char* buffer, int len);
  void slotProcessExited(KProcess* proc);

private:
  Data::CollPtr m_coll;

  KProcess* m_process;
  QByteArray m_data;
};

  } // end namespace
} // end namespace
#endif
