/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "manager.h"
#include "newscript.h"
#include "../filehandler.h"
#include "../tellico_debug.h"
#include "../tellico_utils.h"
#include "../tellico_kernel.h"
#include "../fetch/fetch.h"

#include <kurl.h>
#include <ktar.h>
#include <kglobal.h>
#include <kio/netaccess.h>
#include <kconfig.h>
#include <ktempfile.h>
#include <kio/job.h>
#include <kfileitem.h>
#include <kdeversion.h>
#include <knewstuff/entry.h>
#include <kstandarddirs.h>

#include <qfileinfo.h>
#include <qdir.h>
#include <qptrstack.h>
#include <qvaluestack.h>
#include <qwidget.h>

#include <sys/types.h>
#include <sys/stat.h>

using Tellico::NewStuff::Manager;

Manager::Manager(QObject* parent_) : QObject(parent_), m_tempFile(0) {
  m_infoList.setAutoDelete(true);
}

Manager::~Manager() {
  delete m_tempFile;
  m_tempFile = 0;
}

bool Manager::installTemplate(const KURL& url_, const QString& entryName_) {
  FileHandler::FileRef* ref = FileHandler::fileRef(url_);

  QString xslFile;
  QStringList allFiles;

  bool success = true;

  // is there a better way to figure out if the url points to a XSL file or a tar archive
  // than just trying to open it?
  KTar archive(ref->fileName());
  if(archive.open(IO_ReadOnly)) {
    const KArchiveDirectory* archiveDir = archive.directory();
    archiveDir->copyTo(Tellico::saveLocation(QString::fromLatin1("entry-templates/")));

    allFiles = archiveFiles(archiveDir);
    // remember files installed for template
    xslFile = findXSL(archiveDir);
  } else { // assume it's an xsl file
    QString name = entryName_;
    if(name.isEmpty()) {
      name = url_.fileName();
    }
    if(!name.endsWith(QString::fromLatin1(".xsl"))) {
      name += QString::fromLatin1(".xsl");
    }
    KURL dest;
    dest.setPath(Tellico::saveLocation(QString::fromLatin1("entry-templates/")) + name);
    success = true;
    if(QFile::exists(dest.path())) {
      myDebug() << "Manager::installTemplate() - " << dest.path() << " exists!" << endl;
      success = false;
    } else if(KIO::NetAccess::file_copy(url_, dest)) {
      xslFile = dest.fileName();
      allFiles += xslFile;
    }
  }

  if(xslFile.isEmpty()) {
    success = false;
  } else {
    // remove ".xsl"
    xslFile.truncate(xslFile.length()-4);
    KConfigGroup config(KGlobal::config(), "KNewStuffFiles");
    config.writeEntry(xslFile, allFiles);
    m_urlNameMap.insert(url_, xslFile);
  }

  checkCommonFile();

  delete ref;
  return success;
}

bool Manager::removeTemplate(const QString& name_) {
  KConfigGroup fileGroup(KGlobal::config(), "KNewStuffFiles");
  QStringList files = fileGroup.readListEntry(name_);
  // at least, delete xsl file
  if(files.isEmpty()) {
    kdWarning() << "Manager::deleteTemplate()  no file list found for " << name_ << endl;
    files += name_;
  }

  bool success = true;
  QString path = Tellico::saveLocation(QString::fromLatin1("entry-templates/"));
  for(QStringList::ConstIterator it = files.begin(); it != files.end(); ++it) {
    if((*it).endsWith(QChar('/'))) {
      // ok to not delete all directories
      QDir().rmdir(path + *it);
    } else {
      success = success && QFile(path + *it).remove();
    }
  }

  if(success) {
    fileGroup.deleteEntry(name_);
    KConfigGroup statusGroup(KGlobal::config(), "KNewStuffStatus");
    QString entryName = statusGroup.readEntry(name_);
    statusGroup.deleteEntry(name_);
    statusGroup.deleteEntry(entryName);
  }

  return success;
}

