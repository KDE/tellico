/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#include <config.h>

#include "onixexporter.h"
#include "xslthandler.h"
#include "tellicoxmlexporter.h"
#include "../collection.h"
#include "../core/filehandler.h"
#include "../images/imagefactory.h"
#include "../images/image.h"
#include "../utils/cursorsaver.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

#include <KZip>
#include <KConfigGroup>
#include <KLocalizedString>

#include <QDomDocument>
#include <QFile>
#include <QDateTime>
#include <QBuffer>
#include <QCheckBox>
#include <QGroupBox>
#include <QTextStream>
#include <QVBoxLayout>

using Tellico::Export::ONIXExporter;

ONIXExporter::ONIXExporter(Tellico::Data::CollPtr coll_, const QUrl& baseUrl_)
  : Tellico::Export::Exporter(coll_, baseUrl_),
    m_handler(nullptr),
    m_xsltFile(QStringLiteral("tellico2onix.xsl")),
    m_includeImages(true),
    m_widget(nullptr),
    m_checkIncludeImages(nullptr) {
}

ONIXExporter::~ONIXExporter() {
  delete m_handler;
  m_handler = nullptr;
}

QString ONIXExporter::formatString() const {
  return QStringLiteral("ONIX");
}

QString ONIXExporter::fileFilter() const {
  return i18n("Zip Files") + QLatin1String(" (*.zip)") + QLatin1String(";;") + i18n("All Files") + QLatin1String(" (*)");
}

bool ONIXExporter::exec() {
  Data::CollPtr coll = collection();
  if(!coll) {
    return false;
  }

  QByteArray xml = text().toUtf8(); // encoded in utf-8

  QByteArray data;
  QBuffer buf(&data);

  KZip zip(&buf);
  zip.open(QIODevice::WriteOnly);
  zip.writeFile(QStringLiteral("onix.xml"), xml);

  // use a dict for fast random access to keep track of which images were written to the file
  if(m_includeImages) { // for now, we're ignoring (options() & Export::ExportImages)
    const QString cover = QStringLiteral("cover");
    StringSet imageSet;
    foreach(Data::EntryPtr entry, entries()) {
      const QString entryCover = entry->field(cover);
      const auto& img = ImageFactory::imageById(entryCover);
      if(!img.isNull() && !imageSet.contains(img.id())
         && (img.format() == "JPEG" || img.format() == "JPG" || img.format() == "GIF")) { /// onix only understands jpeg and gif
        QByteArray ba = img.byteArray();
        zip.writeFile(QLatin1String("images/") + entry->field(cover), ba);
        imageSet.add(img.id());
      }
    }
  }

  zip.close();
  return FileHandler::writeDataURL(url(), data, options() & Export::ExportForce);
}

QString ONIXExporter::text() {
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

  QDateTime now = QDateTime::currentDateTime();
  m_handler->addStringParam("sentDate", now.toString(QStringLiteral("yyyyMMddhhmm")).toUtf8());

  m_handler->addStringParam("version", TELLICO_VERSION);

  GUI::CursorSaver cs(Qt::WaitCursor);

  // now grab the XML
  TellicoXMLExporter exporter(coll, baseUrl());
  exporter.setEntries(entries());
  exporter.setFields(fields());
  exporter.setIncludeImages(false); // do not include images in XML
// yes, this should be in utf8, always
  exporter.setOptions(options() | Export::ExportUTF8);
  QDomDocument output = exporter.exportXML();
#if 0
  QFile f(QLatin1String("/tmp/test.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << output.toString();
  }
  f.close();
#endif
  return m_handler->applyStylesheet(output.toString());
}

QWidget* ONIXExporter::widget(QWidget* parent_) {
  if(m_widget) {
    return m_widget;
  }

  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* gbox = new QGroupBox(i18n("ONIX Archive Options"), m_widget);
  QVBoxLayout* vlay = new QVBoxLayout(gbox);

  m_checkIncludeImages = new QCheckBox(i18n("Include images in archive"), gbox);
  m_checkIncludeImages->setChecked(m_includeImages);
  m_checkIncludeImages->setWhatsThis(i18n("If checked, the images in the document will be included "
                                          "in the zipped ONIX archive."));

  vlay->addWidget(m_checkIncludeImages);

  l->addWidget(gbox);
  l->addStretch(1);
  return m_widget;
}

void ONIXExporter::readOptions(KSharedConfigPtr config_) {
  KConfigGroup group(config_, QStringLiteral("ExportOptions - %1").arg(formatString()));
  m_includeImages = group.readEntry("Include Images", m_includeImages);
}

void ONIXExporter::saveOptions(KSharedConfigPtr config_) {
  m_includeImages = m_checkIncludeImages->isChecked();

  KConfigGroup group(config_, QStringLiteral("ExportOptions - %1").arg(formatString()));
  group.writeEntry("Include Images", m_includeImages);
}
