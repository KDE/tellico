/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bookcasezipexporter.h"
#include "bookcasexmlexporter.h"
#include "../collection.h"
#include "../imagefactory.h"

#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kzip.h>

#include <qbuffer.h>

using Bookcase::Export::BookcaseZipExporter;

BookcaseZipExporter::BookcaseZipExporter(const Data::Collection* coll_, const Data::EntryList& list_)
    : Bookcase::Export::DataExporter(coll_, list_) {
}

QString BookcaseZipExporter::formatString() const {
  return i18n("Bookcase Zip File");
}

QString BookcaseZipExporter::fileFilter() const {
  return i18n("*.bcz|Bookcase files(*.bcz)") + QString::fromLatin1("\n") + i18n("*|All files");
}

QByteArray BookcaseZipExporter::data(bool formatFields_) {
  const Data::Collection* coll = collection();
  BookcaseXMLExporter exp(coll, entryList());
  exp.exportImages(false);
  QCString xml = exp.text(formatFields_, true).utf8();

  QByteArray data;
  QBuffer buf(data);

  KZip zip(&buf);
  zip.open(IO_WriteOnly);
  zip.writeFile(QString::fromLatin1("bookcase.xml"), QString::null, QString::null, xml.length(), xml);

  QStringList imageFields;
  for(Data::FieldListIterator it(coll->fieldList()); it.current(); ++it) {
    if(it.current()->type() == Data::Field::Image) {
      imageFields += it.current()->name();
    }
  }

  // use a dict for fast random access to keep track of which images were written to the file
  QDict<int> imageDict;
  for(Data::EntryListIterator it(coll->entryList()); it.current(); ++it) {
    for(QStringList::ConstIterator fIt = imageFields.begin(); fIt != imageFields.end(); ++fIt) {
      const Data::Image& img = ImageFactory::imageById(it.current()->field(*fIt));
      if(!img.isNull() && !imageDict[img.id()]) {
        QByteArray ba = img.byteArray();
//        kdDebug() << "BookcaseZipExporter::data() - writing image id = " << it.current()->field(*fIt) << endl;
        zip.writeFile(QString::fromLatin1("images/") + it.current()->field(*fIt),
                      QString::null, QString::null, ba.size(), ba);
        imageDict.insert(img.id(), reinterpret_cast<const int *>(1));
      }
    }
  }

  zip.close();
  return data;
}
