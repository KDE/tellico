/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
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
#include "document.h"
#include "image.h"
#include "tellico_kernel.h"
#include "tellico_strings.h"

#include <kurl.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>
#include <ktempfile.h>
#include <ksavefile.h>
#include <kapplication.h>
#include <kdebug.h>

#include <qdom.h>
#include <qfile.h>

using Tellico::FileHandler;

FileHandler::FileRef::FileRef(const KURL& url_, bool quiet_) : file(0), isValid(false) {
  if(url_.isEmpty()) {
    return;
  }

#if KDE_IS_VERSION(3,1,90)
  if(!KIO::NetAccess::download(url_, filename, Kernel::self()->widget())) {
#else
  if(!KIO::NetAccess::download(url_, filename)) {
#endif
    if(!quiet_) {
      kapp->restoreOverrideCursor();
      KMessageBox::sorry(Kernel::self()->widget(), i18n(errorLoad).arg(url_.fileName()));
      kapp->setOverrideCursor(Qt::waitCursor);
    }
    return;
  }

  file = new QFile(filename);
  if(!file->open(IO_ReadOnly)) {
    kapp->restoreOverrideCursor();
    KMessageBox::sorry(Kernel::self()->widget(), i18n(errorLoad).arg(url_.fileName()));
    kapp->setOverrideCursor(Qt::waitCursor);
    delete file;
    file = 0;
    KIO::NetAccess::removeTempFile(filename);
    return;
  }

  isValid = true;
}

FileHandler::FileRef::~FileRef() {
  if(!filename.isEmpty()) {
    KIO::NetAccess::removeTempFile(filename);
  }
  if(file) {
    file->close();
  }
  delete file;
  file = 0;
  isValid = false;
}

QString FileHandler::readTextFile(const KURL& url_, bool quiet_) {
  FileRef f(url_, quiet_);
  if(!f.isValid) {
    return QString::null;
  }

  QTextStream stream(f.file);
  return stream.read();
}

QDomDocument FileHandler::readXMLFile(const KURL& url_, bool processNamespace_, bool quiet_) {
  FileRef f(url_, quiet_);
  if(!f.isValid) {
    return QDomDocument();
  }

  QDomDocument doc;
  QString errorMsg;
  int errorLine, errorColumn;
  if(!doc.setContent(f.file, processNamespace_, &errorMsg, &errorLine, &errorColumn)) {
    if(!quiet_) {
      QString details = i18n("There is an XML parsing error in line %1, column %2.").arg(errorLine).arg(errorColumn);
      details += QString::fromLatin1("\n");
      details += i18n("The error message from Qt is:");
      details += QString::fromLatin1("\n\t") + errorMsg;
      KMessageBox::detailedSorry(Kernel::self()->widget(), i18n(errorLoad).arg(url_.fileName()), details);
    }
    return QDomDocument();
  }
  return doc;
}

QByteArray FileHandler::readDataFile(const KURL& url_, bool quiet_) {
  FileRef f(url_, quiet_);
  if(!f.isValid) {
    return QByteArray();
  }

  return f.file->readAll();
}

Tellico::Data::Image* FileHandler::readImageFile(const KURL& url_, bool quiet_) {
  FileRef f(url_, quiet_);
  if(!f.isValid) {
    return 0;
  }

  Data::Image* img = new Data::Image(f.filename);
  if(img->isNull() && !quiet_) {
    QString str = i18n("Tellico is unable to load the image - %1.").arg(url_.fileName());
    KMessageBox::sorry(Kernel::self()->widget(), str);
  }
  return img;
}

bool FileHandler::queryExists(const KURL& url_) {
  bool success = true;
#if KDE_IS_VERSION(3,1,90)
  if(KIO::NetAccess::exists(url_, false, Kernel::self()->widget())) {
#else
  if(KIO::NetAccess::exists(url_, false)) {
#endif
    if(url_ != Kernel::self()->doc()->URL()) {
      kapp->restoreOverrideCursor();
      QString str = i18n("A file named \"%1\" already exists. "
                         "Are you sure you want to overwrite it?").arg(url_.fileName());
      int want_continue = KMessageBox::warningContinueCancel(Kernel::self()->widget(), str,
                                                             i18n("Overwrite File?"),
                                                             i18n("Overwrite"));

      kapp->setOverrideCursor(Qt::waitCursor);
      if(want_continue == KMessageBox::Cancel) {
        return false;
      }
    }

    KURL backup(url_);
    backup.setPath(backup.path() + QString::fromLatin1("~"));
#if KDE_IS_VERSION(3,1,90)
    KIO::NetAccess::del(backup, Kernel::self()->widget()); // might fail if backup doesn't exist, that's ok
    success = KIO::NetAccess::copy(url_, backup, Kernel::self()->widget());
#else
    KIO::NetAccess::del(backup); // might fail if backup doesn't exist, that's ok
    success = KIO::NetAccess::copy(url_, backup);
#endif
    if(!success) {
      kapp->restoreOverrideCursor();
      KMessageBox::sorry(Kernel::self()->widget(), i18n(errorWrite).arg(backup.fileName()));
      kapp->setOverrideCursor(Qt::waitCursor);
    }
  }
  return success;
}

bool FileHandler::writeTextURL(const KURL& url_, const QString& text_, bool encodeUTF8_, bool force_) {
  if((!force_ && !queryExists(url_)) || text_.isNull()) {
    return false;
  }

  if(url_.isLocalFile()) {
    KSaveFile f(url_.path());
    if(f.status() != 0) {
      kapp->restoreOverrideCursor();
      KMessageBox::sorry(Kernel::self()->widget(), i18n(errorWrite).arg(url_.fileName()));
      kapp->setOverrideCursor(Qt::waitCursor);
      return false;
    }
    return FileHandler::writeTextFile(f, text_, encodeUTF8_);
  }

  // save to remote file
  KTempFile tempfile;
  KSaveFile f(tempfile.name());
  if(f.status() != 0) {
    tempfile.unlink();
    kapp->restoreOverrideCursor();
    KMessageBox::sorry(Kernel::self()->widget(), i18n(errorWrite).arg(url_.fileName()));
    kapp->setOverrideCursor(Qt::waitCursor);
    return false;
  }

  bool success = FileHandler::writeTextFile(f, text_, encodeUTF8_);
  if(success) {
#if KDE_IS_VERSION(3,1,90)
    bool uploaded = KIO::NetAccess::upload(tempfile.name(), url_, Kernel::self()->widget());
#else
    bool uploaded = KIO::NetAccess::upload(tempfile.name(), url_);
#endif
    if(!uploaded) {
      tempfile.unlink();
      kapp->restoreOverrideCursor();
      KMessageBox::sorry(Kernel::self()->widget(), i18n(errorUpload).arg(url_.fileName()));
      kapp->setOverrideCursor(Qt::waitCursor);
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
    kdDebug() << "FileHandler::writeTextFile() - status = " << f_.status();
  }
#endif
  return success;
}

bool FileHandler::writeDataURL(const KURL& url_, const QByteArray& data_, bool force_) {
  if(!force_ && !queryExists(url_)) {
    return false;
  }

  if(url_.isLocalFile()) {
    KSaveFile f(url_.path());
    if(f.status() != 0) {
      kapp->restoreOverrideCursor();
      KMessageBox::sorry(Kernel::self()->widget(), i18n(errorWrite).arg(url_.fileName()));
      kapp->setOverrideCursor(Qt::waitCursor);
      return false;
    }
    return FileHandler::writeDataFile(f, data_);
  }

  // save to remote file
  KTempFile tempfile;
  KSaveFile f(tempfile.name());
  if(f.status() != 0) {
    kapp->restoreOverrideCursor();
    KMessageBox::sorry(Kernel::self()->widget(), i18n(errorWrite).arg(url_.fileName()));
    kapp->setOverrideCursor(Qt::waitCursor);
    return false;
  }

  bool success = FileHandler::writeDataFile(f, data_);
  if(success) {
#if KDE_IS_VERSION(3,1,90)
    bool uploaded = KIO::NetAccess::upload(tempfile.name(), url_, Kernel::self()->widget());
#else
    bool uploaded = KIO::NetAccess::upload(tempfile.name(), url_);
#endif
    if(!uploaded) {
      kapp->restoreOverrideCursor();
      KMessageBox::sorry(Kernel::self()->widget(), i18n(errorUpload).arg(url_.fileName()));
      kapp->setOverrideCursor(Qt::waitCursor);
      success = false;
    }
  }
  tempfile.unlink();

  return success;
}

bool FileHandler::writeDataFile(KSaveFile& f_, const QByteArray& data_) {
//  kdDebug() << "FileHandler::writeDataFile()" << endl;
  QDataStream* s = f_.dataStream();
  s->writeRawBytes(data_.data(), data_.size());
  bool success = f_.close();
  return success;
}
