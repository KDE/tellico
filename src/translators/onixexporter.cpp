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

#include "onixexporter.h"
#include "xslthandler.h"
#include "tellicoxmlexporter.h"
#include "../collection.h"
#include "../core/filehandler.h"
#include "../images/imagefactory.h"
#include "../images/image.h"
#include "../tellico_debug.h"
#include "../gui/cursorsaver.h"

#include <config.h>

#include <kstandarddirs.h>
#include <kapplication.h>
#include <kzip.h>
#include <KConfigGroup>
#include <klocale.h>

#include <QDomDocument>
#include <QFile>
#include <QDateTime>
#include <QBuffer>
#include <QCheckBox>
#include <QGroupBox>
#include <QTextStream>
#include <QVBoxLayout>

using Tellico::Export::ONIXExporter;

ONIXExporter::ONIXExporter() : Tellico::Export::Exporter(),
    m_handler(0),
    m_xsltFile(QLatin1String("tellico2onix.xsl")),
    m_includeImages(true),
    m_widget(0) {
}

ONIXExporter::ONIXExporter(Tellico::Data::CollPtr coll_) : Tellico::Export::Exporter(coll_),
    m_handler(0),
    m_xsltFile(QLatin1String("tellico2onix.xsl")),
    m_includeImages(true),
    m_widget(0) {
}

ONIXExporter::~ONIXExporter() {
  delete m_handler;
  m_handler = 0;
}

QString ONIXExporter::formatString() const {
  return i18n("ONIX Archive");
}

QString ONIXExporter::fileFilter() const {
  return i18n("*.zip|Zip Files (*.zip)") + QLatin1Char('\n') + i18n("*|All Files");
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
  zip.writeFile(QLatin1String("onix.xml"), QString(), QString(), xml, xml.size());

  // use a dict for fast random access to keep track of which images were written to the file
  if(m_includeImages) { // for now, we're ignoring (options() & Export::ExportImages)
    const QString cover = QLatin1String("cover");
    StringSet imageSet;
    foreach(Data::EntryPtr entry, entries()) {
      const Data::Image& img = ImageFactory::imageById(entry->field(cover));
      if(!img.isNull() && !imageSet.has(img.id())
         && (img.format() == "JPEG" || img.format() == "JPG" || img.format() == "GIF")) { /// onix only understands jpeg and gif
        QByteArray ba = img.byteArray();
        zip.writeFile(QLatin1String("images/") + entry->field(cover),
                      QString(), QString(), ba, ba.size());
        imageSet.add(img.id());
      }
    }
  }

  zip.close();
  return FileHandler::writeDataURL(url(), data, options() & Export::ExportForce);
//  return FileHandler::writeTextURL(url(), text(),  options() & Export::ExportUTF8, options() & Export::ExportForce);
}

QString ONIXExporter::text() {
  QString xsltfile = KStandardDirs::locate("appdata", m_xsltFile);
  if(xsltfile.isNull()) {
    myDebug() << "ONIXExporter::text() - no xslt file for " << m_xsltFile << endl;
    return QString();
  }

  Data::CollPtr coll = collection();
  if(!coll) {
    myDebug() << "ONIXExporter::text() - no collection pointer!" << endl;
    return QString();
    ;
  }

  // notes about utf-8 encoding:
  // all params should be passed to XSLTHandler in utf8
  // input string to XSLTHandler should be in utf-8, EVEN IF DOM STRING SAYS OTHERWISE

  KUrl u;
  u.setPath(xsltfile);
  // do NOT do namespace processing, it messes up the XSL declaration since
  // QDom thinks there are no elements in the Tellico namespace and as a result
  // removes the namespace declaration
  QDomDocument dom = FileHandler::readXMLFile(u, false);
  if(dom.isNull()) {
    myDebug() << "ONIXExporter::text() - error loading xslt file: " << xsltfile << endl;
    return QString();
  }

  // the stylesheet prints utf-8 by default, if using locale encoding, need
  // to change the encoding attribute on the xsl:output element
  if(!(options() & Export::ExportUTF8)) {
    XSLTHandler::setLocaleEncoding(dom);
  }

  delete m_handler;
  m_handler = new XSLTHandler(dom, QFile::encodeName(xsltfile));

  QDateTime now = QDateTime::currentDateTime();
  m_handler->addStringParam("sentDate", now.toString(QLatin1String("yyyyMMddhhmm")).toUtf8());

  m_handler->addStringParam("version", VERSION);

  GUI::CursorSaver cs(Qt::WaitCursor);

  // now grab the XML
  TellicoXMLExporter exporter(coll);
  exporter.setEntries(entries());
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
  KConfigGroup group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_includeImages = group.readEntry("Include Images", m_includeImages);
}

void ONIXExporter::saveOptions(KSharedConfigPtr config_) {
  m_includeImages = m_checkIncludeImages->isChecked();

  KConfigGroup group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  group.writeEntry("Include Images", m_includeImages);
}

#include "onixexporter.moc"
