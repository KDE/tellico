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
#include "../imagefactory.h"
#include "../tellico_debug.h"

#include <kstandarddirs.h>

#include <qfile.h>

using Tellico::Import::DeliciousImporter;

DeliciousImporter::DeliciousImporter(const KURL& url_) : XSLTImporter(url_) {
  QString xsltFile = locate("appdata", QString::fromLatin1("delicious2tellico.xsl"));
  if(!xsltFile.isEmpty()) {
    KURL u;
    u.setPath(xsltFile);
    XSLTImporter::setXSLTURL(u);
  } else {
    kdWarning() << "DeliciousImporter() - unable to find delicious2tellico.xml!" << endl;
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

  KURL libraryDir = url();
  libraryDir.setPath(url().directory() + "Images/");
  const QStringList imageDirs = QStringList()
                              << QString::fromLatin1("Large Covers/")
                              << QString::fromLatin1("Medium Covers/")
                              << QString::fromLatin1("Small Covers/")
                              << QString::fromLatin1("Plain Covers/");
  const QString commField = QString::fromLatin1("comments");
  const QString uuidField = QString::fromLatin1("uuid");
  const QString coverField = QString::fromLatin1("cover");
  const bool isLocal = url().isLocalFile();

  Data::EntryVec entries = coll->entries();
  for(Data::EntryVecIt entry = entries.begin(); entry != entries.end(); ++entry) {
    QString comments = entry->field(commField);
    if(!comments.isEmpty()) {
      RTF2HTML rtf2html(comments);
      entry->setField(commField, rtf2html.toHTML());
    }

    //try to add images
    QString uuid = entry->field(uuidField);
    if(!uuid.isEmpty() && isLocal) {
      for(QStringList::ConstIterator it = imageDirs.begin(); it != imageDirs.end(); ++it) {
        QString imgPath = libraryDir.path() + *it + uuid;
        if(!QFile::exists(imgPath)) {
          continue;
        }
        QString imgID = ImageFactory::addImage(imgPath, true);
        if(!imgID.isEmpty()) {
          entry->setField(coverField, imgID);
        }
        break;
      }
    }
  }
  coll->removeField(uuidField);
  return coll;
}

#include "deliciousimporter.moc"
