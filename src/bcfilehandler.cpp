/***************************************************************************
                              bcfilehandler.cpp
                             -------------------
    begin                : Sun Oct 12 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bcfilehandler.h"
#include "bookcase.h"
#include "bookcasedoc.h"

#include <kurl.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>
#include <ktempfile.h>
#include <kdebug.h>

#include <qdom.h>
#include <qfile.h>

Bookcase* BCFileHandler::s_bookcase = 0;

QString BCFileHandler::readFile(const KURL& url_) {
  if(url_.isEmpty()) {
    return QString::null;
  }

  QString tmpfile;
  if(!KIO::NetAccess::download(url_, tmpfile)) {
    return QString::null;
  }

  QFile f(tmpfile);
  if(!f.open(IO_ReadOnly)) {
    KIO::NetAccess::removeTempFile(tmpfile);
    return QString::null;
  }

  QTextStream stream(&f);
  QString text = stream.read();

  f.close();
  KIO::NetAccess::removeTempFile(tmpfile);
  return text;
}

QDomDocument BCFileHandler::readXMLFile(const KURL& url_) {
  QDomDocument doc;

  if(url_.isEmpty()) {
    return doc;
  }

  QString tmpfile;
  if(!KIO::NetAccess::download(url_, tmpfile)) {
    return doc;
  }

  QFile f(tmpfile);

  if(!f.open(IO_ReadOnly)) {
     QString str = i18n("Bookcase is unable to open the file - %1.").arg(tmpfile);
     KMessageBox::sorry(s_bookcase, str);
     KIO::NetAccess::removeTempFile(tmpfile);
     return doc;
  }

  char buf[6];
  if(f.readBlock(buf, 5) < 5) {
    f.close();
    QString str = i18n("Bookcase is unable to read the file - %1.").arg(tmpfile);
    KMessageBox::sorry(s_bookcase, str);
    KIO::NetAccess::removeTempFile(tmpfile);
    return doc;
  }

  // Is it plain XML ?
  if(strncasecmp(buf, "<?xml", 5) != 0) {
    f.close();
    QString str = i18n("Bookcase is unable to load the file - %1.").arg(url_.fileName());
    KMessageBox::sorry(s_bookcase, str);
    KIO::NetAccess::removeTempFile(tmpfile);
    return doc;
  }

  f.at(0); // reset file pointer to beginning
  QString errorMsg;
  int errorLine, errorColumn;
  if(!doc.setContent(&f, false, &errorMsg, &errorLine, &errorColumn)) {
    f.close();
    QString str = i18n("Bookcase is unable to load the file - %1.").arg(url_.fileName());
    QString details = i18n("There is an XML parsing error in line %1, column %2.").arg(errorLine).arg(errorColumn);
    details += QString::fromLatin1("\n");
    details += i18n("The error message from Qt is:");
    details += QString::fromLatin1("\n");
    details += QString::fromLatin1("\t") + errorMsg;
    KMessageBox::detailedSorry(s_bookcase, str, details);
    KIO::NetAccess::removeTempFile(tmpfile);
    return doc;
  }

  f.close();
  KIO::NetAccess::removeTempFile(tmpfile);
  return doc;
}

bool BCFileHandler::writeURL(const KURL& url_,  const QString& text_, bool locale_/*=false*/) {
  if(KIO::NetAccess::exists(url_)) {
    if(url_ != s_bookcase->doc()->URL()) {
      QString str = i18n("A file named \"%1\" already exists. "
                         "Are you sure you want to overwrite it?").arg(url_.fileName());
      int want_continue = KMessageBox::warningContinueCancel(s_bookcase, str,
                                                             i18n("Overwrite File?"),
                                                             i18n("Overwrite"));

      if(want_continue == KMessageBox::Cancel) {
        return false;
      }
    }

    KURL backup(url_);
    backup.setPath(backup.path() + QString::fromLatin1("~"));
    KIO::NetAccess::del(backup);
    KIO::NetAccess::copy(url_, backup);
  }

  bool success;
  if(url_.isLocalFile()) {
    QFile f(url_.path());
    success = BCFileHandler::writeFile(f, text_, locale_);
    f.close();
  } else {
    KTempFile tempfile;
    QFile f(tempfile.name());
    success = BCFileHandler::writeFile(f, text_, locale_);
    f.close();
    if(success) {
      bool uploaded = KIO::NetAccess::upload(tempfile.name(), url_);
      if(!uploaded) {
        QString str = i18n("Bookcase is unable to upload the file - %1.").arg(url_.url());
        KMessageBox::sorry(s_bookcase, str);
      }
    }
    tempfile.unlink();
  }

  return success;
}

bool BCFileHandler::writeFile(QFile& f_, const QString& text_, bool locale_) {
  if(f_.open(IO_WriteOnly)) {
    QTextStream t(&f_);
    if(locale_) {
      t.setEncoding(QTextStream::Locale);
    } else {
      t.setEncoding(QTextStream::UnicodeUTF8);
    }
//    kdDebug() << "-----------------------------" << endl
//              << text_ << endl
//              << "-----------------------------" << endl;
    t << text_;
    return true;
  } else {
    QString str = i18n("Bookcase is unable to write the file - %1.").arg(f_.name());
    KMessageBox::sorry(s_bookcase, str);
    return false;
  }
}
