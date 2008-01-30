/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
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
#include "../tellico_kernel.h"
#include "../tellico_debug.h"

#include <kdeversion.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <kio/scheduler.h>
#include <kio/previewjob.h>
#include <ktempfile.h>

#include <qapplication.h>
#include <qfile.h>

#include <unistd.h> // for unlink()

using Tellico::NetAccess;

QStringList* NetAccess::s_tmpFiles = 0;

bool NetAccess::download(const KURL& url_, QString& target_, QWidget* window_) {
  if(url_.isLocalFile()) {
    return KIO::NetAccess::download(url_, target_, window_);
  }

//  if(!KIO::NetAccess::exists(url_, true, window_)) {
//    myDebug() << "NetAccess::download() - does not exist: " << url_ << endl;
//    return false;
//  }

  if(target_.isEmpty()) {
    KTempFile tmpFile;
    target_ = tmpFile.name();
    if(!s_tmpFiles) {
      s_tmpFiles = new QStringList;
    }
    s_tmpFiles->append(target_);
  }

  KURL dest;
  dest.setPath(target_);

  KIO::Job* job = KIO::file_copy(url_, dest, -1, true /*overwrite*/, false /*resume*/, false /*showProgress*/);
  return KIO::NetAccess::synchronousRun(job, window_);
}

void NetAccess::removeTempFile(const QString& name_) {
  if(!s_tmpFiles) {
    return;
  }
  if(s_tmpFiles->contains(name_)) {
    ::unlink(QFile::encodeName(name_));
    s_tmpFiles->remove(name_);
  }
}

QPixmap NetAccess::filePreview(const KURL& url, int size) {
  NetAccess netaccess;

  KURL::List list;
  list.append(url);
  KIO::Job* previewJob = KIO::filePreview(list, size, size);
  connect(previewJob, SIGNAL(gotPreview(const KFileItem*, const QPixmap&)),
          &netaccess, SLOT(slotPreview(const KFileItem*, const QPixmap&)));

  KIO::NetAccess::synchronousRun(previewJob, Kernel::self()->widget());
  return netaccess.m_preview;
}

QPixmap NetAccess::filePreview(KFileItem* item, int size) {
  NetAccess netaccess;

  KFileItemList list;
  list.append(item);
  KIO::Job* previewJob = KIO::filePreview(list, size, size);
  connect(previewJob, SIGNAL(gotPreview(const KFileItem*, const QPixmap&)),
          &netaccess, SLOT(slotPreview(const KFileItem*, const QPixmap&)));

  KIO::NetAccess::synchronousRun(previewJob, Kernel::self()->widget());
  return netaccess.m_preview;
}

void NetAccess::slotPreview(const KFileItem*, const QPixmap& pix_) {
  m_preview = pix_;
}

#include "netaccess.moc"
