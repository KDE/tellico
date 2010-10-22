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
#include "../gui/guiproxy.h"
#include "../tellico_debug.h"

#include <kio/job.h>
#include <kio/netaccess.h>
#include <kio/previewjob.h>
#include <kio/jobuidelegate.h>
#include <ktemporaryfile.h>
#include <klocale.h>

#include <QEventLoop>

static QStringList* tmpfiles = 0;

QString Tellico::NetAccess::s_lastErrorMessage;

using Tellico::NetAccess;

bool NetAccess::download(const KUrl& url_, QString& target_, QWidget* window_, bool quiet_) {
  if(url_.isLocalFile()) {
    return KIO::NetAccess::download(url_, target_, window_);
  }
  Q_ASSERT(target_.isEmpty());
  // copied from KIO::NetAccess::download() apidox except for quiet part
  if(target_.isEmpty()) {
    KTemporaryFile tmpFile;
    tmpFile.setAutoRemove(false);
    tmpFile.open();
    target_ = tmpFile.fileName();
    if(!tmpfiles) {
      tmpfiles = new QStringList();
    }
    tmpfiles->append(target_);
  }

  KUrl dest;
  dest.setPath(target_);
  KIO::JobFlags flags = KIO::Overwrite;
  if(quiet_) {
    flags |= KIO::HideProgressInfo;
  }

  // KIO::storedGet seems to handle Content-Encoding: gzip ok
  QByteArray data;
//  KIO::StoredTransferJob* getJob = KIO::storedGet(url_, KIO::NoReload, flags);
  KIO::TransferJob* getJob = KIO::get(url_, KIO::NoReload, flags);
  if(KIO::NetAccess::synchronousRun(getJob, window_, &data)) {
    QFile f(target_);
    if(f.open(QIODevice::WriteOnly)) {
//      if(f.write(getJob->data()) > -1) {
      if(f.write(data) > -1) {
        return true;
      } else {
        s_lastErrorMessage = i18n(errorWrite, target_);
        myWarning() << "failed to write to" << target_;
      }
    } else {
      s_lastErrorMessage = i18n(errorOpen, target_);
    }
  } else {
    s_lastErrorMessage = QString::fromLatin1("Tellico was unable to download %1").arg(url_.url());
  }

  if(!quiet_ && getJob->ui()) {
    getJob->ui()->showErrorMessage();
  }
  return false;
}

QPixmap NetAccess::filePreview(const KUrl& url, int size) {
  NetAccess netaccess;

  KUrl::List list;
  list.append(url);
  KIO::Job* previewJob = KIO::filePreview(list, size, size);
  connect(previewJob, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
          &netaccess, SLOT(slotPreview(const KFileItem&, const QPixmap&)));

  KIO::NetAccess::synchronousRun(previewJob, GUI::Proxy::widget());
  return netaccess.m_preview;
}

QPixmap NetAccess::filePreview(const KFileItem& item, int size) {
  NetAccess netaccess;

  KFileItemList list;
  list.append(item);
  KIO::Job* previewJob = KIO::filePreview(list, size, size);
  connect(previewJob, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
          &netaccess, SLOT(slotPreview(const KFileItem&, const QPixmap&)));

  KIO::NetAccess::synchronousRun(previewJob, GUI::Proxy::widget());
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
    ::unlink(QFile::encodeName(name));
    tmpfiles->removeAll(name);
  } else {
    KIO::NetAccess::removeTempFile(name);
  }
}

QString NetAccess::lastErrorString() {
  return s_lastErrorMessage;
}

#include "netaccess.moc"
