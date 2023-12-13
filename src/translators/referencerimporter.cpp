/***************************************************************************
    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
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

#include "referencerimporter.h"
#include "../collection.h"
#include "../core/netaccess.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

#include <QPixmap>

using Tellico::Import::ReferencerImporter;

ReferencerImporter::ReferencerImporter(const QUrl& url_) : XSLTImporter(url_) {
  QString xsltFile = DataFileRegistry::self()->locate(QStringLiteral("referencer2tellico.xsl"));
  if(!xsltFile.isEmpty()) {
    QUrl u = QUrl::fromLocalFile(xsltFile);
    XSLTImporter::setXSLTURL(u);
  } else {
    myWarning() << "unable to find referencer2tellico.xsl!";
  }
}

bool ReferencerImporter::canImport(int type) const {
  return type == Data::Collection::Bibtex;
}

Tellico::Data::CollPtr ReferencerImporter::collection() {
  Data::CollPtr coll = XSLTImporter::collection();
  if(!coll) {
    return Data::CollPtr();
  }

  Data::FieldPtr field = coll->fieldByName(QStringLiteral("cover"));
  if(!field && !coll->imageFields().isEmpty()) {
    field = coll->imageFields().front();
  } else if(!field) {
    field = Data::Field::createDefaultField(Data::Field::FrontCoverField);
    coll->addField(field);
  }

  foreach(Data::EntryPtr entry, coll->entries()) {
    QString url = entry->field(QStringLiteral("url"));
    if(url.isEmpty()) {
      continue;
    }
    QPixmap pix = NetAccess::filePreview(QUrl::fromLocalFile(url));
    if(pix.isNull()) {
      continue;
    }
    QString id = ImageFactory::addImage(pix, QStringLiteral("PNG"));
    if(id.isEmpty()) {
      continue;
    }
    entry->setField(field, id);
  }
  return coll;
}
