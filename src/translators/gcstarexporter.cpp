/***************************************************************************
    Copyright (C) 2009-2022 Robby Stephenson <robby@periapsis.org>
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

#include "gcstarexporter.h"
#include "xslthandler.h"
#include "tellicoxmlexporter.h"
#include "../collection.h"
#include "../core/filehandler.h"
#include "../images/imagefactory.h"
#include "../images/image.h"
#include "../progressmanager.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

#include <QDomDocument>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QStandardPaths>
#include <QApplication>

using Tellico::Export::GCstarExporter;

GCstarExporter::GCstarExporter(Tellico::Data::CollPtr coll_) : Tellico::Export::Exporter(coll_),
    m_handler(nullptr),
    m_xsltFile(QStringLiteral("tellico2gcstar.xsl")) {
}

GCstarExporter::~GCstarExporter() {
  delete m_handler;
  m_handler = nullptr;
}

QString GCstarExporter::formatString() const {
  return QStringLiteral("GCstar");
}

QString GCstarExporter::fileFilter() const {
  return i18n("GCstar Data Files") + QLatin1String(" (*.gcs)") + QLatin1String(";;") + i18n("All Files") + QLatin1String(" (*)");
}

bool GCstarExporter::exec() {
  const QString text = this->text();

  bool success = true;
  if(options() & ExportImages) {
    writeImages();
  }
  return !text.isEmpty() &&
         FileHandler::writeTextURL(url(), text, options() & ExportUTF8, options() & Export::ExportForce) &&
         success;
}

QString GCstarExporter::text() {
  QString xsltFile = DataFileRegistry::self()->locate(m_xsltFile);
  if(xsltFile.isNull()) {
    myDebug() << "no xslt file for " << m_xsltFile;
    return QString();
  }

  Data::CollPtr coll = collection();
  if(!coll) {
    myDebug() << "no collection pointer!";
    return QString();
  }

  // notes about utf-8 encoding:
  // all params should be passed to XSLTHandler in utf8
  // input string to XSLTHandler should be in utf-8, EVEN IF DOM STRING SAYS OTHERWISE

  QUrl u = QUrl::fromLocalFile(xsltFile);
  // do NOT do namespace processing, it messes up the XSL declaration since
  // QDom thinks there are no elements in the Tellico namespace and as a result
  // removes the namespace declaration
  QDomDocument dom = FileHandler::readXMLDocument(u, false);
  if(dom.isNull()) {
    myDebug() << "error loading xslt file: " << xsltFile;
    return QString();
  }

  // the stylesheet prints utf-8 by default, if using locale encoding, need
  // to change the encoding attribute on the xsl:output element
  if(!(options() & Export::ExportUTF8)) {
    XSLTHandler::setLocaleEncoding(dom);
  }

  delete m_handler;
  m_handler = new XSLTHandler(dom, QFile::encodeName(xsltFile));
  if(!m_handler->isValid()) {
    return QString();
  }

  if(options() & ExportImages) {
    m_handler->addStringParam("imageDir", QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation).toLocal8Bit() + "/gcstar/images/");
    writeImages();
  }

  // now grab the XML
  TellicoXMLExporter exporter(coll);
  exporter.setEntries(entries());
  exporter.setFields(fields());
  exporter.setIncludeImages(false); // do not include images in XML
// yes, this should be in utf8, always
  exporter.setOptions(options() | Export::ExportUTF8);
  return m_handler->applyStylesheet(exporter.exportXML().toString());
}

QWidget* GCstarExporter::widget(QWidget* parent_) {
  Q_UNUSED(parent_);
  return nullptr;
}

bool GCstarExporter::writeImages() {
  const QString imgDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/gcstar/images/");
  QDir dir(imgDir);
  if(!dir.exists() && !dir.mkpath(QLatin1String("."))) return false;

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, QString(), false);
  item.setTotalSteps(entries().count());
  ProgressItem::Done done(this);
  const uint stepSize = qMax(1, entries().count()/100);
  const bool showProgress = options() & ExportProgress;

  bool success = true;
  uint j = 0;
  foreach(const Data::EntryPtr& entry, entries()) {
    foreach(Data::FieldPtr field, entry->collection()->imageFields()) {
      if(entry->field(field).isEmpty()) {
        break;
      }

      const auto& img = ImageFactory::imageById(entry->field(field));
      if(img.isNull()) {
        break;
      }

      QUrl target = QUrl::fromLocalFile(imgDir);
      target.setPath(target.path() + img.id());
      success &= FileHandler::writeDataURL(target, img.byteArray(), true /* force */);
    }
    if(showProgress && j%stepSize == 0) {
      item.setProgress(j);
      qApp->processEvents();
    }
    ++j;
  }
  return success;
}
