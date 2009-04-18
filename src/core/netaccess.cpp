/***************************************************************************
    copyright            : (C) 2006-2009 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "netaccess.h"
#include "../gui/guiproxy.h"
#include "../tellico_debug.h"

#include <kio/job.h>
#include <kio/netaccess.h>
#include <kio/previewjob.h>
#include <kio/jobuidelegate.h>
#include <ktemporaryfile.h>

static QStringList* tmpfiles = 0;

using Tellico::NetAccess;

bool NetAccess::download(const KUrl& url_, QString& target_, QWidget* window_, bool quiet_) {
  if(url_.isLocalFile()) {
    return KIO::NetAccess::download(url_, target_, window_);
  }
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
  KIO::Job* getJob = KIO::file_copy(url_, dest, -1, flags);
  if(KIO::NetAccess::synchronousRun(getJob, window_)) {
    return true;
  }
  getJob->ui()->showErrorMessage();
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

#include "netaccess.moc"
