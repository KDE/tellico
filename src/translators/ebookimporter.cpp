/***************************************************************************
    Copyright (C) 2023 Robby Stephenson <robby@periapsis.org>
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

#include "ebookimporter.h"
#include "../collections/bookcollection.h"
#include "../core/netaccess.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KFileItem>
#ifdef HAVE_KFILEMETADATA
#include <KFileMetaData/Extractor>
#include <KFileMetaData/ExtractorCollection>
#include <KFileMetaData/SimpleExtractionResult>
#include <KFileMetaData/PropertyInfo>
#endif

#include <QPixmap>

namespace {
  static const int FILE_PREVIEW_SIZE = 196; // same as in pdfimporter.cpp
}

using Tellico::Import::EBookImporter;

EBookImporter::EBookImporter(const QUrl& url_) : Importer(url_) {
}

EBookImporter::EBookImporter(const QList<QUrl>& urls_) : Importer(urls_) {
}

bool EBookImporter::canImport(int type) const {
  return type == Data::Collection::Book;
}

Tellico::Data::CollPtr EBookImporter::collection() {
  Data::CollPtr coll(new Data::BookCollection(true));
#ifdef HAVE_KFILEMETADATA
  KFileMetaData::ExtractorCollection extractors;
  foreach(const QUrl& url, urls()) {
    KFileItem item(url);
//    myDebug() << "Reading" << url.url() << item.mimetype();
    KFileMetaData::SimpleExtractionResult result(url.toLocalFile(),
                                                 item.mimetype(),
                                                 KFileMetaData::ExtractionResult::ExtractMetaData);
    auto exList = extractors.fetchExtractors(item.mimetype());
    if(exList.isEmpty()) continue;
    foreach(auto ex, exList) {
      ex->extract(&result);
    }
    bool isEmpty = true;
    Data::EntryPtr entry(new Data::Entry(coll));
    entry->setField(QStringLiteral("comments"), url.toLocalFile());
    QStringList authors, publishers, genres, keywords;
    KFileMetaData::PropertyMap properties = result.properties();
    KFileMetaData::PropertyMap::const_iterator it = properties.constBegin();
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

        default:
          if(!value.isEmpty()) {
            myDebug() << "skipping" << it.key() << it.value();
          }
          break;
      }
    }
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
    if(!isEmpty) {
      QPixmap pix = NetAccess::filePreview(url, FILE_PREVIEW_SIZE);
      if(pix.isNull()) {
        myDebug() << "No file preview from pdf";
      } else {
        // is png best option?
        QString id = ImageFactory::addImage(pix, QStringLiteral("PNG"));
        if(!id.isEmpty()) {
          Data::FieldPtr field = coll->fieldByName(QStringLiteral("cover"));
          if(!field && !coll->imageFields().isEmpty()) {
            field = coll->imageFields().front();
          } else if(!field) {
            field = new Data::Field(QStringLiteral("cover"), i18n("Front Cover"), Data::Field::Image);
            coll->addField(field);
          }
          entry->setField(field, id);
        }
      }
      coll->addEntries(entry);
     }
  }
#endif
  return coll;
}

void EBookImporter::slotCancel() {
  myDebug() << "EBookImporter::slotCancel() - unimplemented";
}
