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
#include "../utils/cursorsaver.h"
#include "../utils/guiproxy.h"
#include "../utils/xmlhandler.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KFileItem>
#include <KIO/FileCopyJob>
#include <KIO/DeleteJob>
#include <KBackup>
#include <KJobWidgets>

#include <QUrl>
#include <QDomDocument>
#include <QFile>
#include <QTextStream>
#include <QTemporaryFile>
#include <QSaveFile>

namespace {
  static const int MAX_TEXT_CHUNK_WRITE_SIZE = 100 * 1024 * 1024;
}

using Tellico::FileHandler;

FileHandler::FileRef::FileRef(const QUrl& url_, bool quiet_) : m_device(nullptr), m_isValid(false) {
  if(url_.isEmpty()) {
    return;
  }

  if(!Tellico::NetAccess::download(url_, m_filename, GUI::Proxy::widget(), quiet_)) {
    QString s = Tellico::NetAccess::lastErrorString();
    if(s.isEmpty()) {
      myLog() << "Can't download" << url_.toDisplayString(QUrl::PreferLocalFile);
    } else {
      myLog() << s;
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
  m_device = nullptr;
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
    m_device = nullptr;
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
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
      stream.setCodec("UTF-8");
#else
      stream.setEncoding(QStringConverter::Utf8);
#endif
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
        KMessageBox::detailedError(GUI::Proxy::widget(), i18n(errorLoad, url_.fileName()), details);
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

// TODO: really, this should be decoupled from the writeBackupFile() function
// but every other function that calls it would need to be updated
bool FileHandler::queryExists(const QUrl& url_) {
  if(url_.isEmpty() || !QFile::exists(url_.toLocalFile())) {
    return true;
  }

  // no need to check if we're actually overwriting the current url
  // the TellicoImporter forces the write
  GUI::CursorSaver cs(Qt::ArrowCursor);
  QString str = i18n("A file named \"%1\" already exists. "
                     "Are you sure you want to overwrite it?", url_.fileName());
  int want_continue = KMessageBox::warningContinueCancel(GUI::Proxy::widget(), str,
                                                         i18n("Overwrite File?"),
                                                         KStandardGuiItem::overwrite());

  if(want_continue == KMessageBox::Cancel) {
    return false;
  }
  return writeBackupFile(url_);
}

bool FileHandler::writeBackupFile(const QUrl& url_) {
  bool success = true;
  if(url_.isLocalFile()) {
    success = KBackup::simpleBackupFile(url_.toLocalFile());
  } else {
    QUrl backup(url_);
    backup.setPath(backup.toLocalFile() + QLatin1Char('~'));
    KIO::DeleteJob* delJob = KIO::del(backup);
    KJobWidgets::setWindow(delJob, GUI::Proxy::widget());
    delJob->exec(); // might fail if backup doesn't exist, that's ok
    KIO::FileCopyJob* job = KIO::file_copy(url_, backup, -1, KIO::Overwrite);
    KJobWidgets::setWindow(job, GUI::Proxy::widget());
    success = job->exec();
  }
  if(!success) {
    GUI::Proxy::sorry(i18n(errorWrite, url_.fileName() + QLatin1Char('~')));
  }
  return success;
}

bool FileHandler::writeTextURL(const QUrl& url_, const QString& text_, bool encodeUTF8_, bool force_, bool quiet_) {
  if((!force_ && !queryExists(url_)) || text_.isNull()) {
    return false;
  }

  if(url_.isLocalFile()) {
    QSaveFile f(url_.toLocalFile());
    f.open(QIODevice::WriteOnly);
    if(f.error() != QFile::NoError) {
      if(!quiet_) {
        GUI::Proxy::sorry(i18n(errorWrite, url_.fileName()));
      }
      return false;
    }
    return FileHandler::writeTextFile(f, text_, encodeUTF8_);
  }

  // save to remote file
  QTemporaryFile tempfile;
  tempfile.open();
  QSaveFile f(tempfile.fileName());
  f.open(QIODevice::WriteOnly);
  if(f.error() != QFile::NoError) {
    tempfile.remove();
    if(!quiet_) {
      GUI::Proxy::sorry(i18n(errorWrite, url_.fileName()));
    }
    return false;
  }

  bool success = FileHandler::writeTextFile(f, text_, encodeUTF8_);
  if(success) {
    KIO::Job* job = KIO::file_copy(QUrl::fromLocalFile(tempfile.fileName()), url_, -1, KIO::Overwrite);
    KJobWidgets::setWindow(job, GUI::Proxy::widget());
    success = job->exec();
    if(!success && !quiet_) {
      GUI::Proxy::sorry(i18n(errorUpload, url_.fileName()));
    }
  }
  tempfile.remove();

  return success;
}

bool FileHandler::writeTextFile(QSaveFile& file_, const QString& text_, bool encodeUTF8_) {
  QTextStream ts(&file_);
  if(encodeUTF8_) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    ts.setCodec("UTF-8");
#else
    ts.setEncoding(QStringConverter::Utf8);
#endif
  }
  // KDE Bug 380832. If string is longer than MAX_TEXT_CHUNK_WRITE_SIZE characters, split into chunks.
  for(int i = 0; i < text_.length(); i += MAX_TEXT_CHUNK_WRITE_SIZE) {
    ts << text_.mid(i, MAX_TEXT_CHUNK_WRITE_SIZE);
  }
  file_.flush();
  const bool success = file_.commit();
  if(!success) {
    myLog() << "Failed to write text file:" << file_.error();
  }
  return success;
}

bool FileHandler::writeDataURL(const QUrl& url_, const QByteArray& data_, bool force_, bool quiet_) {
  if(!force_ && !queryExists(url_)) {
    return false;
  }

  if(url_.isLocalFile()) {
    QSaveFile f(url_.toLocalFile());
    f.open(QIODevice::WriteOnly);
    if(f.error() != QFile::NoError) {
      if(!quiet_) {
        GUI::Proxy::sorry(i18n(errorWrite, url_.fileName()));
      }
      return false;
    }
    return FileHandler::writeDataFile(f, data_);
  }

  // save to remote file
  QTemporaryFile tempfile;
  tempfile.open();
  QSaveFile f(tempfile.fileName());
  f.open(QIODevice::WriteOnly);
  if(f.error() != QFile::NoError) {
    if(!quiet_) {
      GUI::Proxy::sorry(i18n(errorWrite, url_.fileName()));
    }
    return false;
  }

  bool success = FileHandler::writeDataFile(f, data_);
  if(success) {
    KIO::Job* job = KIO::file_copy(QUrl::fromLocalFile(tempfile.fileName()), url_, -1, KIO::Overwrite);
    KJobWidgets::setWindow(job, GUI::Proxy::widget());
    success = job->exec();
    if(!success && !quiet_) {
      GUI::Proxy::sorry(i18n(errorUpload, url_.fileName()));
    }
  }
  tempfile.remove();

  return success;
}

bool FileHandler::writeDataFile(QSaveFile& file_, const QByteArray& data_) {
  QDataStream s(&file_);
  s.writeRawData(data_.data(), data_.size());
  file_.flush();
  const bool success = file_.commit();
  if(!success) {
    myDebug() << "Failed to write data file:" << file_.error();
  }
  return success;
}
