/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
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

#include <kurl.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>
#include <ktempfile.h>
#include <ksavefile.h>
#include <kapplication.h>

#include <qdom.h>
#include <qfile.h>

using Tellico::FileHandler;

FileHandler::FileRef::FileRef(const KURL& url_, bool quiet_) : m_file(0), m_isValid(false) {
  if(url_.isEmpty()) {
    return;
  }

#if KDE_IS_VERSION(3,1,90)
  if(!KIO::NetAccess::download(url_, m_filename, Kernel::self()->widget())) {
#else
  if(!KIO::NetAccess::download(url_, m_filename)) {
#endif
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
    KIO::NetAccess::removeTempFile(m_filename);
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
    KIO::NetAccess::removeTempFile(m_filename);
    m_isValid = false;
    return false;
  }
  return true;
}

FileHandler::FileRef* FileHandler::fileRef(const KURL& url_, bool quiet_) {
  return new FileRef(url_, quiet_);
}

QString FileHandler::readTextFile(const KURL& url_, bool quiet_) {
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
  bool success = true;
#if KDE_IS_VERSION(3,1,90)
  if(KIO::NetAccess::exists(url_, false, Kernel::self()->widget())) {
#else
  if(KIO::NetAccess::exists(url_, false)) {
#endif
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
#if KDE_IS_VERSION(3,1,90)
    KIO::NetAccess::del(backup, Kernel::self()->widget()); // might fail if backup doesn't exist, that's ok
    success = KIO::NetAccess::copy(url_, backup, Kernel::self()->widget());
#else
    KIO::NetAccess::del(backup); // might fail if backup doesn't exist, that's ok
    success = KIO::NetAccess::copy(url_, backup);
#endif
    if(!success) {
      Kernel::self()->sorry(i18n(errorWrite).arg(backup.fileName()));
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
      Kernel::self()->sorry(i18n(errorWrite).arg(url_.fileName()));
      return false;
    }
    return FileHandler::writeTextFile(f, text_, encodeUTF8_);
  }

  // save to remote file
  KTempFile tempfile;
  KSaveFile f(tempfile.name());
  if(f.status() != 0) {
    tempfile.unlink();
    Kernel::self()->sorry(i18n(errorWrite).arg(url_.fileName()));
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
      Kernel::self()->sorry(i18n(errorUpload).arg(url_.fileName()));
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
      Kernel::self()->sorry(i18n(errorWrite).arg(url_.fileName()));
      return false;
    }
    return FileHandler::writeDataFile(f, data_);
  }

  // save to remote file
  KTempFile tempfile;
  KSaveFile f(tempfile.name());
  if(f.status() != 0) {
    Kernel::self()->sorry(i18n(errorWrite).arg(url_.fileName()));
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
      Kernel::self()->sorry(i18n(errorUpload).arg(url_.fileName()));
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
  return f_.close();
}
