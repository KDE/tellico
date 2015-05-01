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

#include "filehandler.h"
#include "tellico_strings.h"
#include "netaccess.h"
#include "../images/image.h"
#include "../gui/cursorsaver.h"
#include "../gui/guiproxy.h"
#include "../utils/xmlhandler.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>
#include <ktemporaryfile.h>
#include <ksavefile.h>
#include <kfileitem.h>
#include <kio/job.h>
#include <kfilterdev.h>
#include <kdeversion.h>
#include <kicon.h>
#include <kiconloader.h>

#include <QUrl>
#include <QDomDocument>
#include <QFile>
#include <QTextStream>
#include <QList>

using Tellico::FileHandler;

FileHandler::FileRef::FileRef(const QUrl& url_, bool quiet_) : m_device(0), m_isValid(false) {
  if(url_.isEmpty()) {
    return;
  }

  if(!Tellico::NetAccess::download(url_, m_filename, GUI::Proxy::widget(), quiet_)) {
    myDebug() << "can't download" << url_;
    QString s = Tellico::NetAccess::lastErrorString();
    if(!s.isEmpty()) {
      myDebug() << s;
    }
    if(!quiet_) {
      GUI::Proxy::sorry(s.isEmpty() ? i18n(errorLoad, url_.fileName()) : s);
    }
    return;
  }

  m_device = new QFile(m_filename);
  m_isValid = true;
}

FileHandler::FileRef::~FileRef() {
  if(!m_filename.isEmpty()) {
    Tellico::NetAccess::removeTempFile(m_filename);
  }
  if(m_device) {
    m_device->close();
  }
  delete m_device;
  m_device = 0;
  m_isValid = false;
}

bool FileHandler::FileRef::open(bool quiet_) {
  if(!isValid()) {
    return false;
  }
  if(!m_device || !m_device->open(QIODevice::ReadOnly)) {
    if(!quiet_) {
      QUrl u = QUrl::fromLocalFile(fileName());
      GUI::Proxy::sorry(i18n(errorLoad, u.fileName()));
    }
    delete m_device;
    m_device = 0;
    m_isValid = false;
    return false;
  }
  return true;
}

FileHandler::FileRef* FileHandler::fileRef(const QUrl& url_, bool quiet_) {
  return new FileRef(url_, quiet_);
}

QString FileHandler::readTextFile(const QUrl& url_, bool quiet_/*=false*/, bool useUTF8_ /*false*/) {
  FileRef f(url_, quiet_);
  if(!f.isValid()) {
    return QString();
  }

  if(f.open(quiet_)) {
    QTextStream stream(f.file());
    if(useUTF8_) {
      stream.setCodec("UTF-8");
    }
    return stream.readAll();
  }
  return QString();
}

QString FileHandler::readXMLFile(const QUrl& url_, bool quiet_/*=false*/) {
  FileRef f(url_, quiet_);
  if(!f.isValid()) {
    return QString();
  }

  if(f.open(quiet_)) {
    return XMLHandler::readXMLData(f.file()->readAll());
  }
  return QString();
}

QDomDocument FileHandler::readXMLDocument(const QUrl& url_, bool processNamespace_, bool quiet_) {
  FileRef f(url_, quiet_);
  if(!f.isValid()) {
    return QDomDocument();
  }

  QDomDocument doc;
  QString errorMsg;
  int errorLine, errorColumn;
  if(!f.open(quiet_)) {
    return QDomDocument();
  }
  if(!doc.setContent(f.file(), processNamespace_, &errorMsg, &errorLine, &errorColumn)) {
    if(!quiet_) {
      QString details = i18n("There is an XML parsing error in line %1, column %2.", errorLine, errorColumn);
      details += QLatin1String("\n");
      details += i18n("The error message from Qt is:");
      details += QLatin1String("\n\t") + errorMsg;
      GUI::CursorSaver cs(Qt::ArrowCursor);
      if(GUI::Proxy::widget()) {
        KMessageBox::detailedSorry(GUI::Proxy::widget(), i18n(errorLoad, url_.fileName()), details);
      }
    }
    return QDomDocument();
  }
  return doc;
}

