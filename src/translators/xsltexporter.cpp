/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
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
#include "../filehandler.h"

#include <klocale.h>
#include <kurlrequester.h>

#include <qlabel.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qdom.h>
#include <qwhatsthis.h>

using Bookcase::Export::XSLTExporter;

XSLTExporter::XSLTExporter(const Data::Collection* coll_) : Export::TextExporter(coll_),
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
  box->setSpacing(4);
  (void) new QLabel(i18n("XSLT File:"), box);
  m_URLRequester = new KURLRequester(box);
  QWhatsThis::add(m_URLRequester, i18n("Choose the XSLT file used to transform the Bookcase XML data."));

  l->addStretch(1);
  return m_widget;
}

QString XSLTExporter::text(bool formatFields_, bool encodeUTF8_) {
  KURL url = m_URLRequester->url();
  if(url.isEmpty() || !url.isValid()) {
    return QString::null;
  }
  //  XSLTHandler handler(FileHandler::readXMLFile(url));
  XSLTHandler handler(url);

  BookcaseXMLExporter exporter(collection());
  exporter.setEntryList(entryList());
  QDomDocument dom = exporter.exportXML(formatFields_, encodeUTF8_);
  return handler.applyStylesheet(dom.toString(), encodeUTF8_);
}
