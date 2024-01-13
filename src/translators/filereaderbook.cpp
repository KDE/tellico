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

#include "filereaderbook.h"
#include "../fieldformat.h"
#include "../images/imagefactory.h"
#include "../core/netaccess.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KFileItem>
#ifdef HAVE_KFILEMETADATA
#include <KFileMetaData/Extractor>
#include <KFileMetaData/ExtractorCollection>
#include <KFileMetaData/SimpleExtractionResult>
#include <KFileMetaData/PropertyInfo>
// kfilemetadata_version.h was added in 5.94, so first use kcoreaddons_version to check
// with the expectation that the two versions should match or be no less than
#include <kcoreaddons_version.h>
#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(5,94,0)
#include <kfilemetadata_version.h>
#endif
#endif

#include <QPixmap>
#include <QIcon>

namespace {
  static const int FILE_PREVIEW_SIZE = 128;
}

using Tellico::FileReaderBook;

class FileReaderBook::Private {
public:
  Private() : useFilePreview(false) {}

  bool useFilePreview;
  // cache the icon image ids to avoid repeated creation of Data::Image objects
  QHash<QString, QString> iconImageId;
#ifdef HAVE_KFILEMETADATA
  KFileMetaData::ExtractorCollection extractors;
  QHash<KFileMetaData::Property::Property, QString> propertyNameHash;
#endif
};

FileReaderBook::FileReaderBook(const QUrl& url_) : AbstractFileReader(url_), d(new Private) {
}

FileReaderBook::~FileReaderBook() = default;

void FileReaderBook::setUseFilePreview(bool filePreview_) {
  d->useFilePreview = filePreview_;
}

bool FileReaderBook::populate(Data::EntryPtr entry, const KFileItem& item) {
  // reads pdf and ebooks
  if(item.mimetype() != QLatin1String("application/pdf") &&
     item.mimetype() != QLatin1String("application/epub+zip") &&
     item.mimetype() != QLatin1String("application/fb2+zip") &&
     item.mimetype() != QLatin1String("application/fb2+xml") &&
     item.mimetype() != QLatin1String("application/x-mobipocket-ebook")) {
    return false;
  }
#ifdef HAVE_KFILEMETADATA
  auto exList = d->extractors.fetchExtractors(item.mimetype());
  if(exList.isEmpty()) {
    return false;
  }

  KFileMetaData::SimpleExtractionResult result(item.url().toLocalFile(),
                                               item.mimetype(),
                                               KFileMetaData::ExtractionResult::ExtractMetaData);
  foreach(auto ex, exList) {
    ex->extract(&result);
  }
  bool isEmpty = true;
  QStringList authors, publishers, genres, keywords;
#if KFILEMETADATA_VERSION >= QT_VERSION_CHECK(5,89,0)
  auto properties = result.properties(KFileMetaData::PropertiesMapType::MultiMap);
#else
  auto properties = result.properties();
#endif
  auto it = properties.constBegin();
  for( ; it != properties.constEnd(); ++it) {
    const QString value = it.value().toString();
    if(value.isEmpty()) continue;
    switch(it.key()) {
      case KFileMetaData::Property::Title:
        isEmpty = false; // require a title or author
        entry->setField(QStringLiteral("title"), value);
        break;

      case KFileMetaData::Property::Author:
        isEmpty = false; // require a title or author
        authors += value;
        break;

      case KFileMetaData::Property::Publisher:
        publishers += value;
        break;

      case KFileMetaData::Property::Subject:
        keywords += value;
        break;

      case KFileMetaData::Property::Genre:
        genres += value;
        break;

      case KFileMetaData::Property::ReleaseYear:
        entry->setField(QStringLiteral("pub_year"), value);
        break;

      // is description usually the plot or just comments?
      case KFileMetaData::Property::Description:
        entry->setField(QStringLiteral("plot"), value);
        break;

      case KFileMetaData::Property::PageCount:
        entry->setField(QStringLiteral("pages"), value);
        break;

      default:
        if(!value.isEmpty()) {
//          myDebug() << "skipping" << it.key() << it.value();
        }
        break;
    }
  }

  if(isEmpty) return false;

  if(!authors.isEmpty()) {
    entry->setField(QStringLiteral("author"), authors.join(FieldFormat::delimiterString()));
  }
  if(!publishers.isEmpty()) {
    entry->setField(QStringLiteral("publisher"), publishers.join(FieldFormat::delimiterString()));
  }
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
  if(d->useFilePreview) {
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
