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

#include "deliciousimporter.h"
#include "../collection.h"
#include "../rtf2html/rtf2html.h"

#include <kstandarddirs.h>

using Tellico::Import::DeliciousImporter;

DeliciousImporter::DeliciousImporter(const KURL& url_) : XSLTImporter(url_) {
  QString xsltFile = locate("appdata", QString::fromLatin1("delicious2tellico.xsl"));
  if(!xsltFile.isEmpty()) {
    KURL u;
    u.setPath(xsltFile);
    XSLTImporter::setXSLTURL(u);
  } else {
    kdWarning() << "DeliciousImporter() - unable to find referencer2tellico.xml!" << endl;
  }
}

bool DeliciousImporter::canImport(int type) const {
  return type == Data::Collection::Book;
}

Tellico::Data::CollPtr DeliciousImporter::collection() {
  Data::CollPtr coll = XSLTImporter::collection();
  if(!coll) {
    return 0;
  }

  Data::EntryVec entries = coll->entries();
  for(Data::EntryVecIt entry = entries.begin(); entry != entries.end(); ++entry) {
    QString comments = entry->field(QString::fromLatin1("comments"));
    if(comments.isEmpty()) {
      continue;
    }
    RTF2HTML rtf2html(comments);
    entry->setField(QString::fromLatin1("comments"), rtf2html.toHTML());
  }
  return coll;
}

#include "deliciousimporter.moc"
