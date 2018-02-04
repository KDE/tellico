/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#include <config.h>

#include "filelistingimporter.h"
#include "../collections/filecatalog.h"
#include "../entry.h"
#include "../field.h"
#include "../fieldformat.h"
#include "../images/imagefactory.h"
#include "../utils/tellico_utils.h"
#include "../utils/guiproxy.h"
#include "../progressmanager.h"
#include "../core/netaccess.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KJobWidgets/KJobWidgets>
#include <KIO/Job>
#include <Solid/Device>
#include <Solid/StorageVolume>
#include <Solid/StorageAccess>

#ifdef HAVE_KFILEMETADATA
#include <KFileMetaData/Extractor>
#include <KFileMetaData/ExtractorCollection>
#include <KFileMetaData/SimpleExtractionResult>
#include <KFileMetaData/PropertyInfo>
#endif

#include <QDate>
#include <QDir>
#include <QCheckBox>
#include <QGroupBox>
#include <QFile>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QApplication>

namespace {
  static const int FILE_PREVIEW_SIZE = 128;
}

using Tellico::Import::FileListingImporter;

FileListingImporter::FileListingImporter(const QUrl& url_) : Importer(url_), m_coll(nullptr), m_widget(nullptr),
    m_recursive(nullptr), m_filePreview(nullptr), m_job(nullptr), m_cancelled(false) {
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
  const QString volume = volumeName();

  // the importer might be running without a gui/widget
  m_job = (m_widget && m_recursive->isChecked())
          ? KIO::listRecursive(url(), KIO::DefaultFlags, false /* include hidden */)
          : KIO::listDir(url(), KIO::DefaultFlags, false /* include hidden */);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job, SIGNAL(entries(KIO::Job*, const KIO::UDSEntryList&)),
          SLOT(slotEntries(KIO::Job*, const KIO::UDSEntryList&)));

  if(!m_job->exec() || m_cancelled) {
    myDebug() << "did not run job:" << m_job->errorString();
    return Data::CollPtr();
  }

#ifdef HAVE_KFILEMETADATA
  KFileMetaData::ExtractorCollection extractors;
#endif

  QStringList metaIgnore = QStringList()
                         << QStringLiteral("mimeType")
                         << QStringLiteral("url")
                         << QStringLiteral("fileName")
                         << QStringLiteral("lastModified")
                         << QStringLiteral("contentSize")
                         << QStringLiteral("type");

  const bool usePreview = m_widget && m_filePreview->isChecked();

  const QString title    = QStringLiteral("title");
  const QString url      = QStringLiteral("url");
  const QString desc     = QStringLiteral("description");
  const QString vol      = QStringLiteral("volume");
  const QString folder   = QStringLiteral("folder");
  const QString type     = QStringLiteral("mimetype");
  const QString size     = QStringLiteral("size");
  const QString perm     = QStringLiteral("permissions");
  const QString owner    = QStringLiteral("owner");
  const QString group    = QStringLiteral("group");
  const QString created  = QStringLiteral("created");
  const QString modified = QStringLiteral("modified");
  const QString metainfo = QStringLiteral("metainfo");
  const QString icon     = QStringLiteral("icon");

  m_coll = new Data::FileCatalog(true);
  QString tmp;
  const uint stepSize = qMax(1, m_files.count()/100);
  const bool showProgress = options() & ImportProgress;

  item.setTotalSteps(m_files.count());
  uint j = 0;
  foreach(const KFileItem& item, m_files) {
    if(m_cancelled) {
      break;
    }

    Data::EntryPtr entry(new Data::Entry(m_coll));

    const QUrl u = item.url();
    entry->setField(title,  u.fileName());
    entry->setField(url,    u.url());
    entry->setField(desc,   item.mimeComment());
    entry->setField(vol,    volume);
    tmp = QDir(this->url().toLocalFile()).relativeFilePath(u.adjusted(QUrl::RemoveFilename|QUrl::StripTrailingSlash).path());
    // use empty string for root folder instead of "."
    entry->setField(folder, tmp == QLatin1String(".") ? QString() : tmp);
    entry->setField(type,   item.mimetype());
    entry->setField(size,   KIO::convertSize(item.size()));
    entry->setField(perm,   item.permissionsString());
    entry->setField(owner,  item.user());
    entry->setField(group,  item.group());

    QDateTime dt(item.time(KFileItem::CreationTime));
    if(!dt.isNull()) {
      entry->setField(created, dt.date().toString(Qt::ISODate));
    }
    dt = QDateTime(item.time(KFileItem::ModificationTime));
    if(!dt.isNull()) {
      entry->setField(modified, dt.date().toString(Qt::ISODate));
    }

#ifdef HAVE_KFILEMETADATA
    KFileMetaData::SimpleExtractionResult result(u.url(), item.mimetype(), KFileMetaData::ExtractionResult::ExtractMetaData);
    QList<KFileMetaData::Extractor*> exList = extractors.fetchExtractors(item.mimetype());
    foreach(KFileMetaData::Extractor* ex, exList) {
      ex->extract(&result);
    }
    QStringList strings;
    KFileMetaData::PropertyMap properties = result.properties();
    KFileMetaData::PropertyMap::const_iterator it = properties.constBegin();
    for( ; it != properties.constEnd(); ++it) {
      const QString s = it.value().toString();
      if(!s.isEmpty()) {
        const QString label = KFileMetaData::PropertyInfo(it.key()).displayName();
        if(!metaIgnore.contains(label)) {
          strings << label + FieldFormat::columnDelimiterString() + s;
        }
      }
    }
    entry->setField(metainfo, strings.join(FieldFormat::rowDelimiterString()));
#endif

    if(!m_cancelled && usePreview) {
      m_pixmap = Tellico::NetAccess::filePreview(item, FILE_PREVIEW_SIZE);
      if(m_pixmap.isNull()) {
        m_pixmap = QIcon::fromTheme(item.iconName()).pixmap(QSize(FILE_PREVIEW_SIZE, FILE_PREVIEW_SIZE));
      }
    } else {
      m_pixmap = QIcon::fromTheme(item.iconName()).pixmap(QSize(FILE_PREVIEW_SIZE, FILE_PREVIEW_SIZE));
    }

    if(!m_pixmap.isNull()) {
      // is png best option?
      const QString id = ImageFactory::addImage(m_pixmap, QStringLiteral("PNG"));
      if(!id.isEmpty()) {
        entry->setField(icon, id);
      }
    }

    m_coll->addEntries(entry);

    if(showProgress && j%stepSize == 0) {
      ProgressManager::self()->setProgress(this, j);
      qApp->processEvents();
    }
    ++j;
  }

  if(m_cancelled) {
    m_coll = Data::CollPtr();
    return m_coll;
  }

  return m_coll;
}

