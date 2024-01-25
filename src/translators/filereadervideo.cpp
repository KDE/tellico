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

#include <config.h>

#include "filereadervideo.h"
#include "../fieldformat.h"
#include "../images/imagefactory.h"
#include "../core/netaccess.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KFileItem>

#include <QPixmap>
#include <QIcon>

namespace {
  static const int FILE_PREVIEW_SIZE = 128;
}

using Tellico::FileReaderVideo;

class FileReaderVideo::Private {
public:
  Private() {}

  // cache the icon image ids to avoid repeated creation of Data::Image objects
  QHash<QString, QString> iconImageId;
};

FileReaderVideo::FileReaderVideo(const QUrl& url_) : FileReaderMetaData(url_), d(new Private) {
}

FileReaderVideo::~FileReaderVideo() = default;

bool FileReaderVideo::populate(Data::EntryPtr entry, const KFileItem& item) {
  // reads pdf and ebooks
  if(!item.mimetype().startsWith(QLatin1String("video"))) {
    return false;
  }
#ifndef HAVE_KFILEMETADATA
  return false;
#else
  bool isEmpty = true;
  QStringList genres, keywords;
  const auto props = properties(item);
  for(auto it = props.constBegin(); it != props.constEnd(); ++it) {
    const QString value = it.value().toString();
    if(value.isEmpty()) continue;
    switch(it.key()) {
      case KFileMetaData::Property::Title:
        isEmpty = false; // require a title or author
        entry->setField(QStringLiteral("title"), value);
        break;

      case KFileMetaData::Property::Subject:
        keywords += value;
        break;

      case KFileMetaData::Property::Genre:
        genres += value;
        break;

      case KFileMetaData::Property::ReleaseYear:
        entry->setField(QStringLiteral("year"), value);
        break;

      case KFileMetaData::Property::AspectRatio:
        entry->setField(QStringLiteral("aspect-ratio"), value);
        break;

      case KFileMetaData::Property::Duration:
        entry->setField(QStringLiteral("running-time"), QString::number(value.toInt()/60));
        break;

      case KFileMetaData::Property::Description:
        entry->setField(QStringLiteral("plot"), value);
        break;

      default:
        if(!value.isEmpty()) {
          myDebug() << "skipping" << it.key() << it.value();
        }
        break;
    }
  }

  if(isEmpty) return false;

  if(!genres.isEmpty()) {
    entry->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));
  }
  if(!keywords.isEmpty()) {
    entry->setField(QStringLiteral("keyword"), keywords.join(FieldFormat::delimiterString()));
  }

  const QString url = QStringLiteral("url");
  if(!entry->collection()->hasField(url)) {
    Data::FieldPtr f(new Data::Field(url, i18n("URL"), Data::Field::URL));
    f->setCategory(i18n("Personal"));
    entry->collection()->addField(f);
  }
  entry->setField(url, item.url().url());

  const QString cover = QStringLiteral("cover");
  QPixmap pixmap;
  if(useFilePreview()) {
    pixmap = Tellico::NetAccess::filePreview(item, FILE_PREVIEW_SIZE);
  }
  if(pixmap.isNull()) {
    if(d->iconImageId.contains(item.iconName())) {
      entry->setField(cover, d->iconImageId.value(item.iconName()));
    } else {
      pixmap = QIcon::fromTheme(item.iconName()).pixmap(QSize(FILE_PREVIEW_SIZE, FILE_PREVIEW_SIZE));
      const QString id = ImageFactory::addImage(pixmap, QStringLiteral("PNG"));
      if(!id.isEmpty()) {
        entry->setField(cover, id);
        d->iconImageId.insert(item.iconName(), id);
      }
    }
  } else {
    const QString id = ImageFactory::addImage(pixmap, QStringLiteral("PNG"));
    if(!id.isEmpty()) {
      entry->setField(cover, id);
    }
  }

#endif
  return true;
}
