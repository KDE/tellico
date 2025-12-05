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
#include "../tellico_debug.h"
#include "../progressmanager.h"

#include <KLocalizedString>
#include <KZip>

#include <QDomDocument>
#include <QBuffer>
#include <QApplication>

using namespace Tellico;
using Tellico::Export::TellicoZipExporter;

TellicoZipExporter::TellicoZipExporter(Data::CollPtr coll, const QUrl& baseUrl_)
  : Exporter(coll, baseUrl_),
    m_includeImages(true),
    m_cancelled(false) {
}

QString TellicoZipExporter::formatString() const {
  return QStringLiteral("Zip");
}

QString TellicoZipExporter::fileFilter() const {
  return i18n("Tellico Files") + QLatin1String(" (*.tc *.bc)") + QLatin1String(";;") + i18n("All Files") + QLatin1String(" (*)");
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
  connect(&item, &Tellico::ProgressItem::signalCancelled, this, &Tellico::Export::TellicoZipExporter::slotCancel);
  ProgressItem::Done done(this);

  TellicoXMLExporter exp(coll, baseUrl());
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
  if(!zip.open(QIODevice::WriteOnly)) {
    return false;
  }
  zip.writeFile(QStringLiteral("tellico.xml"), xml);

  if(m_includeImages) {
    ProgressManager::self()->setProgress(this, 10);
    const QString imagesDir = QStringLiteral("images/");
    StringSet imageSet;
    // take intersection with the fields to be exported
    Data::FieldList imageFields = Tellico::listIntersection(coll->imageFields(), fields());
    // already took 10%, only 90% left
    const int stepSize = qMax(1, (coll->entryCount() * imageFields.count()) / 90);
    int j = 0;
    foreach(Data::EntryPtr entry, entries()) {
      if(m_cancelled) {
        break;
      }
      foreach(Data::FieldPtr imageField, imageFields) {
        const QString id = entry->field(imageField);
        if(id.isEmpty() || imageSet.contains(id)) {
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
        zip.writeFile(imagesDir + id, ba);
        imageSet.add(id);
        if(j%stepSize == 0) {
          ProgressManager::self()->setProgress(this, qMin(10+j/stepSize, 99));
          qApp->processEvents();
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

  return FileHandler::writeDataURL(url(), data, options() & Export::ExportForce);
}

void TellicoZipExporter::slotCancel() {
  m_cancelled = true;
}
