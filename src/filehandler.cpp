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
#include "mainwindow.h"
#include "document.h"
#include "image.h"
#include "utils.h" // needed for version macros
#include "error_strings.h" // needed for error strings

#include <kurl.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>
#include <ktempfile.h>
#include <ksavefile.h>
#include <kdebug.h>

#include <qdom.h>
#include <qfile.h>

using Bookcase::FileHandler;

Bookcase::MainWindow* FileHandler::s_mainWindow = 0;

FileHandler::FileRef::FileRef(const KURL& url_) : file(0), isValid(false) {
  if(url_.isEmpty()) {
    return;
  }

#if KDE_IS_VERSION(3,1,90)
  if(!KIO::NetAccess::download(url_, filename, FileHandler::s_mainWindow)) {
#else
  if(!KIO::NetAccess::download(url_, filename)) {
#endif
    KMessageBox::sorry(FileHandler::s_mainWindow, i18n(loadError).arg(url_.fileName()));
    return;
  }

  file = new QFile(filename);
  if(!file->open(IO_ReadOnly)) {
    KMessageBox::sorry(FileHandler::s_mainWindow, i18n(loadError).arg(url_.fileName()));
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

QString FileHandler::readTextFile(const KURL& url_) {
  FileRef f(url_);
  if(!f.isValid) {
    return QString::null;
  }

  QTextStream stream(f.file);
  return stream.read();
}

QDomDocument FileHandler::readXMLFile(const KURL& url_) {
  FileRef f(url_);
  if(!f.isValid) {
    return QDomDocument();
  }

  QDomDocument doc;
  QString errorMsg;
  int errorLine, errorColumn;
  if(!doc.setContent(f.file, false, &errorMsg, &errorLine, &errorColumn)) {
    QString details = i18n("There is an XML parsing error in line %1, column %2.").arg(errorLine).arg(errorColumn);
    details += QString::fromLatin1("\n");
    details += i18n("The error message from Qt is:");
    details += QString::fromLatin1("\n\t") + errorMsg;
    KMessageBox::detailedSorry(s_mainWindow, i18n(loadError).arg(url_.fileName()), details);
    return QDomDocument();
  }
  return doc;
}

QByteArray FileHandler::readDataFile(const KURL& url_) {
  FileRef f(url_);
  if(!f.isValid) {
    return QByteArray();
  }

  return f.file->readAll();
}

Bookcase::Data::Image* FileHandler::readImageFile(const KURL& url_) {
  FileRef f(url_);
  if(!f.isValid) {
    return 0;
  }

  return new Data::Image(f.filename);
}

bool FileHandler::queryExists(const KURL& url_) {
  bool success = true;
#if KDE_IS_VERSION(3,1,90)
  if(KIO::NetAccess::exists(url_, false, s_mainWindow)) {
#elif KDE_IS_VERSION(3,0,90)
  if(KIO::NetAccess::exists(url_, false)) {
#else
  if(KIO::NetAccess::exists(url_)) {
#endif
    if(url_ != s_mainWindow->doc()->URL()) {
      QString str = i18n("A file named \"%1\" already exists. "
                         "Are you sure you want to overwrite it?").arg(url_.fileName());
      int want_continue = KMessageBox::warningContinueCancel(s_mainWindow, str,
                                                             i18n("Overwrite File?"),
                                                             i18n("Overwrite"));

      if(want_continue == KMessageBox::Cancel) {
        return false;
      }
    }

    KURL backup(url_);
    backup.setPath(backup.path() + QString::fromLatin1("~"));
#if KDE_IS_VERSION(3,1,90)
    KIO::NetAccess::del(backup, s_mainWindow); // might fail if backup doesn't exist, that's ok
    success = KIO::NetAccess::copy(url_, backup, s_mainWindow);
#else
    KIO::NetAccess::del(backup); // might fail if backup doesn't exist, that's ok
    success = KIO::NetAccess::copy(url_, backup);
#endif
    if(!success) {
      KMessageBox::sorry(s_mainWindow, i18n(writeError).arg(backup.fileName()));
    }
  }
  return success;
}

bool FileHandler::writeTextURL(const KURL& url_, const QString& text_, bool locale_) {
  if(!queryExists(url_)) {
    return false;
  }

  if(url_.isLocalFile()) {
    KSaveFile f(url_.path());
    if(f.status() != 0) {
      KMessageBox::sorry(s_mainWindow, i18n(writeError).arg(url_.fileName()));
      return false;
    }
    return FileHandler::writeTextFile(f, text_, locale_);
  }

  // save to remote file
  KTempFile tempfile;
  KSaveFile f(tempfile.name());
  if(f.status() != 0) {
    KMessageBox::sorry(s_mainWindow, i18n(writeError).arg(url_.fileName()));
    return false;
  }

  bool success = FileHandler::writeTextFile(f, text_, locale_);
  if(success) {
#if KDE_IS_VERSION(3,1,90)
    bool uploaded = KIO::NetAccess::upload(tempfile.name(), url_, s_mainWindow);
#else
    bool uploaded = KIO::NetAccess::upload(tempfile.name(), url_);
#endif
    if(!uploaded) {
      KMessageBox::sorry(s_mainWindow, i18n(uploadError).arg(url_.fileName()));
      success = false;
    }
  }
  tempfile.unlink();

  return success;
}

bool FileHandler::writeTextFile(KSaveFile& f_, const QString& text_, bool locale_) {
  QTextStream* t = f_.textStream();
  if(locale_) {
    t->setEncoding(QTextStream::Locale);
  } else {
    t->setEncoding(QTextStream::UnicodeUTF8);
  }
//    kdDebug() << "-----------------------------" << endl
//              << text_ << endl
//              << "-----------------------------" << endl;
  (*t) << text_;
  bool success = f_.close();
  if(!success) {
    kdDebug() << "FileHandler::writeTextFile() - status = " << f_.status();
  }
  return success;
}

bool FileHandler::writeDataURL(const KURL& url_, const QByteArray& data_) {
  if(!queryExists(url_)) {
    return false;
  }

  if(url_.isLocalFile()) {
    KSaveFile f(url_.path());
    if(f.status() != 0) {
      KMessageBox::sorry(s_mainWindow, i18n(writeError).arg(url_.fileName()));
      return false;
    }
    return FileHandler::writeDataFile(f, data_);
  }

  // save to remote file
  KTempFile tempfile;
  KSaveFile f(tempfile.name());
  if(f.status() != 0) {
    KMessageBox::sorry(s_mainWindow, i18n(writeError).arg(url_.fileName()));
    return false;
  }

  bool success = FileHandler::writeDataFile(f, data_);
  if(success) {
#if KDE_IS_VERSION(3,1,90)
    bool uploaded = KIO::NetAccess::upload(tempfile.name(), url_, s_mainWindow);
#else
    bool uploaded = KIO::NetAccess::upload(tempfile.name(), url_);
#endif
    if(!uploaded) {
      KMessageBox::sorry(s_mainWindow, i18n(uploadError).arg(url_.fileName()));
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
