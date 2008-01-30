/***************************************************************************
    copyright            : (C) 2007 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "referencerimporter.h"
#include "../collection.h"
#include "../core/netaccess.h"
#include "../imagefactory.h"

#include <kstandarddirs.h>

using Tellico::Import::ReferencerImporter;

ReferencerImporter::ReferencerImporter(const KURL& url_) : XSLTImporter(url_) {
  QString xsltFile = locate("appdata", QString::fromLatin1("referencer2tellico.xsl"));
  if(!xsltFile.isEmpty()) {
    KURL u;
    u.setPath(xsltFile);
    XSLTImporter::setXSLTURL(u);
  } else {
    kdWarning() << "ReferencerImporter() - unable to find referencer2tellico.xml!" << endl;
  }
}

bool ReferencerImporter::canImport(int type) const {
  return type == Data::Collection::Bibtex;
}

Tellico::Data::CollPtr ReferencerImporter::collection() {
  Data::CollPtr coll = XSLTImporter::collection();
  if(!coll) {
    return 0;
  }

  Data::FieldPtr field = coll->fieldByName(QString::fromLatin1("cover"));
  if(!field && !coll->imageFields().isEmpty()) {
    field = coll->imageFields().front();
  } else if(!field) {
    field = new Data::Field(QString::fromLatin1("cover"), i18n("Front Cover"), Data::Field::Image);
    coll->addField(field);
  }

  Data::EntryVec entries = coll->entries();
  for(Data::EntryVecIt entry = entries.begin(); entry != entries.end(); ++entry) {
    QString url = entry->field(QString::fromLatin1("url"));
    if(url.isEmpty()) {
      continue;
    }
    QPixmap pix = NetAccess::filePreview(url);
    if(pix.isNull()) {
      continue;
    }
    QString id = ImageFactory::addImage(pix, QString::fromLatin1("PNG"));
    if(id.isEmpty()) {
      continue;
    }
    entry->setField(field, id);
  }
  return coll;
}

#include "referencerimporter.moc"
