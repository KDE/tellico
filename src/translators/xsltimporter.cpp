/***************************************************************************
                              xsltimporter.cpp
                             -------------------
    begin                : Wed Sep 24 2003
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

#include "xsltimporter.h"
#include "xslthandler.h"
#include "bookcasexmlimporter.h"
#include "../bcfilehandler.h"

#include <klocale.h>
#include <kurlrequester.h>
#include <kdebug.h>

#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>

XSLTImporter::XSLTImporter(const KURL& url_) : TextImporter(url_), m_coll(0), m_widget(0) {
}

BCCollection* XSLTImporter::collection() {
  if(!m_widget) {
    return 0;
  }

  if(m_coll) {
    return m_coll;
  }

  KURL u = m_URLRequester->url();
  if(u.isEmpty() || !u.isValid()) {
    setStatusMessage(i18n("A valid XSLT file is needed to import the file."));
    return 0;
  }

  QDomDocument doc = BCFileHandler::readXMLFile(u);
  XSLTHandler handler(doc.toString());
//  kdDebug() << text() << endl;
  // TODO: is there anyway to know if the text is in utf-8 or not?
  QString str = handler.applyStylesheet(text(), false);
//  kdDebug() << str << endl;
  
  BookcaseXMLImporter imp(str);
  m_coll = imp.collection();
  setStatusMessage(imp.statusMessage());
  return m_coll;
}

QWidget* XSLTImporter::widget(QWidget* parent_, const char* name_) {
  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* box = new QGroupBox(1, Qt::Vertical, i18n("XSLT Options"), m_widget);
  (void) new QLabel(i18n("XSLT File:"), box);
  m_URLRequester = new KURLRequester(box);
  l->addWidget(box);

  l->addStretch(1);
  return m_widget;
}
