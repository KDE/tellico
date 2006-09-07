/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "filelistingimporter.h"
#include "../collections/filecatalog.h"
#include "../entry.h"
#include "../field.h"
#include "../latin1literal.h"
#include "../imagefactory.h"
#include "../tellico_utils.h"
#include "../tellico_kernel.h"
#include "../progressmanager.h"

#include <kapplication.h>
#include <kmountpoint.h>
#include <kio/job.h>
#include <kio/previewjob.h>
#include <kio/netaccess.h>

#include <qcheckbox.h>
#include <qvgroupbox.h>
#include <qlayout.h>
#include <qwhatsthis.h>
#include <qfile.h>
#include <qfileinfo.h>

namespace {
  static const int FILE_PREVIEW_SIZE = 128;
}

using Tellico::Import::FileListingImporter;

FileListingImporter::FileListingImporter(const KURL& url_) : Importer(url_), m_coll(0), m_widget(0),
    m_job(0), m_cancelled(false) {
  m_files.setAutoDelete(true);
}

bool FileListingImporter::canImport(int type) const {
  return type == Data::Collection::File;
}

Tellico::Data::CollPtr FileListingImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, i18n("Scanning files..."), true);
  item.setTotalSteps(100);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  ProgressItem::Done done(this);

  // going to assume only one volume will ever be imported
  QString volume = volumeName();

  m_job = m_recursive->isChecked()
          ? KIO::listRecursive(url(), true, false)
          : KIO::listDir(url(), true, false);
  connect(m_job, SIGNAL(entries(KIO::Job*, const KIO::UDSEntryList&)),
          SLOT(slotEntries(KIO::Job*, const KIO::UDSEntryList&)));

  if(!KIO::NetAccess::synchronousRun(m_job, Kernel::self()->widget()) || m_cancelled) {
    return 0;
  }

  bool usePreview = m_filePreview->isChecked();

  const QString title    = QString::fromLatin1("title");
  const QString url      = QString::fromLatin1("url");
  const QString desc     = QString::fromLatin1("description");
  const QString vol      = QString::fromLatin1("volume");
  const QString folder   = QString::fromLatin1("folder");
  const QString type     = QString::fromLatin1("mimetype");
  const QString size     = QString::fromLatin1("size");
  const QString perm     = QString::fromLatin1("permissions");
  const QString owner    = QString::fromLatin1("owner");
  const QString group    = QString::fromLatin1("group");
  const QString created  = QString::fromLatin1("created");
  const QString modified = QString::fromLatin1("modified");
  const QString metainfo = QString::fromLatin1("metainfo");
  const QString icon     = QString::fromLatin1("icon");

  m_coll = new Data::FileCatalog(true);
  QString tmp;
  const uint stepSize = QMAX(1, m_files.count()/100);
  item.setTotalSteps(m_files.count());
  uint j = 0;
  for(KFileItemListIterator it(m_files); !m_cancelled && it.current(); ++it, ++j) {
    Data::EntryPtr entry = new Data::Entry(m_coll);

    const KURL u = it.current()->url();
    entry->setField(title,  u.fileName());
    entry->setField(url,    u.url());
    entry->setField(desc,   it.current()->mimeComment());
    entry->setField(vol,    volume);
    tmp = KURL::relativePath(this->url().path(), u.directory());
    // remove "./" from the string
    entry->setField(folder, tmp.right(tmp.length()-2));
    entry->setField(type,   it.current()->mimetype());
    entry->setField(size,   KIO::convertSize(it.current()->size()));
    entry->setField(perm,   it.current()->permissionsString());
    entry->setField(owner,  it.current()->user());
    entry->setField(group,  it.current()->group());

    time_t t = it.current()->time(KIO::UDS_CREATION_TIME);
    if(t > 0) {
      QDateTime dt;
      dt.setTime_t(t);
      entry->setField(created, dt.toString(Qt::ISODate));
    }
    t = it.current()->time(KIO::UDS_MODIFICATION_TIME);
    if(t > 0) {
      QDateTime dt;
      dt.setTime_t(t);
      entry->setField(modified, dt.toString(Qt::ISODate));
    }
    const KFileMetaInfo& meta = it.current()->metaInfo();
    if(meta.isValid() && !meta.isEmpty()) {
      const QStringList keys = meta.supportedKeys();
      QStringList strings;
      for(QStringList::ConstIterator it2 = keys.begin(); it2 != keys.end(); ++it2) {
        KFileMetaInfoItem item = meta.item(*it2);
        if(item.isValid()) {
          QString s = item.string();
          if(!s.isEmpty()) {
            strings << item.key() + "::" + s;
          }
        }
      }
      entry->setField(metainfo, strings.join(QString::fromLatin1("; ")));
    }

    if(!m_cancelled && usePreview) {
      m_pixmap = QPixmap();
      KFileItemList list;
      list.append(it.current());
      KIO::Job* previewJob = KIO::filePreview(list, FILE_PREVIEW_SIZE, FILE_PREVIEW_SIZE);
      connect(previewJob, SIGNAL(gotPreview(const KFileItem*, const QPixmap&)),
              this, SLOT(slotPreview(const KFileItem*, const QPixmap&)));

      if(!KIO::NetAccess::synchronousRun(previewJob, Kernel::self()->widget())) {
        m_pixmap = QPixmap();
      } else if(m_pixmap.isNull()) {
        m_pixmap = it.current()->pixmap(0);
      }
    } else {
      m_pixmap = it.current()->pixmap(0);
    }

    if(!m_pixmap.isNull()) {
      // is png best option?
      QString id = ImageFactory::addImage(m_pixmap, QString::fromLatin1("PNG"));
      if(!id.isEmpty()) {
        entry->setField(icon, id);
      }
    }

    m_coll->addEntry(entry);

    if(j%stepSize == 0) {
      ProgressManager::self()->setProgress(this, j);
      kapp->processEvents();
    }
  }

  if(m_cancelled) {
    m_coll = 0;
    return 0;
  }

  m_coll->updateDicts(m_coll->entries());
  return m_coll;
}

