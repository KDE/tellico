/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "filehandler.h"
#include "tellico_kernel.h"
#include "image.h"
#include "tellico_strings.h"
#include "tellico_utils.h"
#include "tellico_debug.h"
#include "core/netaccess.h"

#include <kurl.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>
#include <ktempfile.h>
#include <ksavefile.h>
#include <kapplication.h>
#include <kfileitem.h>
#include <kio/chmodjob.h>

#include <qdom.h>
#include <qfile.h>

using Tellico::FileHandler;

class FileHandler::ItemDeleter : public QObject {
public:
  ItemDeleter(KIO::Job* job, KFileItem* item) : QObject(), m_job(job), m_item(item) {
    FileHandler::s_deleterList.append(this);
    connect(job, SIGNAL(result(KIO::Job*)), SLOT(deleteLater()));
  }
  ~ItemDeleter() {
    FileHandler::s_deleterList.removeRef(this);
    if(m_job) m_job->kill();
    delete m_item; m_item = 0;
  }
private:
  QGuardedPtr<KIO::Job> m_job;
  KFileItem* m_item;
};

QPtrList<FileHandler::ItemDeleter> FileHandler::s_deleterList;

FileHandler::FileRef::FileRef(const KURL& url_, bool quiet_) : m_file(0), m_isValid(false) {
  if(url_.isEmpty()) {
    return;
  }

  if(!Tellico::NetAccess::download(url_, m_filename, Kernel::self()->widget())) {
    myDebug() << "FileRef::FileRef() - can't download " << url_ << endl;
    QString s = KIO::NetAccess::lastErrorString();
    if(!s.isEmpty()) {
      myDebug() << s << endl;
    }
    if(!quiet_) {
      Kernel::self()->sorry(i18n(errorLoad).arg(url_.fileName()));
    }
    return;
  }

  m_file = new QFile(m_filename);
  if(m_file->exists()) {
    m_isValid = true;
  } else {
    delete m_file;
    m_file = 0;
  }
}

FileHandler::FileRef::~FileRef() {
  if(!m_filename.isEmpty()) {
    Tellico::NetAccess::removeTempFile(m_filename);
  }
  if(m_file) {
    m_file->close();
  }
  delete m_file;
  m_file = 0;
  m_isValid = false;
}

bool FileHandler::FileRef::open(bool quiet_) {
  if(!isValid()) {
    return false;
  }
  if(!m_file || !m_file->open(IO_ReadOnly)) {
    if(!quiet_) {
      KURL u;
      u.setPath(fileName());
      Kernel::self()->sorry(i18n(errorLoad).arg(u.fileName()));
    }
    delete m_file;
    m_file = 0;
    m_isValid = false;
    return false;
  }
  return true;
}

FileHandler::FileRef* FileHandler::fileRef(const KURL& url_, bool quiet_) {
  return new FileRef(url_, quiet_);
}

QString FileHandler::readTextFile(const KURL& url_, bool quiet_/*=false*/) {
  FileRef f(url_, quiet_);
  if(!f.isValid()) {
    return QString::null;
  }

  f.open(quiet_);
  QTextStream stream(f.file());
  return stream.read();
}

QDomDocument FileHandler::readXMLFile(const KURL& url_, bool processNamespace_, bool quiet_) {
  FileRef f(url_, quiet_);
  if(!f.isValid()) {
    return QDomDocument();
  }

  QDomDocument doc;
  QString errorMsg;
  int errorLine, errorColumn;
  f.open(quiet_);
  if(!doc.setContent(f.file(), processNamespace_, &errorMsg, &errorLine, &errorColumn)) {
    if(!quiet_) {
      QString details = i18n("There is an XML parsing error in line %1, column %2.").arg(errorLine).arg(errorColumn);
      details += QString::fromLatin1("\n");
      details += i18n("The error message from Qt is:");
      details += QString::fromLatin1("\n\t") + errorMsg;
      GUI::CursorSaver cs(Qt::arrowCursor);
      KMessageBox::detailedSorry(Kernel::self()->widget(), i18n(errorLoad).arg(url_.fileName()), details);
    }
    return QDomDocument();
  }
  return doc;
}

