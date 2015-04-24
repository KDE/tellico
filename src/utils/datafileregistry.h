/***************************************************************************
    Copyright (C) 2015 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_DATAFILEREGISTRY_H
#define TELLICO_DATAFILEREGISTRY_H

#include <QStringList>

namespace Tellico {

/**
 * @author Robby Stephenson
 *
 * This class replicates, in small part, a functionality deprecated and removed
 * by the KDE4 KStandardDirs class for adding a list of custom directories for
 * searching for resource types. This class only looks for data file, and defaults
 * to QStandardPaths::DataLocation. For unit tests, in particular, it is useful
 * to be able to read and use data files from the source directory.
 */
class DataFileRegistry {

public:
  static DataFileRegistry* self();

  void addDataLocation(const QString& dir, bool prepend = false);
  QString locate(const QString& fileName);

private:
  DataFileRegistry();

  QStringList m_dataLocations;
};

} // end namespace
#endif
