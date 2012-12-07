/***************************************************************************
    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
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