QWidget* FileListingImporter::widget(QWidget* parent_) {
  if(m_widget) {
    return m_widget;
  }

  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* gbox = new QGroupBox(i18n("File Listing Options"), m_widget);
  QVBoxLayout* vlay = new QVBoxLayout(gbox);

  m_recursive = new QCheckBox(i18n("Recursive folder search"), gbox);
  m_recursive->setWhatsThis(i18n("If checked, folders are recursively searched for all files."));
  // by default, make it checked
  m_recursive->setChecked(true);

  m_filePreview = new QCheckBox(i18n("Generate file previews"), gbox);
  m_filePreview->setWhatsThis(i18n("If checked, previews of the file contents are generated, which can slow down "
                                      "the folder listing."));
  // by default, make it no previews
  m_filePreview->setChecked(false);

  vlay->addWidget(m_recursive);
  vlay->addWidget(m_filePreview);

  l->addWidget(gbox);
  l->addStretch(1);
  return m_widget;
}

void FileListingImporter::slotEntries(KIO::Job* job_, const KIO::UDSEntryList& list_) {
  if(m_cancelled) {
    job_->kill();
    m_job = nullptr;
    return;
  }

  for(KIO::UDSEntryList::ConstIterator it = list_.begin(); it != list_.end(); ++it) {
    KFileItem item(*it, url(), false, true);
    if(item.isFile()) {
      m_files.append(item);
    }
  }
}

QString FileListingImporter::volumeName() const {
  const QString filePath = url().toLocalFile();
  QString matchingPath, volume;
  QList<Solid::Device> devices = Solid::Device::listFromType(Solid::DeviceInterface::StorageVolume, QString());
  foreach(const Solid::Device& device, devices) {
    const Solid::StorageAccess* acc = device.as<const Solid::StorageAccess>();
    if(acc && !acc->filePath().isEmpty() && filePath.startsWith(acc->filePath())) {
      // it might be possible for one volume to be mounted at /dir1 and another to be mounted at /dir1/dir2
      // so we need to find the longest natching filePath
      if(acc->filePath().length() > matchingPath.length()) {
        matchingPath = acc->filePath();
        const Solid::StorageVolume* vol = device.as<const Solid::StorageVolume>();
        if(vol) {
          volume = vol->label();
        }
      }
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