QByteArray FileHandler::readDataFile(const KURL& url_, bool quiet_) {
  FileRef f(url_, quiet_);
  if(!f.isValid()) {
    return QByteArray();
  }

  f.open(quiet_);
  return f.file()->readAll();
}

Tellico::Data::Image* FileHandler::readImageFile(const KURL& url_, bool quiet_, const KURL& referrer_) {
  if(url_.isLocalFile()) {
    return readImageFile(url_, quiet_);
  }

  KTempFile tmpFile;
  KURL tmpURL;
  tmpURL.setPath(tmpFile.name());
  tmpFile.setAutoDelete(true);

  KIO::Job* job = KIO::file_copy(url_, tmpURL, -1, true /* overwrite */);
  job->addMetaData(QString::fromLatin1("referrer"), referrer_.url());

  if(!KIO::NetAccess::synchronousRun(job, Kernel::self()->widget())) {
    if(!quiet_) {
      Kernel::self()->sorry(i18n(errorLoad).arg(url_.fileName()));
    }
    return 0;
  }
  return readImageFile(tmpURL, quiet_);
}

Tellico::Data::Image* FileHandler::readImageFile(const KURL& url_, bool quiet_) {
  FileRef f(url_, quiet_);
  if(!f.isValid()) {
    return 0;
  }

  Data::Image* img = new Data::Image(f.fileName());
  if(img->isNull() && !quiet_) {
    QString str = i18n("Tellico is unable to load the image - %1.").arg(url_.fileName());
    Kernel::self()->sorry(str);
  }
  return img;
}

bool FileHandler::queryExists(const KURL& url_) {
  if(!KIO::NetAccess::exists(url_, false, Kernel::self()->widget())) {
    return true;
  }

  // we always overwrite the current URL without asking
  if(url_ != Kernel::self()->URL()) {
    GUI::CursorSaver cs(Qt::arrowCursor);
    QString str = i18n("A file named \"%1\" already exists. "
                       "Are you sure you want to overwrite it?").arg(url_.fileName());
    int want_continue = KMessageBox::warningContinueCancel(Kernel::self()->widget(), str,
                                                           i18n("Overwrite File?"),
                                                           i18n("Overwrite"));

    if(want_continue == KMessageBox::Cancel) {
      return false;
    }
  }

  KURL backup(url_);
  backup.setPath(backup.path() + QString::fromLatin1("~"));

  bool success = true;
  if(url_.isLocalFile()) {
    // KSaveFile messes up users and groups
    // the user is always reset to the current user
    KFileItemList list;
    int perm = -1;
    QString grp;
    if(KIO::NetAccess::exists(url_, false, Kernel::self()->widget())) {
      KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url_, true);
      perm = item.permissions();
      grp = item.group();

      KFileItem* backupItem = new KFileItem(KFileItem::Unknown, KFileItem::Unknown, backup, true);
      list.append(backupItem);
    }

    success = KSaveFile::backupFile(url_.path());
    if(!list.isEmpty()) {
      // have to leave the user alone
      KIO::Job* job = KIO::chmod(list, perm, 0, QString(), grp, true, false);
      new ItemDeleter(job, list.first());
    }
  } else {
    KIO::NetAccess::del(backup, Kernel::self()->widget()); // might fail if backup doesn't exist, that's ok
    success = KIO::NetAccess::file_copy(url_, backup, -1 /* permissions */, true /* overwrite */,
                                        false /* resume */, Kernel::self()->widget());
  }
  if(!success) {
    Kernel::self()->sorry(i18n(errorWrite).arg(url_.fileName() + '~'));
  }
  return success;
}

