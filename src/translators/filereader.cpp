/***************************************************************************
    Copyright (C) 2024 Robby Stephenson <robby@periapsis.org>
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

#include "filereader.h"
#include "../fieldformat.h"
#include "../images/imagefactory.h"
#include "../core/netaccess.h"

#include <Solid/Device>
#include <Solid/StorageVolume>
#include <Solid/StorageAccess>

#include <KFileItem>
#ifdef HAVE_KFILEMETADATA
#include <KFileMetaData/Extractor>
#include <KFileMetaData/SimpleExtractionResult>
#endif

#include <QDir>
#include <QIcon>

namespace {
  static const int FILE_PREVIEW_SIZE = 128;
}

using Tellico::FileReaderMetaData;
using Tellico::FileReaderFile;

#ifdef HAVE_KFILEMETADATA
#if KFILEMETADATA_VERSION > QT_VERSION_CHECK(5,240,0)
KFileMetaData::PropertyMultiMap FileReaderMetaData::properties(const KFileItem& item_) {
#else
KFileMetaData::PropertyMap FileReaderMetaData::properties(const KFileItem& item_) {
#endif
  KFileMetaData::SimpleExtractionResult result(item_.url().toLocalFile(),
                                               item_.mimetype(),
                                               KFileMetaData::ExtractionResult::ExtractMetaData);
  QList<KFileMetaData::Extractor*> exList = m_extractors.fetchExtractors(item_.mimetype());
  foreach(KFileMetaData::Extractor* ex, exList) {
// initializing exempi can cause a crash in Exiv for files with XMP data
// crude workaround is to avoid using the exivextractor and the only apparent way is to
// match against the mimetypes. Here, we use image/x-exv as the canary in the coal mine
// see https://bugs.kde.org/show_bug.cgi?id=390744
#ifdef HAVE_EXEMPI
    if(!ex->mimetypes().contains(QStringLiteral("image/x-exv"))) {
#else
    if(true) {
#endif
      ex->extract(&result);
    }
  }
#if KFILEMETADATA_VERSION > QT_VERSION_CHECK(5,240,0)
  return result.properties();
#elif KFILEMETADATA_VERSION >= QT_VERSION_CHECK(5,89,0)
  return result.properties(KFileMetaData::PropertiesMapType::MultiMap);
#else
  return result.properties();
#endif
}
#endif

class FileReaderFile::Private {
public:
  Private() {}

  QString volume;
  QStringList metaIgnore;
  // cache the icon image ids to avoid repeated creation of Data::Image objects
  QHash<QString, QString> iconImageId;
#ifdef HAVE_KFILEMETADATA
  QHash<KFileMetaData::Property::Property, QString> propertyNameHash;
#endif
};

FileReaderFile::FileReaderFile(const QUrl& url_) : FileReaderMetaData(url_), d(new Private) {
  // going to assume only one volume will ever be imported
  d->volume = volumeName();
  d->metaIgnore << QStringLiteral("mimeType")
                << QStringLiteral("url")
                << QStringLiteral("fileName")
                << QStringLiteral("lastModified")
                << QStringLiteral("contentSize")
                << QStringLiteral("type");
}

FileReaderFile::~FileReaderFile() = default;

bool FileReaderFile::populate(Data::EntryPtr entry, const KFileItem& item) {
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

  const QUrl u = item.url();
  entry->setField(title,  u.fileName());
  entry->setField(url,    u.url());
  entry->setField(desc,   item.mimeComment());
  entry->setField(vol,    d->volume);
  const QString folderPath = QDir(this->url().toLocalFile()).relativeFilePath(u.adjusted(QUrl::RemoveFilename|QUrl::StripTrailingSlash).path());
  // use empty string for root folder instead of "."
  entry->setField(folder, folderPath == QLatin1String(".") ? QString() : folderPath);
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
  QStringList strings;
  const auto props = properties(item);
  for(auto it = props.constBegin(); it != props.constEnd(); ++it) {
    const QString value = it.value().toString();
    if(!value.isEmpty()) {
      QString label;
      if(d->propertyNameHash.contains(it.key())) {
        label = d->propertyNameHash.value(it.key());
      } else {
        label = KFileMetaData::PropertyInfo(it.key()).displayName();
        d->propertyNameHash.insert(it.key(), label);
      }
      if(!d->metaIgnore.contains(label)) {
        strings << label + FieldFormat::columnDelimiterString() + value;
      }
    }
  }
  entry->setField(metainfo, strings.join(FieldFormat::rowDelimiterString()));
#endif

  QPixmap pixmap;
  if(useFilePreview()) {
    pixmap = Tellico::NetAccess::filePreview(item, FILE_PREVIEW_SIZE);
  }
  if(pixmap.isNull()) {
    if(d->iconImageId.contains(item.iconName())) {
      entry->setField(icon, d->iconImageId.value(item.iconName()));
    } else {
      pixmap = QIcon::fromTheme(item.iconName()).pixmap(QSize(FILE_PREVIEW_SIZE, FILE_PREVIEW_SIZE));
      const QString id = ImageFactory::addImage(pixmap, QStringLiteral("PNG"));
      if(!id.isEmpty()) {
        entry->setField(icon, id);
        d->iconImageId.insert(item.iconName(), id);
      }
    }
  } else {
    const QString id = ImageFactory::addImage(pixmap, QStringLiteral("PNG"));
    if(!id.isEmpty()) {
      entry->setField(icon, id);
    }
  }

  return true;
}

QString FileReaderFile::volumeName() const {
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
