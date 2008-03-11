/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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
#include "../collection.h"
#include "../imagefactory.h"
#include "../image.h"
#include "../filehandler.h"
#include "../stringset.h"
#include "../tellico_debug.h"
#include "../progressmanager.h"

#include <klocale.h>
#include <kconfig.h>
#include <kzip.h>
#include <kapplication.h>

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
  m_cancelled = false;
  Data::CollPtr coll = collection();
  if(!coll) {
    return false;
  }

  // TODO: maybe need label?
  ProgressItem& item = ProgressManager::self()->newProgressItem(this, QString::null, true);
  item.setTotalSteps(100);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  ProgressItem::Done done(this);

  TellicoXMLExporter exp;
  exp.setEntries(entries());
  exp.setURL(url()); // needed in case of relative URL values
  long opt = options();
  opt |= Export::ExportUTF8; // always export to UTF-8
  opt |= Export::ExportImages; // always list the images in the xml
  opt &= ~Export::ExportProgress; // don't show progress for xml export
  exp.setOptions(opt);
  exp.setIncludeImages(false); // do not include the images themselves in XML
  QCString xml = exp.exportXML().toCString(); // encoded in utf-8
  ProgressManager::self()->setProgress(this, 5);

  QByteArray data;
  QBuffer buf(data);

  if(m_cancelled) {
    return true; // intentionally cancelled
  }

  KZip zip(&buf);
  zip.open(IO_WriteOnly);
  zip.writeFile(QString::fromLatin1("tellico.xml"), QString::null, QString::null, xml.length(), xml);

  if(m_includeImages) {
    ProgressManager::self()->setProgress(this, 10);
    // gonna be lazy and just increment progress every 3 images
    // it might be less, might be more
    uint j = 0;
    const QString imagesDir = QString::fromLatin1("images/");
    StringSet imageSet;
    Data::FieldVec imageFields = coll->imageFields();
    // already took 10%, only 90% left
    const uint stepSize = QMAX(1, (coll->entryCount() * imageFields.count()) / 90);
    for(Data::EntryVec::ConstIterator it = entries().begin(); it != entries().end() && !m_cancelled; ++it) {
      for(Data::FieldVec::Iterator fIt = imageFields.begin(); fIt != imageFields.end(); ++fIt, ++j) {
        const QString id = it->field(fIt);
        if(id.isEmpty() || imageSet.has(id)) {
          continue;
        }
        const Data::ImageInfo& info = ImageFactory::imageInfo(id);
        if(info.linkOnly) {
          myLog() << "TellicoZipExporter::exec() - not copying linked image: " << id << endl;
          continue;
        }
        const Data::Image& img = ImageFactory::imageById(id);
        // if no image, continue
        if(img.isNull()) {
          kdWarning() << "TellicoZipExporter::exec() - no image found for " << fIt->title() << " field" << endl;
          kdWarning() << "...for the entry titled " << it->title() << endl;
          continue;
        }
        QByteArray ba = img.byteArray();
//        myDebug() << "TellicoZipExporter::data() - adding image id = " << it->field(fIt) << endl;
        zip.writeFile(imagesDir + id, QString::null, QString::null, ba.size(), ba);
        imageSet.add(id);
        if(j%stepSize == 0) {
          ProgressManager::self()->setProgress(this, QMIN(10+j/stepSize, 99));
          kapp->processEvents();
        }
      }
    }
  } else {
    ProgressManager::self()->setProgress(this, 80);
  }

  zip.close();
  if(m_cancelled) {
    return true;
  }

  bool success = FileHandler::writeDataURL(url(), data, options() & Export::ExportForce);
  return success;
}

void TellicoZipExporter::slotCancel() {
  m_cancelled = true;
}

#include "tellicozipexporter.moc"