bool Manager::installScript(const KURL& url_) {
  FileHandler::FileRef* ref = FileHandler::fileRef(url_);

  KTar archive(ref->fileName());
  if(!archive.open(IO_ReadOnly)) {
    myDebug() << "Manager::installScript() - can't open tar file" << endl;
    return false;
  }

  const KArchiveDirectory* archiveDir = archive.directory();

  QString exeFile = findEXE(archiveDir);
  if(exeFile.isEmpty()) {
    myDebug() << "Manager::installScript() - no exe file found" << endl;
    return false;
  }

  QFileInfo exeInfo(exeFile);
  DataSourceInfo* info = new DataSourceInfo();

  QString copyTarget = Tellico::saveLocation(QString::fromLatin1("data-sources/"));
  QString scriptFolder;

  // package could have a top-level directory or not
  // it should have a directory...
  const KArchiveEntry* firstEntry = archiveDir->entry(archiveDir->entries().first());
  if(firstEntry->isFile()) {
    copyTarget += exeInfo.baseName() + '/';
    if(QFile::exists(copyTarget)) {
      KURL u;
      u.setPath(scriptFolder);
      myDebug() << "Manager::installScript() - deleting " << scriptFolder << endl;
      KIO::NetAccess::del(u, Kernel::self()->widget());
      info->isUpdate = true;
    }
    scriptFolder = copyTarget;
  } else {
    scriptFolder = copyTarget + firstEntry->name() + '/';
    if(QFile::exists(copyTarget + exeFile)) {
      info->isUpdate = true;
    }
  }

  // overwrites stuff there
  archiveDir->copyTo(copyTarget);

  info->specFile = scriptFolder + exeInfo.baseName() + ".spec";
  if(!QFile::exists(info->specFile)) {
    myDebug() << "Manager::installScript() - no spec file for script! " << info->specFile << endl;
    delete info;
    return false;
  }
  info->sourceName = exeFile;
  info->sourceExec = copyTarget + exeFile;
  m_infoList.append(info);
  m_urlNameMap.insert(url_, exeFile);

  KURL dest;
  dest.setPath(info->sourceExec);
  KFileItem item(KFileItem::Unknown, KFileItem::Unknown, dest, true);
  ::chmod(QFile::encodeName(dest.path()), item.permissions() | S_IXUSR);

  {
    KConfig spec(info->specFile, false, false);
    // update name
    info->sourceName = spec.readEntry("Name", info->sourceName);
    spec.writePathEntry("ExecPath", info->sourceExec);
    spec.writeEntry("NewStuffName", info->sourceName);
    spec.writeEntry("DeleteOnRemove", true);
  }

  {
    KConfigGroup config(KGlobal::config(), "KNewStuffFiles");
    config.writeEntry(info->sourceName, archiveFiles(archiveDir));
  }


//  myDebug() << "Manager::installScript() - exeFile = " << exeFile << endl;
//  myDebug() << "Manager::installScript() - sourceExec = " << info->sourceExec << endl;
//  myDebug() << "Manager::installScript() - sourceName = " << info->sourceName << endl;
//  myDebug() << "Manager::installScript() - specFile = " << info->specFile << endl;

  delete ref;
  return true;
}

bool Manager::removeScript(const QString& name_) {
  KConfigGroup fileGroup(KGlobal::config(), "KNewStuffFiles");
  QStringList files = fileGroup.readListEntry(name_);

  bool success = true;
  QString path = Tellico::saveLocation(QString::fromLatin1("data-sources/"));
  for(QStringList::ConstIterator it = files.begin(); it != files.end(); ++it) {
    if((*it).endsWith(QChar('/'))) {
      // ok to not delete all directories
      QDir().rmdir(path + *it);
    } else {
      success = success && QFile(path + *it).remove();
    }
  }

  if(success) {
    fileGroup.deleteEntry(name_);
    KConfigGroup statusGroup(KGlobal::config(), "KNewStuffStatus");
    QString entryName = statusGroup.readEntry(name_);
    statusGroup.deleteEntry(name_);
    statusGroup.deleteEntry(entryName);
  }

  return success;
}

Tellico::NewStuff::InstallStatus Manager::installStatus(KNS::Entry* entry_) {
  KConfigGroup config(KGlobal::config(), "KNewStuffStatus");
  QString datestring = config.readEntry(entry_->name());
  if(datestring.isEmpty()) {
    return NotInstalled;
  }

  QDate date = QDate::fromString(datestring, Qt::ISODate);
  if(!date.isValid()) {
    return NotInstalled;
  }
  if(date < entry_->releaseDate()) {
    return OldVersion;
  }
  return Current;
}

QStringList Manager::archiveFiles(const KArchiveDirectory* dir_, const QString& path_) {
  QStringList list;

  const QStringList dirEntries = dir_->entries();
  for(QStringList::ConstIterator it = dirEntries.begin(); it != dirEntries.end(); ++it) {
    const QString& entry = *it;
    const KArchiveEntry* curEntry = dir_->entry(entry);
    if(curEntry->isFile()) {
      list += path_ + entry;
    } else if(curEntry->isDirectory()) {
      list += archiveFiles(static_cast<const KArchiveDirectory*>(curEntry), path_ + entry + '/');
      // add directory AFTER contents, since we delete from the top down later
      list += path_ + entry + '/';
    }
  }

  return list;
}

