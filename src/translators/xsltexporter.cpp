/***************************************************************************
                              xsltexporter.cpp
                             -------------------
    begin                : Sat Aug 2 2003
    copyright            : (C) 2003 by Robby Stephenson
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
#include "bookcasexmlexporter.h"
#include "../bcfilehandler.h"

#include <klocale.h>
#include <kurlrequester.h>

#include <qlabel.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qdom.h>
#include <qwhatsthis.h>

XSLTExporter::XSLTExporter(const BCCollection* coll_, BCUnitList list_) : Exporter(coll_, list_),
  m_widget(0),
  m_URLRequester(0) {
}

QString XSLTExporter::formatString() const {
  return i18n("XSLT");
}

QString XSLTExporter::fileFilter() const {
  return i18n("*|All files");
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
  (void) new QLabel(i18n("XSLT File:"), box);
  m_URLRequester = new KURLRequester(box);
  QWhatsThis::add(m_URLRequester, i18n("Choose the XSLT file used to transform the Bookcase XML data."));

  l->addStretch(1);
  return m_widget;
}

QString XSLTExporter::text(bool formatAttributes_, bool encodeUTF8_) {
  KURL url = m_URLRequester->url();
  if(url.isValid()) {
    XSLTHandler handler(BCFileHandler::readFile(url));

    BookcaseXMLExporter exporter(collection(), unitList());
    QDomDocument dom = exporter.exportXML(formatAttributes_, encodeUTF8_);
    return handler.applyStylesheet(dom.toString(), encodeUTF8_);
  }
  return QString::null;
}
