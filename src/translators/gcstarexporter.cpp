/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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
#include "../tellico_debug.h"

#include <kstandarddirs.h>
#include <klocale.h>

#include <QDomDocument>
#include <QFile>
#include <QTextStream>

using Tellico::Export::GCstarExporter;

GCstarExporter::GCstarExporter(Tellico::Data::CollPtr coll_) : Tellico::Export::Exporter(coll_),
    m_handler(0),
    m_xsltFile(QLatin1String("tellico2gcstar.xsl")) {
}

GCstarExporter::~GCstarExporter() {
  delete m_handler;
  m_handler = 0;
}

QString GCstarExporter::formatString() const {
  // TODO i18n this?
  return QLatin1String("GCstar");
}

QString GCstarExporter::fileFilter() const {
  return i18n("*.gcs|GCstar Data Files (*.gcs)") + QLatin1Char('\n') + i18n("*|All Files");
}

bool GCstarExporter::exec() {
  const QString text = this->text();
  return !text.isEmpty() && FileHandler::writeTextURL(url(), text, options() & ExportUTF8, options() & Export::ExportForce);
}

QString GCstarExporter::text() {
  QString xsltfile = KStandardDirs::locate("appdata", m_xsltFile);
  if(xsltfile.isNull()) {
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

  KUrl u;
  u.setPath(xsltfile);
  // do NOT do namespace processing, it messes up the XSL declaration since
  // QDom thinks there are no elements in the Tellico namespace and as a result
  // removes the namespace declaration
  QDomDocument dom = FileHandler::readXMLFile(u, false);
  if(dom.isNull()) {
    myDebug() << "error loading xslt file: " << xsltfile;
    return QString();
  }

  // the stylesheet prints utf-8 by default, if using locale encoding, need
  // to change the encoding attribute on the xsl:output element
  if(!(options() & Export::ExportUTF8)) {
    XSLTHandler::setLocaleEncoding(dom);
  }

  delete m_handler;
  m_handler = new XSLTHandler(dom, QFile::encodeName(xsltfile));
  if(!m_handler || !m_handler->isValid()) {
    myDebug() << "bad handler";
    return QString();
  }

  // now grab the XML
  TellicoXMLExporter exporter(coll);
  exporter.setEntries(entries());
  exporter.setIncludeImages(false); // do not include images in XML
// yes, this should be in utf8, always
  exporter.setOptions(options() | Export::ExportUTF8);
  return m_handler->applyStylesheet(exporter.exportXML().toString());
}

QWidget* GCstarExporter::widget(QWidget* parent_) {
  Q_UNUSED(parent_);
  return 0;
}

#include "gcstarexporter.moc"