// don't recurse, the .xsl must be in top directory
QString Manager::findXSL(const KArchiveDirectory* dir_) {
  const QStringList entries = dir_->entries();
  for(QStringList::ConstIterator it = entries.begin(); it != entries.end(); ++it) {
    const QString& entry = *it;
    if(entry.endsWith(QString::fromLatin1(".xsl"))) {
      return entry;
    }
  }
  return QString();
}

QString Manager::findEXE(const KArchiveDirectory* dir_) {
  QPtrStack<KArchiveDirectory> dirStack;
  QValueStack<QString> dirNameStack;

  dirStack.push(dir_);
  dirNameStack.push(QString());

  do {
    const QString dirName = dirNameStack.pop();
    const KArchiveDirectory* curDir = dirStack.pop();
    const QStringList entries = curDir->entries();
    for(QStringList::ConstIterator it = entries.begin(); it != entries.end(); ++it) {
      const QString& entry = *it;
      const KArchiveEntry* archEntry = curDir->entry(entry);

      if(archEntry->isFile() && (archEntry->permissions() & S_IEXEC)) {
        return dirName + entry;
      } else if(archEntry->isDirectory()) {
        dirStack.push(static_cast<const KArchiveDirectory*>(archEntry));
        dirNameStack.push(dirName + entry + '/');
      }
    }
  } while(!dirStack.isEmpty());

  return QString();
}

void Manager::install(DataType type_, KNS::Entry* entry_) {
  if(m_tempFile) {
    delete m_tempFile;
  }
  m_tempFile = new KTempFile();
  m_tempFile->setAutoDelete(true);

  KURL destination;
  destination.setPath(m_tempFile->name());
  KIO::FileCopyJob* job = KIO::file_copy(entry_->payload(), destination, -1, true);
  connect(job, SIGNAL(result(KIO::Job*)), SLOT(slotDownloadJobResult(KIO::Job*)));
  m_jobMap.insert(job, EntryPair(entry_, type_));
}

void Manager::slotDownloadJobResult(KIO::Job* job_) {
  KIO::FileCopyJob* job = static_cast<KIO::FileCopyJob*>(job_);
  if(job->error()) {
    delete m_tempFile;
    m_tempFile = 0;
    job->showErrorDialog(Kernel::self()->widget());
    return;
  }

  KNS::Entry* entry = m_jobMap[job_].first;
  DataType type = m_jobMap[job_].second;

  bool success;
  if(type == EntryTemplate) {
    success = installTemplate(job->destURL(), entry->name());
  } else {
    // needed so the GPG signature can be checked
    NewScript* newScript = new NewScript(this, Kernel::self()->widget());
    success = newScript->install(job->destURL().path());
  }

  if(success) {
    const QString name = m_urlNameMap[job->destURL()];
    KConfigGroup config(KGlobal::config(), "KNewStuffStatus");
    // have to keep a config entry that maps the name of the file to the name
    // of the newstuff entry, so we can track which entries are installed
    config.writeEntry(name, entry->name());
    config.writeEntry(entry->name(), entry->releaseDate().toString(Qt::ISODate));
    config.sync();
    emit signalInstalled(entry);
  } else {
    kdWarning() << "Manager::slotDownloadJobResult() - Failed to install" << endl;
  }
  delete m_tempFile;
  m_tempFile = 0;
}

bool Manager::checkCommonFile() {
  // look for a file that gets installed to know the installation directory
  // need to check timestamps
  QString userDataDir = Tellico::saveLocation(QString::null);
  QString userCommonFile = userDataDir + '/' + QString::fromLatin1("tellico-common.xsl");
  if(QFile::exists(userCommonFile)) {
    // check timestamps
    // pics/tellico.png is not likely to be in a user directory
    QString installDir = KGlobal::dirs()->findResourceDir("appdata", QString::fromLatin1("pics/tellico.png"));
    QString installCommonFile = installDir + '/' + QString::fromLatin1("tellico-common.xsl");
#ifndef NDEBUG
    if(userCommonFile == installCommonFile) {
      kdWarning() << "Manager::checkCommonFile() - install location is same as user location" << endl;
    }
#endif
    QFileInfo installInfo(installCommonFile);
    QFileInfo userInfo(userCommonFile);
    if(installInfo.lastModified() > userInfo.lastModified()) {
      // the installed file has been modified more recently than the user's
      // remove user's tellico-common.xsl file so it gets copied again
      myLog() << "Manager::checkCommonFile() - removing " << userCommonFile << endl;
      myLog() << "Manager::checkCommonFile() - copying  " << installCommonFile << endl;
      QFile::remove(userCommonFile);
    } else {
      return true;
    }
  }
  KURL src, dest;
  src.setPath(KGlobal::dirs()->findResource("appdata", QString::fromLatin1("tellico-common.xsl")));
  dest.setPath(userCommonFile);
  return KIO::NetAccess::file_copy(src, dest);
}

#include "manager.moc"
