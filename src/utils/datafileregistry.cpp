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

#include "datafileregistry.h"
#include "../tellico_debug.h"

#include <QFileInfo>
#include <QStandardPaths>

using Tellico::DataFileRegistry;

Tellico::DataFileRegistry* DataFileRegistry::self() {
  static DataFileRegistry registry;
  return &registry;
}

DataFileRegistry::DataFileRegistry() = default;

void DataFileRegistry::addDataLocation(const QString& dir_, bool prepend_) {
  if(dir_.isEmpty()) {
    return;
  }

  QFileInfo info(dir_);
  if(!info.exists()) {
    return;
  }

  QString dataDir;
  if(info.isDir()) {
    dataDir = dir_;
  } else {
    dataDir = info.canonicalPath();
  }
  if(!dataDir.endsWith(QLatin1Char('/'))) {
    dataDir += QLatin1Char('/');
  }

  if(!m_dataLocations.contains(dataDir)) {
    if(prepend_) {
      m_dataLocations.prepend(dataDir);
    } else {
      m_dataLocations.append(dataDir);
    }
  }
}

QString DataFileRegistry::locate(const QString& fileName_) {
  // account for absolute paths
  if(QFileInfo(fileName_).isAbsolute()) {
    return fileName_;
  }
  for(const auto& dataDir : std::as_const(m_dataLocations)) {
    if(QFileInfo::exists(dataDir + fileName_)) {
      return dataDir + fileName_;
    }
  }
  return QStandardPaths::locate(QStandardPaths::AppDataLocation, fileName_);
}