bool FileHandler::writeTextURL(const KURL& url_, const QString& text_, bool encodeUTF8_, bool force_, bool quiet_) {
  if((!force_ && !queryExists(url_)) || text_.isNull()) {
    if(text_.isNull()) {
      myDebug() << "FileHandler::writeTextURL() - null string for " << url_ << endl;
    }
    return false;
  }

  if(url_.isLocalFile()) {
    // KSaveFile messes up users and groups
    // the user is always reset to the current user
    KFileItemList list;
    int perm = -1;
    QString grp;
    if(KIO::NetAccess::exists(url_, false, Kernel::self()->widget())) {
      KFileItem* item = new KFileItem(KFileItem::Unknown, KFileItem::Unknown, url_, true);
      list.append(item);
      perm = item->permissions();
      grp = item->group();
    }

    KSaveFile f(url_.path());
    if(f.status() != 0) {
      if(!quiet_) {
        Kernel::self()->sorry(i18n(errorWrite).arg(url_.fileName()));
      }
      return false;
    }
    bool success = FileHandler::writeTextFile(f, text_, encodeUTF8_);
    if(!list.isEmpty()) {
      // have to leave the user alone
      KIO::Job* job = KIO::chmod(list, perm, 0, QString(), grp, true, false);
      new ItemDeleter(job, list.first());
    }
    return success;
  }

  // save to remote file
  KTempFile tempfile;
  KSaveFile f(tempfile.name());
  if(f.status() != 0) {
    tempfile.unlink();
    if(!quiet_) {
      Kernel::self()->sorry(i18n(errorWrite).arg(url_.fileName()));
    }
    return false;
  }

  bool success = FileHandler::writeTextFile(f, text_, encodeUTF8_);
  if(success) {
    bool uploaded = KIO::NetAccess::upload(tempfile.name(), url_, Kernel::self()->widget());
    if(!uploaded) {
      tempfile.unlink();
      if(!quiet_) {
        Kernel::self()->sorry(i18n(errorUpload).arg(url_.fileName()));
      }
      success = false;
    }
  }
  tempfile.unlink();

  return success;
}

bool FileHandler::writeTextFile(KSaveFile& f_, const QString& text_, bool encodeUTF8_) {
  QTextStream* t = f_.textStream();
  if(encodeUTF8_) {
    t->setEncoding(QTextStream::UnicodeUTF8);
  } else {
    t->setEncoding(QTextStream::Locale);
  }
//    kdDebug() << "-----------------------------" << endl
//              << text_ << endl
//              << "-----------------------------" << endl;
  (*t) << text_;
  bool success = f_.close();
#ifndef NDEBUG
  if(!success) {
    myDebug() << "FileHandler::writeTextFile() - status = " << f_.status();
  }
#endif
  return success;
}

bool FileHandler::writeDataURL(const KURL& url_, const QByteArray& data_, bool force_, bool quiet_) {
  if(!force_ && !queryExists(url_)) {
    return false;
  }

  if(url_.isLocalFile()) {
    // KSaveFile messes up users and groups
    // the user is always reset to the current user
    KFileItemList list;
    int perm = -1;
    QString grp;
    if(KIO::NetAccess::exists(url_, false, Kernel::self()->widget())) {
      KFileItem* item = new KFileItem(KFileItem::Unknown, KFileItem::Unknown, url_, true);
      list.append(item);
      perm = item->permissions();
      grp = item->group();
    }

    KSaveFile f(url_.path());
    if(f.status() != 0) {
      if(!quiet_) {
        Kernel::self()->sorry(i18n(errorWrite).arg(url_.fileName()));
      }
      return false;
    }
    bool success = FileHandler::writeDataFile(f, data_);
    if(!list.isEmpty()) {
      // have to leave the user alone
      KIO::Job* job = KIO::chmod(list, perm, 0, QString(), grp, true, false);
      new ItemDeleter(job, list.first());
    }
    return success;
  }

  // save to remote file
  KTempFile tempfile;
  KSaveFile f(tempfile.name());
  if(f.status() != 0) {
    if(!quiet_) {
      Kernel::self()->sorry(i18n(errorWrite).arg(url_.fileName()));
    }
    return false;
  }

  bool success = FileHandler::writeDataFile(f, data_);
  if(success) {
    success = KIO::NetAccess::upload(tempfile.name(), url_, Kernel::self()->widget());
    if(!success && !quiet_) {
      Kernel::self()->sorry(i18n(errorUpload).arg(url_.fileName()));
    }
  }
  tempfile.unlink();

  return success;
}

bool FileHandler::writeDataFile(KSaveFile& f_, const QByteArray& data_) {
//  myDebug() << "FileHandler::writeDataFile()" << endl;
  QDataStream* s = f_.dataStream();
  s->writeRawBytes(data_.data(), data_.size());
  return f_.close();
}

void FileHandler::clean() {
  s_deleterList.setAutoDelete(true);
  s_deleterList.clear();
  s_deleterList.setAutoDelete(false);
}
