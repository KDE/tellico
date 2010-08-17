/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include "tellicozipexporter.h"
#include "tellicoxmlexporter.h"
#include "../collection.h"
#include "../images/imagefactory.h"
#include "../images/image.h"
#include "../images/imageinfo.h"
#include "../core/filehandler.h"
#include "../utils/stringset.h"
#include "../tellico_debug.h"
#include "../progressmanager.h"

#include <klocale.h>
#include <kconfig.h>
#include <kzip.h>
#include <kapplication.h>

#include <qdom.h>
#include <QBuffer>

using namespace Tellico;
using Tellico::Export::TellicoZipExporter;

TellicoZipExporter::TellicoZipExporter(Data::CollPtr coll) : Exporter(coll)
    , m_includeImages(true), m_cancelled(false) {
}

QString TellicoZipExporter::formatString() const {
  return i18n("Tellico Zip File");
}

QString TellicoZipExporter::fileFilter() const {
  return i18n("*.tc *.bc|Tellico Files (*.tc)") + QLatin1Char('\n') + i18n("*|All Files");
}

bool TellicoZipExporter::exec() {
  m_cancelled = false;
  Data::CollPtr coll = collection();
  if(!coll) {
    return false;
  }

  // TODO: maybe need label?
  ProgressItem& item = ProgressManager::self()->newProgressItem(this, QString(), true);
  item.setTotalSteps(100);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  ProgressItem::Done done(this);

  TellicoXMLExporter exp(coll);
  exp.setEntries(entries());
  exp.setFields(fields());
  exp.setURL(url()); // needed in case of relative URL values
  long opt = options();
  opt |= Export::ExportUTF8; // always export to UTF-8
  opt |= Export::ExportImages; // always list the images in the xml
  opt &= ~Export::ExportProgress; // don't show progress for xml export
  exp.setOptions(opt);
  exp.setIncludeImages(false); // do not include the images themselves in XML
  QByteArray xml = exp.exportXML().toByteArray(); // encoded in utf-8
  ProgressManager::self()->setProgress(this, 5);

  QByteArray data;
  QBuffer buf(&data);

  if(m_cancelled) {
    return true; // intentionally cancelled
  }

  KZip zip(&buf);
  zip.open(QIODevice::WriteOnly);
  zip.writeFile(QLatin1String("tellico.xml"), QString(), QString(), xml, xml.size());

  if(m_includeImages) {
    ProgressManager::self()->setProgress(this, 10);
    // gonna be lazy and just increment progress every 3 images
    // it might be less, might be more
    int j = 0;
    const QString imagesDir = QLatin1String("images/");
    StringSet imageSet;
    Data::FieldList imageFields = coll->imageFields();
    // take intersection with the fields to be exported
    imageFields = QSet<Data::FieldPtr>::fromList(imageFields).intersect(fields().toSet()).toList();
    // already took 10%, only 90% left
    const int stepSize = qMax(1, (coll->entryCount() * imageFields.count()) / 90);
    foreach(Data::EntryPtr entry, entries()) {
      if(m_cancelled) {
        break;
      }
      foreach(Data::FieldPtr imageField, imageFields) {
        const QString id = entry->field(imageField);
        if(id.isEmpty() || imageSet.has(id)) {
          continue;
        }
        const Data::ImageInfo& info = ImageFactory::imageInfo(id);
        if(info.linkOnly) {
          myLog() << "not copying linked image: " << id;
          continue;
        }
        const Data::Image& img = ImageFactory::imageById(id);
        // if no image, continue
        if(img.isNull()) {
          myWarning() << "no image found for " << imageField->title() << " field";
          myWarning() << "...for the entry titled " << entry->title();
          continue;
        }
        QByteArray ba = img.byteArray();
//        myDebug() << "adding image id = " << it->field(fIt);
        zip.writeFile(imagesDir + id, QString(), QString(), ba, ba.size());
        imageSet.add(id);
        if(j%stepSize == 0) {
          ProgressManager::self()->setProgress(this, qMin(10+j/stepSize, 99));
          kapp->processEvents();
        }
        ++j;
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
