/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include "tellico_utils.h"

#include <KGlobal>
#include <KLibrary>
#include <KStandardDirs>

#include <QDir>

QStringList Tellico::findAllSubDirs(const QString& dir_) {
  if(dir_.isEmpty()) {
    return QStringList();
  }

  // TODO: build in symlink checking, for now, prohibit
  QDir dir(dir_, QString(), QDir::Name | QDir::IgnoreCase, QDir::Dirs | QDir::Readable | QDir::NoSymLinks);

  QStringList allSubdirs; // the whole list

  // find immediate sub directories
  const QStringList subdirs = dir.entryList();
  for(QStringList::ConstIterator subdir = subdirs.begin(); subdir != subdirs.end(); ++subdir) {
    if((*subdir).isEmpty() || *subdir == QLatin1String(".") || *subdir == QLatin1String("..")) {
      continue;
    }
    QString absSubdir = dir.absoluteFilePath(*subdir);
    allSubdirs += findAllSubDirs(absSubdir);
    allSubdirs += absSubdir;
  }
  return allSubdirs;
}

KLibrary* Tellico::openLibrary(const QString& libName_) {
  KLibrary* library = new KLibrary(libName_);
  if(!library->load()) {
//    myWarning() << "Could not load library'" << libName_ << "'";
//    myWarning() << "ERROR:" << library->errorString();
    return 0;
  }

  return library;
}

QString Tellico::saveLocation(const QString& dir_) {
  return KGlobal::dirs()->saveLocation("appdata", dir_, true);
}
