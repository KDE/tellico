/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>
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

#include "manager.h"
#include "newstuffadaptor.h"
#include "../utils/cursorsaver.h"
#include "../utils/tellico_utils.h"
#include "../tellico_debug.h"

#include <KTar>
#include <KConfig>
#include <KFileItem>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KDesktopFile>
#include <KIO/FileCopyJob>
#include <KIO/DeleteJob>

#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QGlobalStatic>

#include <sys/types.h>
#include <sys/stat.h>
// for msvc
#ifndef S_IXUSR
#define S_IXUSR 00100
#endif

namespace Tellico {
  namespace NewStuff {

class ManagerSingleton {
public:
  Tellico::NewStuff::Manager self;
};

  }
}

Q_GLOBAL_STATIC(Tellico::NewStuff::ManagerSingleton, s_instance)

using Tellico::NewStuff::Manager;

Manager::Manager() : QObject(nullptr) {
  QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.tellico"));
  new NewstuffAdaptor(this);
  QDBusConnection::sessionBus().registerObject(QStringLiteral("/NewStuff"), this);
}

Manager::~Manager() {
  QDBusConnection::sessionBus().unregisterObject(QStringLiteral("/NewStuff"));
  auto interface = QDBusConnection::sessionBus().interface();
  if(interface) {
    // the windows build was crashing here when the interface was null
    // see https://bugs.kde.org/show_bug.cgi?id=422468
    interface->unregisterService(QStringLiteral("org.kde.tellico"));
  }
}

Manager* Manager::self() {
  return &s_instance->self;
}

bool Manager::installTemplate(const QString& file_) {
  if(file_.isEmpty()) {
    return false;
  }
  GUI::CursorSaver cs;

  QString xslFile;
  QStringList allFiles;

  bool success = true;

  static const QRegularExpression digitsDashRx(QLatin1String("^\\d+-"));
  // is there a better way to figure out if the url points to a XSL file or a tar archive
  // than just trying to open it?
  KTar archive(file_);
  if(archive.open(QIODevice::ReadOnly)) {
    const KArchiveDirectory* archiveDir = archive.directory();
    archiveDir->copyTo(Tellico::saveLocation(QStringLiteral("entry-templates/")));

    allFiles = archiveFiles(archiveDir);
    // remember files installed for template
    xslFile = findXSL(archiveDir);
  } else { // assume it's an xsl file
    QString name = QFileInfo(file_).fileName();
    if(!name.endsWith(QLatin1String(".xsl"))) {
      name += QLatin1String(".xsl");
    }
    name.remove(digitsDashRx); // Remove possible kde-files.org id
    name = Tellico::saveLocation(QStringLiteral("entry-templates/")) + name;
    // Should overwrite since we might be upgrading
    if(QFile::exists(name)) {
      QFile::remove(name);
    }
    auto job = KIO::file_copy(QUrl::fromLocalFile(file_), QUrl::fromLocalFile(name));
    if(job->exec()) {
      xslFile = QFileInfo(name).fileName();
      allFiles << xslFile;
    }
  }

  if(xslFile.isEmpty()) {
    success = false;
  } else {
    KConfigGroup config(KSharedConfig::openConfig(), QLatin1String("KNewStuffFiles"));
    config.writeEntry(file_, allFiles);
    config.writeEntry(xslFile, file_);
  }
  Tellico::checkCommonXSLFile();
  return success;
}

QMap<QString, QString> Manager::userTemplates() {
  QDir dir(Tellico::saveLocation(QStringLiteral("entry-templates/")));
  dir.setNameFilters(QStringList() << QStringLiteral("*.xsl"));
  dir.setFilter(QDir::Files | QDir::Writable);
  QStringList files = dir.entryList();
  QMap<QString, QString> nameFileMap;
  foreach(const QString& file, files) {
    QString name = file;
    name.truncate(file.length()-4); // remove ".xsl"
    name.replace(QLatin1Char('_'), QLatin1Char(' '));
    nameFileMap.insert(name, file);
  }
  return nameFileMap;
}

bool Manager::removeTemplateByName(const QString& name_) {
  if(name_.isEmpty()) {
    return false;
  }

  QString xslFile = userTemplates().value(name_);
  if(!xslFile.isEmpty()) {
    KConfigGroup config(KSharedConfig::openConfig(), QLatin1String("KNewStuffFiles"));
    QString file = config.readEntry(xslFile, QString());
    if(!file.isEmpty()) {
      return removeTemplate(file);
    }
    // At least remove xsl file
    QFile::remove(Tellico::saveLocation(QStringLiteral("entry-templates/")) + xslFile);
    return true;
  }
  return false;
}

