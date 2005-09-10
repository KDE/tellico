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

#include "xsltexporter.h"
#include "xslthandler.h"
#include "tellicoxmlexporter.h"
#include "../filehandler.h"

#include <klocale.h>
#include <kurlrequester.h>

#include <qlabel.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qdom.h>
#include <qwhatsthis.h>

using Tellico::Export::XSLTExporter;

XSLTExporter::XSLTExporter() : Export::Exporter(),
    m_widget(0),
    m_URLRequester(0) {
}

QString XSLTExporter::formatString() const {
  return i18n("XSLT");
}

QString XSLTExporter::fileFilter() const {
  return i18n("*|All Files");
}


bool XSLTExporter::exec() {
  KURL u = m_URLRequester->url();
  if(u.isEmpty() || !u.isValid()) {
    return QString::null;
  }
  //  XSLTHandler handler(FileHandler::readXMLFile(url));
  XSLTHandler handler(u);

  TellicoXMLExporter exporter;
  exporter.setEntries(entries());
  exporter.setOptions(options());
  QDomDocument dom = exporter.exportXML();
  return FileHandler::writeTextURL(url(), handler.applyStylesheet(dom.toString()),
                                   options() & ExportUTF8, options() & Export::ExportForce);
}

QWidget* XSLTExporter::widget(QWidget* parent_, const char* name_/*=0*/) {
  if(m_widget && m_widget->parent() == parent_) {
    return m_widget;
  }

  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* group = new QGroupBox(1, Qt::Horizontal, i18n("XSLT Options"), m_widget);
  l->addWidget(group);

  QHBox* box = new QHBox(group);
  box->setSpacing(4);
  (void) new QLabel(i18n("XSLT file:"), box);
  m_URLRequester = new KURLRequester(box);
  QWhatsThis::add(m_URLRequester, i18n("Choose the XSLT file used to transform the Tellico XML data."));

  l->addStretch(1);
  return m_widget;
}
