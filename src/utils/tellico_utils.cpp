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

#include <config.h> // for TELLICO_VERSION

#include "tellico_utils.h"
#include "string_utils.h"
#include "../tellico_debug.h"

#include <KIO/FileCopyJob>

#include <QIcon>
#include <QStandardPaths>
#include <QDir>
#include <QPixmap>
#include <QDateTime>
#include <QUrl>
#include <QSet>
#include <QDialog>

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

QStringList Tellico::locateAllFiles(const QString& fileName_) {
  QStringList files;
  QSet<QString> uniqueNames;

  QString dirPart = fileName_.section(QLatin1Char('/'), 0, -2);
  QString filePart = fileName_.section(QLatin1Char('/'), -1);

  const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, dirPart, QStandardPaths::LocateDirectory);
  foreach(const QString& dir, dirs) {
    const QStringList fileNames = QDir(dir).entryList(QStringList() << filePart);
    foreach(const QString& file, fileNames) {
      if(!uniqueNames.contains(file)) {
        files.append(dir + QLatin1Char('/') + file);
        uniqueNames += file;
      }
    }
  }
  return files;
}

QString Tellico::installationDir() {
  // look for a file that gets installed to know the installation directory
  static QString appdir;
  if(appdir.isEmpty()) {
    appdir = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("tellico/pics/tellico.png"));
    // remove the file name string. Important to keep trailing slash
    appdir.chop(QStringLiteral("pics/tellico.png").length());
//    myDebug() << "InstallationDir:" << appdir;
  }
  return appdir;
}

QString Tellico::saveLocation(const QString& dir_) {
  QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator() + dir_;
  QDir dir;
  bool success = dir.mkpath(path);
  if(!success) {
    myWarning() << "Failed to make path:" << path;
  }
  return path;
}

const QPixmap& Tellico::pixmap(const QString& value_) {
  static QHash<int, QPixmap*> pixmaps;
  if(pixmaps.isEmpty()) {
    pixmaps.insert(-1, new QPixmap());
  }
  bool ok;
  int n = Tellico::toUInt(value_, &ok);
  if(!ok || n < 1 || n > 10) {
    return *pixmaps[-1];
  }
  if(pixmaps[n]) {
    return *pixmaps[n];
  }

  QString picName = QStringLiteral(":/icons/stars%1").arg(n);
  QPixmap* pix = new QPixmap(QIcon(picName).pixmap(QSize(n*16, 16)));
  pixmaps.insert(n, pix);
  return *pix;
}

bool Tellico::checkCommonXSLFile() {
  // look for a file that gets installed to know the installation directory
  // need to check timestamps
  QString userDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QString userCommonFile = userDataDir + QDir::separator() + QLatin1String("tellico-common.xsl");
  if(QFile::exists(userCommonFile)) {
    // check timestamps
    // pics/tellico.png is not likely to be in a user directory
    QString installDir = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("pics/tellico.png"));
    installDir = QFileInfo(installDir).absolutePath();
    QString installCommonFile = installDir + QDir::separator() + QLatin1String("tellico-common.xsl");
    if(userCommonFile == installCommonFile) {
      myWarning() << "install location is same as user location";
    }
    QFileInfo installInfo(installCommonFile);
    QFileInfo userInfo(userCommonFile);
    if(installInfo.lastModified() > userInfo.lastModified()) {
      // the installed file has been modified more recently than the user's
      // remove user's tellico-common.xsl file so it gets copied again
//      myLog() << "removing" << userCommonFile;
//      myLog() << "copying" << installCommonFile;
      QFile::remove(userCommonFile);
    } else {
      // we're done
      return true;
    }
  }
  QUrl src = QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("tellico-common.xsl")));
  if(src.isEmpty()) {
    myDebug() << "checkCommonXSLFile() - could not locate tellico-common.xsl";
    return false;
  }
  QUrl dest = QUrl::fromLocalFile(userCommonFile);
  return KIO::file_copy(src, dest)->exec();
}

void Tellico::activateDialog(QDialog* dlg_) {
  Q_ASSERT(dlg_);
  dlg_->show();
  dlg_->raise();
  dlg_->activateWindow();
}

void Tellico::addUserAgent(KIO::Job* job_) {
  job_->addMetaData(QLatin1StringView("SendUserAgent"), QLatin1String("true"));
  job_->addMetaData(QLatin1StringView("UserAgent"),
                    QStringLiteral("Tellico/%1 ( https://tellico-project.org )").arg(QStringLiteral(TELLICO_VERSION)));
}
