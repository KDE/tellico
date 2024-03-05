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

#include "netaccess.h"
#include "tellico_strings.h"
#include "../utils/guiproxy.h"
#include "../tellico_debug.h"

#include <KIO/StoredTransferJob>
#include <KIO/PreviewJob>
#include <KIO/StatJob>
#include <KIO/JobUiDelegate>
#include <KJobWidgets>
#include <KLocalizedString>

#include <QUrl>
#include <QFileInfo>
#include <QTemporaryFile>

static QStringList* tmpfiles = nullptr;

QString Tellico::NetAccess::s_lastErrorMessage;

using Tellico::NetAccess;

bool NetAccess::download(const QUrl& url_, QString& target_, QWidget* window_, bool quiet_) {
  // copied from KIO::NetAccess::download() apidox except for quiet part
  if(url_.isLocalFile()) {
    target_ = url_.toLocalFile();
    const bool readable = QFileInfo(target_).isReadable();
    if(!readable) {
      s_lastErrorMessage = TC_I18N2(errorOpen, target_);
    }
    return readable;
  }

  Q_ASSERT(target_.isEmpty());
  if(target_.isEmpty()) {
    QTemporaryFile tmpFile;
    tmpFile.setAutoRemove(false);
    tmpFile.open();
    target_ = tmpFile.fileName();
    if(!tmpfiles) {
      tmpfiles = new QStringList();
    }
    tmpfiles->append(target_);
  }

  KIO::JobFlags flags = KIO::Overwrite;
  if(quiet_ || !window_) {
    flags |= KIO::HideProgressInfo;
  }

  // KIO::storedGet seems to handle Content-Encoding: gzip ok
  KIO::StoredTransferJob* getJob = KIO::storedGet(url_, KIO::NoReload, flags);
  KJobWidgets::setWindow(getJob, window_);
  if(getJob->exec()) {
    QFile f(target_);
    if(f.open(QIODevice::WriteOnly)) {
      if(f.write(getJob->data()) > -1) {
        return true;
      } else {
        s_lastErrorMessage = TC_I18N2(errorWrite, target_);
        myWarning() << "failed to write to" << target_;
      }
    } else {
      s_lastErrorMessage = TC_I18N2(errorOpen, target_);
    }
  } else {
    s_lastErrorMessage = QStringLiteral("Tellico was unable to download %1").arg(url_.url());
    myWarning() << getJob->errorString();
  }

  if(!quiet_ && getJob->uiDelegate()) {
    getJob->uiDelegate()->showErrorMessage();
  }
  return false;
}

QPixmap NetAccess::filePreview(const QUrl& url, int size) {
  return filePreview(KFileItem(url), size);
}

QPixmap NetAccess::filePreview(const KFileItem& item, int size) {
  NetAccess netaccess;

  // the default plugins are not used by default (what???)
  // the default ones are in config settings instead, so ignore that
  const QStringList plugins = KIO::PreviewJob::defaultPlugins();
  KIO::PreviewJob* previewJob = KIO::filePreview(KFileItemList() << item, QSize(size, size),
                                                 &plugins);
  connect(previewJob, &KIO::PreviewJob::gotPreview,
          &netaccess, &Tellico::NetAccess::slotPreview);

  if(GUI::Proxy::widget()) {
    KJobWidgets::setWindow(previewJob, GUI::Proxy::widget());
  }
  if(!previewJob->exec()) {
    myDebug() << "Preview job did not succeed";
  }
  if(previewJob->error() != 0) {
    myDebug() << previewJob->errorString();
  }
  return netaccess.m_preview;
}

void NetAccess::slotPreview(const KFileItem&, const QPixmap& pix_) {
  m_preview = pix_;
}

void NetAccess::removeTempFile(const QString& name) {
  if(!tmpfiles) {
    return;
  }
  if(tmpfiles->contains(name)) {
    QFile::remove(name);
    tmpfiles->removeAll(name);
  }
}

bool NetAccess::exists(const QUrl& url_, bool sourceSide_, QWidget* window_) {
  if(url_.isLocalFile()) {
    return QFile::exists(url_.toLocalFile());
  }

  KIO::JobFlags flags = KIO::DefaultFlags;
  if(!window_) flags |= KIO::HideProgressInfo;
  KIO::StatJob* job = KIO::stat(url_, flags);
  KJobWidgets::setWindow(job, window_);
  job->setSide(sourceSide_ ? KIO::StatJob::SourceSide : KIO::StatJob::DestinationSide);
  return job->exec();
}

QString NetAccess::lastErrorString() {
  return s_lastErrorMessage;
}