bool Manager::removeTemplate(const QString& file_) {
  if(file_.isEmpty()) {
    return false;
  }
  GUI::CursorSaver cs;

  KConfigGroup fileGroup(KSharedConfig::openConfig(), QLatin1String("KNewStuffFiles"));
  QStringList files = fileGroup.readEntry(file_, QStringList());

  if(files.isEmpty()) {
    myWarning() << "No file list found for" << file_;
    return false;
  }

  bool success = true;
  QString path = Tellico::saveLocation(QStringLiteral("entry-templates/"));
  foreach(const QString& file, files) {
    if(file.endsWith(QDir::separator())) {
      // ok to not delete all directories
      QDir().rmdir(path + file);
    } else {
      success = QFile::remove(path + file) && success;
      if(!success) {
        myDebug() << "Failed to remove" << (path+file);
      }
    }
  }

  // remove config entries even if unsuccessful
  fileGroup.deleteEntry(file_);
  QString key = fileGroup.entryMap().key(file_);
  fileGroup.deleteEntry(key);
  KSharedConfig::openConfig()->sync();
  return success;
}

bool Manager::installScript(const QString& file_) {
  if(file_.isEmpty()) {
    return false;
  }
  GUI::CursorSaver cs;

  QString realFile = file_;

  KTar archive(file_);
  QString copyTarget = Tellico::saveLocation(QStringLiteral("data-sources/"));
  QString scriptFolder;
  QString exeFile;
  QString sourceName;

  static const QRegularExpression digitsDashRx(QLatin1String("^\\d+-"));
  if(archive.open(QIODevice::ReadOnly)) {
    const KArchiveDirectory* archiveDir = archive.directory();
    exeFile = findEXE(archiveDir);
    if(exeFile.isEmpty()) {
      myDebug() << "No exe file found";
      return false;
    }
    sourceName = QFileInfo(exeFile).baseName();
    if(sourceName.isEmpty()) {
      myDebug() << "Invalid packet name";
      return false;
    }
    // package could have a top-level directory or not
    // it should have a directory...
    foreach(const QString& entry, archiveDir->entries()) {
      if(entry.indexOf(QDir::separator()) < 0) {
        // archive does have multiple root items -> extract own dir
        copyTarget += sourceName;
        scriptFolder = copyTarget + QDir::separator();
        break;
      }
    }
    if(scriptFolder.isEmpty()) { // one root item
      scriptFolder = copyTarget + exeFile.left(exeFile.indexOf(QDir::separator())) + QDir::separator();
    }
    // overwrites stuff there
    archiveDir->copyTo(copyTarget);
  } else { // assume it's an script file
    exeFile = QFileInfo(file_).fileName();

    exeFile.remove(digitsDashRx); // Remove possible kde-files.org id
    sourceName = QFileInfo(exeFile).completeBaseName();
    if(sourceName.isEmpty()) {
      myDebug() << "Invalid packet name";
      return false;
    }
    copyTarget += sourceName;
    scriptFolder = copyTarget + QDir::separator();
    QDir().mkpath(scriptFolder);
    auto job = KIO::file_copy(QUrl::fromLocalFile(file_), QUrl::fromLocalFile(scriptFolder + exeFile));
    if(!job->exec()) {
      myDebug() << "Copy failed";
      return false;
    }
    realFile = exeFile;
  }

  QString specFile = scriptFolder + QFileInfo(exeFile).completeBaseName() + QLatin1String(".spec");
  QString sourceExec = scriptFolder + exeFile;
  QUrl dest = QUrl::fromLocalFile(sourceExec);
  KFileItem item(dest);
  item.setDelayedMimeTypes(true);
  int out = ::chmod(QFile::encodeName(dest.path()).constData(), item.permissions() | S_IXUSR);
  if(out != 0) {
    myDebug() << "Failed to set permissions for" << dest.path();
  }

  KDesktopFile df(specFile);
  KConfigGroup cg = df.desktopGroup();
  // update name
  sourceName = cg.readEntry("Name", sourceName);
  cg.writeEntry("ExecPath", sourceExec);
  cg.writeEntry("NewStuffName", sourceName);
  cg.writeEntry("DeleteOnRemove", true);

  KConfigGroup config(KSharedConfig::openConfig(), QLatin1String("KNewStuffFiles"));
  config.writeEntry(sourceName, realFile);
  config.writeEntry(realFile, scriptFolder);
  //  myDebug() << "exeFile = " << exeFile;
  //  myDebug() << "sourceExec = " << info->sourceExec;
  //  myDebug() << "sourceName = " << info->sourceName;
  //  myDebug() << "specFile = " << info->specFile;
  KConfigGroup configGroup(KSharedConfig::openConfig(), QStringLiteral("Data Sources"));
  int nSources = configGroup.readEntry("Sources Count", 0);
  config.writeEntry(sourceName + QLatin1String("_nbr"), nSources);
  configGroup.writeEntry("Sources Count", nSources + 1);
  KConfigGroup sourceGroup(KSharedConfig::openConfig(), QStringLiteral("Data Source %1").arg(nSources));
  sourceGroup.writeEntry("Name", sourceName);
  sourceGroup.writeEntry("ExecPath", sourceExec);
  sourceGroup.writeEntry("DeleteOnRemove", true);
  sourceGroup.writeEntry("Type", 5);
  KSharedConfig::openConfig()->sync();
  return true;
}