QByteArray FileHandler::readDataFile(const QUrl& url_, bool quiet_) {
  FileRef f(url_, quiet_);
  if(!f.isValid()) {
    return QByteArray();
  }

  f.open(quiet_);
  return f.file()->readAll();
}

Tellico::Data::Image* FileHandler::readImageFile(const QUrl& url_, const QString& id_, bool quiet_, const QUrl& referrer_) {
  if(referrer_.isEmpty() || url_.isLocalFile()) {
    return readImageFile(url_, id_, quiet_);
  }

  KTemporaryFile tempFile;
  tempFile.open();
  tempFile.setAutoRemove(true);
  QUrl tempURL = QUrl::fromLocalFile(tempFile.fileName());

  KIO::JobFlags flags = KIO::Overwrite;
  if(quiet_) {
    flags |= KIO::HideProgressInfo;
  }

  KIO::Job* job = KIO::file_copy(url_, tempURL, -1, flags);
  job->addMetaData(QLatin1String("referrer"), referrer_.url());

  if(!KIO::NetAccess::synchronousRun(job, GUI::Proxy::widget())) {
    if(!quiet_) {
      QString str = i18n("Tellico is unable to load the image - %1.", url_.toDisplayString());
      GUI::Proxy::sorry(str);
    }
    return 0;
  }
  return readImageFile(tempURL, id_, quiet_);
}

Tellico::Data::Image* FileHandler::readImageFile(const QUrl& url_, const QString& id_, bool quiet_) {
  FileRef f(url_, quiet_);
  if(!f.isValid()) {
    return 0;
  }

  Data::Image* img = new Data::Image(f.fileName(), id_);
  if(img->isNull() && !quiet_) {
    QString str = i18n("Tellico is unable to load the image - %1.", url_.fileName());
    GUI::Proxy::sorry(str);
  }
  return img;
}

// really, this should be decoupled from the writeBackupFile() function
// but every other function that calls it would need to be updated
bool FileHandler::queryExists(const QUrl& url_) {
  if(url_.isEmpty() || !KIO::NetAccess::exists(url_, KIO::NetAccess::SourceSide, GUI::Proxy::widget())) {
    return true;
  }

  // no need to check if we're actually overwriting the current url
  // the TellicoImporter forces the write
  GUI::CursorSaver cs(Qt::ArrowCursor);
  KGuiItem guiItem(i18n("Overwrite"));
  guiItem.setIcon(KIcon(QLatin1String("document-save-as"),
                        KIconLoader::global(),
                        QStringList() << QLatin1String("emblem-important")));
  QString str = i18n("A file named \"%1\" already exists. "
                     "Are you sure you want to overwrite it?", url_.fileName());
  int want_continue = KMessageBox::warningContinueCancel(GUI::Proxy::widget(), str,
                                                         i18n("Overwrite File?"),
                                                         guiItem);

  if(want_continue == KMessageBox::Cancel) {
    return false;
  }
  return writeBackupFile(url_);
}

bool FileHandler::writeBackupFile(const QUrl& url_) {
  bool success = true;
  if(url_.isLocalFile()) {
    // KDE bug 178640, for versions prior to KDE 4.2RC1, backup file was not deleted first
    // this might fail if a different backup scheme is being used
    if(KDE::version() < KDE_MAKE_VERSION(4, 1, 90)) {
      QFile::remove(url_.path() + QLatin1Char('~'));
    }
//    success = KSaveFile::backupFile(url_.path());
    success = KBackup::backupFile(url_.path());
    if(KDE::version() < KDE_MAKE_VERSION(4, 1, 90)) {
      success = true; // ignore error for old version because of bug
    }
  } else {
    QUrl backup(url_);
    backup.setPath(backup.path() + QLatin1Char('~'));
    KIO::NetAccess::del(backup, GUI::Proxy::widget()); // might fail if backup doesn't exist, that's ok
    KIO::FileCopyJob* job = KIO::file_copy(url_, backup, -1, KIO::Overwrite);
    success = KIO::NetAccess::synchronousRun(job, GUI::Proxy::widget());
  }
  if(!success) {
    GUI::Proxy::sorry(i18n(errorWrite, url_.fileName() + QLatin1Char('~')));
  }
  return success;
}

