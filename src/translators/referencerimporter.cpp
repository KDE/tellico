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
#include "../tellico_debug.h"

#include <kstandarddirs.h>

#include <QPixmap>

using Tellico::Import::ReferencerImporter;

ReferencerImporter::ReferencerImporter(const KUrl& url_) : XSLTImporter(url_) {
  QString xsltFile = KStandardDirs::locate("appdata", QLatin1String("referencer2tellico.xsl"));
  if(!xsltFile.isEmpty()) {
    KUrl u;
    u.setPath(xsltFile);
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

  Data::FieldPtr field = coll->fieldByName(QLatin1String("cover"));
  if(!field && !coll->imageFields().isEmpty()) {
    field = coll->imageFields().front();
  } else if(!field) {
    field = new Data::Field(QLatin1String("cover"), i18n("Front Cover"), Data::Field::Image);
    coll->addField(field);
  }

  foreach(Data::EntryPtr entry, coll->entries()) {
    QString url = entry->field(QLatin1String("url"));
    if(url.isEmpty()) {
      continue;
    }
    QPixmap pix = NetAccess::filePreview(url);
    if(pix.isNull()) {
      continue;
    }
    QString id = ImageFactory::addImage(pix, QLatin1String("PNG"));
    if(id.isEmpty()) {
      continue;
    }
    entry->setField(field, id);
  }
  return coll;
}

#include "referencerimporter.moc"
