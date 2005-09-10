/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "tellicozipexporter.h"
#include "tellicoxmlexporter.h"
#include "../document.h"
#include "../collection.h"
#include "../imagefactory.h"
#include "../filehandler.h"
#include "../stringset.h"

#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kzip.h>

#include <qdom.h>
#include <qbuffer.h>

using Tellico::Export::TellicoZipExporter;

QString TellicoZipExporter::formatString() const {
  return i18n("Tellico Zip File");
}

QString TellicoZipExporter::fileFilter() const {
  return i18n("*.tc *.bc|Tellico Files (*.tc)") + QChar('\n') + i18n("*|All Files");
}

bool TellicoZipExporter::exec() {
  const Data::Collection* coll = Data::Document::self()->collection();
  if(!coll) {
    return false;
  }

  TellicoXMLExporter exp;
  exp.setEntries(entries());
  exp.setOptions(options() | Export::ExportUTF8); // always export to UTF-8
  exp.setIncludeImages(false); // do not include images in XML
  QCString xml = exp.exportXML().toCString(); // encoded in utf-8

  QByteArray data;
  QBuffer buf(data);

  KZip zip(&buf);
  zip.open(IO_WriteOnly);
  zip.writeFile(QString::fromLatin1("tellico.xml"), QString::null, QString::null, xml.length(), xml);

  QStringList imageFields;
  Data::FieldVec fields = coll->fields();
  for(Data::FieldVec::Iterator it = fields.begin(); it != fields.end(); ++it) {
    if(it->type() == Data::Field::Image) {
      imageFields += it->name();
    }
  }

  StringSet imageSet;
  for(Data::EntryVec::ConstIterator it = entries().begin(); it != entries().end(); ++it) {
    for(QStringList::ConstIterator fIt = imageFields.begin(); fIt != imageFields.end(); ++fIt) {
      const Data::Image& img = ImageFactory::imageById(it->field(*fIt));
      // if no image or is already writen, continue
      if(img.isNull() || imageSet.has(img.id())) {
        continue;
      }
      QByteArray ba = img.byteArray();
//      kdDebug() << "TellicoZipExporter::data() - writing image id = " << it.current()->field(*fIt) << endl;
      zip.writeFile(QString::fromLatin1("images/") + it->field(*fIt),
                    QString::null, QString::null, ba.size(), ba);
      imageSet.add(img.id());
    }
  }

  zip.close();
  return FileHandler::writeDataURL(url(), data, options() & Export::ExportForce);
}
