/***************************************************************************
    Copyright (C) 2003-2025 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_UTILS_H
#define TELLICO_UTILS_H

#include <QStringList>

class QPixmap;
class QDialog;
namespace KIO {
  class Job;
}

/**
 * This file contains utility functions.
 *
 * @author Robby Stephenson
 */
namespace Tellico {
  /**
   * Returns a list of the subdirectories in @param dir
   * Symbolic links are ignored
   */
  QStringList findAllSubDirs(const QString& dir);
  /**
   * Does something similar to KDE4 KStandardDirs::findALlResources with wildcards
   */
  QStringList locateAllFiles(const QString& fileName);
  /**
   * Return something likely to be the installation directory
   */
  QString installationDir();

  QString saveLocation(const QString& dir);

  const QPixmap& pixmap(const QString& value);

  /**
   * Checks that the tellico-common.xsl file has been properly copied
   * to the user's directory.
   */
  bool checkCommonXSLFile();

  void activateDialog(QDialog* dialog);

  void addUserAgent(KIO::Job* job);
}

#endif