QWidget* FileListingImporter::widget(QWidget* parent_, const char* name_) {
  if(m_widget) {
    return m_widget;
  }

  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QVGroupBox* box = new QVGroupBox(i18n("File Listing Options"), m_widget);

  m_recursive = new QCheckBox(i18n("Recursive folder search"), box);
  QWhatsThis::add(m_recursive, i18n("If checked, folders are recursively searched for all files."));
  // by default, make it checked
  m_recursive->setChecked(true);

  m_filePreview = new QCheckBox(i18n("Generate file previews"), box);
  QWhatsThis::add(m_filePreview, i18n("If checked, previews of the file contents are generated, which can slow down "
                                      "the folder listing."));
  // by default, make it no previews
  m_filePreview->setChecked(false);

  l->addWidget(box);
  l->addStretch(1);
  return m_widget;
}

void FileListingImporter::slotEntries(KIO::Job* job_, const KIO::UDSEntryList& list_) {
  if(m_cancelled) {
    job_->kill();
    m_job = 0;
    return;
  }

  for(KIO::UDSEntryList::ConstIterator it = list_.begin(); it != list_.end(); ++it) {
    KFileItem* item = new KFileItem(*it, url(), false, true);
    if(item->isFile()) {
      m_files.append(item);
    } else {
      delete item;
    }
  }
}

void FileListingImporter::slotPreview(const KFileItem*, const QPixmap& pix_) {
  m_pixmap = pix_;
}

QString FileListingImporter::volumeName() const {
  QString volume;
  const KMountPoint::List mountPoints = KMountPoint::currentMountPoints(KMountPoint::NeedRealDeviceName);
  for(KMountPoint::List::ConstIterator it = mountPoints.begin(), end = mountPoints.end(); it != end; ++it) {
    if(url().path() == (*it)->mountPoint()) {
      volume = (*it)->mountPoint();
      if(!(*it)->realDeviceName().isEmpty()) {
        QString devName = (*it)->realDeviceName();
        if(devName.endsWith(QChar('/'))) {
          devName.truncate(devName.length()-1);
        }
        QFile dev(devName);
        if(dev.open(IO_ReadOnly)) {
          // can't seek, it's sequential
          for(uint i = 0; i < 32808; ++i) {
            dev.getch();
          }
          char buf[33];
          int ret = dev.readBlock(buf, 32);
          if(ret == 32) {
            buf[33] = '\0';
            volume = QString::fromLatin1(buf, 32).stripWhiteSpace();
          }
        }
      }
      break;
    }
  }
  return volume;
}

void FileListingImporter::slotCancel() {
  m_cancelled = true;
  if(m_job) {
    m_job->kill();
  }
}

#include "filelistingimporter.moc"
