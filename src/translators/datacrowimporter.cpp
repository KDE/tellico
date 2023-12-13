/***************************************************************************
    Copyright (C) 2022 Robby Stephenson <robby@periapsis.org>
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

#include "datacrowimporter.h"
#include "../collection.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

#include <QFile>
#include <QDir>

using Tellico::Import::DataCrowImporter;

DataCrowImporter::DataCrowImporter(const QUrl& url_) : XSLTImporter(url_) {
  QString xsltFile = DataFileRegistry::self()->locate(QStringLiteral("datacrow2tellico.xsl"));
  if(!xsltFile.isEmpty()) {
    QUrl u = QUrl::fromLocalFile(xsltFile);
    XSLTImporter::setXSLTURL(u);
  } else {
    myWarning() << "unable to find datacrow2tellico.xml!";
  }
}

bool DataCrowImporter::canImport(int type) const {
  return type == Data::Collection::Book ||
         type == Data::Collection::Video ||
         type == Data::Collection::Album;
}

Tellico::Data::CollPtr DataCrowImporter::collection() {
  Data::CollPtr coll = XSLTImporter::collection();
  if(!coll) {
    return Data::CollPtr();
  }

  // only load images from local file
  if(!url().isLocalFile()) {
    return coll;
  }

  const QString baseDir = url().adjusted(QUrl::RemoveFilename).toLocalFile();
  const QString coverField(QStringLiteral("cover"));

  // load images
  foreach(Data::EntryPtr entry, coll->entries()) {
    QString cover = entry->field(coverField);
    if(cover.isEmpty() || !cover.contains(QLatin1Char('/'))) {
      continue;
    }

    // quite possible cover path includes window separator
    const QString imgPath = baseDir + cover.replace(QLatin1Char('\\'),
                                                    QLatin1Char('/'),
                                                    Qt::CaseInsensitive);
    if(!QFile::exists(imgPath)) {
      myDebug() << "failed to find" << imgPath;
      continue;
    }
    QString imgID = ImageFactory::addImage(QUrl::fromLocalFile(imgPath), true);
    if(!imgID.isEmpty()) {
      entry->setField(coverField, imgID, false /* no change to mdate */);
    }
  }

  return coll;
}
