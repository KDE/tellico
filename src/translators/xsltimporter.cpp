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

#include "xsltimporter.h"
#include "xslthandler.h"
#include "bookcaseimporter.h"
#include "../filehandler.h"

#include <klocale.h>
#include <kurlrequester.h>
#include <kdebug.h>

#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>

using Bookcase::Import::XSLTImporter;

XSLTImporter::XSLTImporter(const KURL& url_) : Bookcase::Import::TextImporter(url_),
    m_coll(0),
    m_widget(0),
    m_URLRequester(0) {
}

Bookcase::Data::Collection* XSLTImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  if(m_xsltURL.isEmpty()) {
    // if there's also no widget, then something went wrong
    if(!m_widget) {
      setStatusMessage(i18n("A valid XSLT file is needed to import the file."));
      return 0;
    }
    m_xsltURL = m_URLRequester->url();
  }
  if(m_xsltURL.isEmpty() || !m_xsltURL.isValid()) {
    setStatusMessage(i18n("A valid XSLT file is needed to import the file."));
    return 0;
  }

  XSLTHandler handler(m_xsltURL);
  if(!handler.isValid()) {
    setStatusMessage(i18n("Tellico encountered an error in XSLT processing."));
    return 0;
  }
//  kdDebug() << text() << endl;
  // FIXME: is there anyway to know if the text is in utf-8 or not? Assume it is.
  QString str = handler.applyStylesheet(text(), true);
//  kdDebug() << str << endl;

  Import::BookcaseImporter imp(str);
  connect(&imp, SIGNAL(signalFractionDone(float)), SIGNAL(signalFractionDone(float)));
  m_coll = imp.collection();
  setStatusMessage(imp.statusMessage());
  return m_coll;
}

QWidget* XSLTImporter::widget(QWidget* parent_, const char* name_) {
  // if the url has already been set, then there's no widget
  if(!m_xsltURL.isEmpty()) {
    return 0;
  }

  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* box = new QGroupBox(1, Qt::Vertical, i18n("XSLT Options"), m_widget);
  l->addWidget(box);

  (void) new QLabel(i18n("XSLT File:"), box);
  m_URLRequester = new KURLRequester(box);

  QString filter = i18n("*.xsl|XSL files (*.xsl)") + QChar('\n');
  filter += i18n("*|All files");
  m_URLRequester->setFilter(filter);

  l->addStretch(1);
  return m_widget;
}

#include "xsltimporter.moc"