bool FileHandler::writeTextURL(const QUrl& url_, const QString& text_, bool encodeUTF8_, bool force_, bool quiet_) {
  if((!force_ && !queryExists(url_)) || text_.isNull()) {
    if(text_.isNull()) {
      myDebug() << "null string for" << url_;
    }
    return false;
  }

  if(url_.isLocalFile()) {
    KSaveFile f(url_.path());
    f.open();
    if(f.error() != QFile::NoError) {
      if(!quiet_) {
        GUI::Proxy::sorry(i18n(errorWrite, url_.fileName()));
      }
      return false;
    }
    bool success = FileHandler::writeTextFile(f, text_, encodeUTF8_);
    return success;
  }

  // save to remote file
  KTemporaryFile tempfile;
  tempfile.open();
  KSaveFile f(tempfile.fileName());
  f.open();
  if(f.error() != QFile::NoError) {
    tempfile.remove();
    if(!quiet_) {
      GUI::Proxy::sorry(i18n(errorWrite, url_.fileName()));
    }
    return false;
  }

  bool success = FileHandler::writeTextFile(f, text_, encodeUTF8_);
  if(success) {
    success = KIO::NetAccess::upload(tempfile.fileName(), url_, GUI::Proxy::widget());
    if(!success) {
      tempfile.remove();
      if(!quiet_) {
        GUI::Proxy::sorry(i18n(errorUpload, url_.fileName()));
      }
    }
  }
  tempfile.remove();

  return success;
}

bool FileHandler::writeTextFile(KSaveFile& file_, const QString& text_, bool encodeUTF8_) {
  QTextStream t(&file_);
  if(encodeUTF8_) {
    t.setCodec("UTF-8");
  }
  t << text_;
  file_.flush();
  bool success = file_.finalize();
#ifndef NDEBUG
  if(!success) {
    myDebug() << "error = " << file_.error();
  }
#endif
  return success;
}

bool FileHandler::writeDataURL(const QUrl& url_, const QByteArray& data_, bool force_, bool quiet_) {
  if(!force_ && !queryExists(url_)) {
    return false;
  }

  if(url_.isLocalFile()) {
    KSaveFile f(url_.path());
    f.open();
    if(f.error() != QFile::NoError) {
      if(!quiet_) {
        GUI::Proxy::sorry(i18n(errorWrite, url_.fileName()));
      }
      return false;
    }
    return FileHandler::writeDataFile(f, data_);
  }

  // save to remote file
  KTemporaryFile tempfile;
  tempfile.open();
  KSaveFile f(tempfile.fileName());
  f.open();
  if(f.error() != QFile::NoError) {
    if(!quiet_) {
      GUI::Proxy::sorry(i18n(errorWrite, url_.fileName()));
    }
    return false;
  }

  bool success = FileHandler::writeDataFile(f, data_);
  if(success) {
    success = KIO::NetAccess::upload(tempfile.fileName(), url_, GUI::Proxy::widget());
    if(!success && !quiet_) {
      GUI::Proxy::sorry(i18n(errorUpload, url_.fileName()));
    }
  }
  tempfile.remove();

  return success;
}

bool FileHandler::writeDataFile(KSaveFile& file_, const QByteArray& data_) {
  QDataStream s(&file_);
  s.writeRawData(data_.data(), data_.size());
  file_.flush();
  bool success = file_.finalize();
#ifndef NDEBUG
  if(!success) {
    myDebug() << "error = " << file_.error();
  }
#endif
  return success;
}