bool Manager::removeScriptByName(const QString& name_) {
  if(name_.isEmpty()) {
    return false;
  }

  KConfigGroup config(KSharedConfig::openConfig(), QLatin1String("KNewStuffFiles"));
  QString file = config.readEntry(name_, QString());
  if(!file.isEmpty()) {
    return removeScript(file);
  }
  return false;
}

bool Manager::removeScript(const QString& file_) {
  if(file_.isEmpty()) {
    return false;
  }
  GUI::CursorSaver cs;

  QFileInfo fi(file_);
  const QString realFile = fi.fileName();
  const QString sourceName = fi.completeBaseName();

  bool success = true;
  KConfigGroup fileGroup(KSharedConfig::openConfig(), QLatin1String("KNewStuffFiles"));
  QString scriptFolder = fileGroup.readEntry(file_, QString());
  if(scriptFolder.isEmpty()) {
    scriptFolder = fileGroup.readEntry(realFile, QString());
  }
  int source = fileGroup.readEntry(file_ + QLatin1String("_nbr"), -1);
  if(source == -1) {
    source = fileGroup.readEntry(sourceName + QLatin1String("_nbr"), -1);
  }

  if(!scriptFolder.isEmpty()) {
    KIO::del(QUrl::fromLocalFile(scriptFolder))->exec();
  }
  if(source != -1) {
    KConfigGroup configGroup(KSharedConfig::openConfig(), QStringLiteral("Data Sources"));
    int nSources = configGroup.readEntry("Sources Count", 0);
    configGroup.writeEntry("Sources Count", nSources - 1);
    KConfigGroup sourceGroup(KSharedConfig::openConfig(), QStringLiteral("Data Source %1").arg(source));
    sourceGroup.deleteGroup();
  }

  // remove config entries even if unsuccessful
  fileGroup.deleteEntry(file_);
  QString key = fileGroup.entryMap().key(file_);
  if(!key.isEmpty()) fileGroup.deleteEntry(key);
  fileGroup.deleteEntry(realFile);
  key = fileGroup.entryMap().key(realFile);
  if(!key.isEmpty()) fileGroup.deleteEntry(key);
  fileGroup.deleteEntry(file_ + QLatin1String("_nbr"));
  fileGroup.deleteEntry(sourceName + QLatin1String("_nbr"));
  KSharedConfig::openConfig()->sync();
  return success;
}

QStringList Manager::archiveFiles(const KArchiveDirectory* dir_, const QString& path_) {
  QStringList list;

  foreach(const QString& entry, dir_->entries()) {
    const KArchiveEntry* curEntry = dir_->entry(entry);
    if(curEntry->isFile()) {
      list += path_ + entry;
    } else if(curEntry->isDirectory()) {
      list += archiveFiles(static_cast<const KArchiveDirectory*>(curEntry), path_ + entry + QDir::separator());
      // add directory AFTER contents, since we delete from the top down later
      list += path_ + entry + QDir::separator();
    }
  }

  return list;
}

// don't recurse, the .xsl must be in top directory
QString Manager::findXSL(const KArchiveDirectory* dir_) {
  foreach(const QString& entry, dir_->entries()) {
    if(entry.endsWith(QLatin1String(".xsl"))) {
      return entry;
    }
  }
  return QString();
}

QString Manager::findEXE(const KArchiveDirectory* dir_) {
  QStack<const KArchiveDirectory*> dirStack;
  QStack<QString> dirNameStack;

  dirStack.push(dir_);
  dirNameStack.push(QString());

  do {
    const QString dirName = dirNameStack.pop();
    const KArchiveDirectory* curDir = dirStack.pop();
    foreach(const QString& entry, curDir->entries()) {
      const KArchiveEntry* archEntry = curDir->entry(entry);

      if(archEntry->isFile() && (archEntry->permissions() & S_IEXEC)) {
        return dirName + entry;
      } else if(archEntry->isDirectory()) {
        dirStack.push(static_cast<const KArchiveDirectory*>(archEntry));
        dirNameStack.push(dirName + entry + QDir::separator());
      }
    }
  } while(!dirStack.isEmpty());

  return QString();
}
