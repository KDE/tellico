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

#include "entryview.h"
#include "entry.h"
#include "filehandler.h"
#include "translators/xslthandler.h"
#include "translators/bookcasexmlexporter.h"
#include "collection.h"
#include "imagefactory.h"

#include <kstandarddirs.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <krun.h>

#include <qfile.h>
#include <qapplication.h> // needed for default palette

using Bookcase::EntryView;

EntryView::EntryView(QWidget* parent_, const char* name_) : KHTMLPart(parent_, name_),
    m_entry(0), m_xsltHandler(0), m_run(0) {
  setJScriptEnabled(false);
  setJavaEnabled(false);
  setMetaRefreshEnabled(false);
  setPluginsEnabled(false);

  connect(browserExtension(), SIGNAL(openURLRequest(const KURL &, const KParts::URLArgs &)),
          this, SLOT(slotOpenURL(const KURL &)));
}

EntryView::~EntryView() {
  delete m_xsltHandler;
  m_xsltHandler = 0;
  if(m_run) {
    m_run->abort();
  }
}

void EntryView::clear() {
  m_entry = 0;
  // just clear the view
  begin();
  end();
}

void EntryView::showEntry(const Data::Entry* const entry_) {
//  kdDebug() << "EntryView::showEntry()" << endl;
  if(!entry_ || !m_xsltHandler) {
    clear();
    return;
  }

#if 0
  clear();
  setXSLTFile(m_xsltFile);
#endif
  if(!m_xsltHandler->isValid()) {
    setXSLTFile(m_xsltFile);
  }

  // by setting the xslt file as the URL, any images referenced in the xslt "theme" can be found
  // by simply using a relative path in the xslt file
  begin(m_xsltFile);

  Data::EntryList list;
  list.append(entry_);
  Export::BookcaseXMLExporter exporter(entry_->collection(), list);
  // exportXML(bool formatValues, bool encodeUTF8);
  QDomDocument dom = exporter.exportXML(false, true);
//  kdDebug() << dom.toString() << endl;

  if(!m_tempDirSet) {
    // look for a file that gets installed to know the installation directory
    QString appdir = KGlobal::dirs()->findResourceDir("appdata", QString::fromLatin1("pics/bookcase.png"));
    appdir += QString::fromLatin1("pics/");
    m_xsltHandler->addStringParam("datadir", QFile::encodeName(appdir));
    m_xsltHandler->addStringParam("tmpdir", QFile::encodeName(ImageFactory::tempDir()));
    m_tempDirSet = true;
  }

  QString html = m_xsltHandler->applyStylesheet(dom.toString(), true);
  // write out image files
  for(Data::FieldListIterator it(entry_->collection()->imageFields()); it.current(); ++it) {
    const QString& id = entry_->field(it.current()->name());
    if(!id.isEmpty()) {
      bool success = ImageFactory::writeImage(id);
      if(!success) {
        kdWarning() << "EntryView::showEntry() - unable to write temporary image file: "
                    << entry_->field(it.current()->name()) << endl;
      }
    }
  }

#if 0
  QFile f(QString::fromLatin1("/tmp/test.html"));
  if(f.open(IO_WriteOnly)) {
    QTextStream t(&f);
    t << html;
  }
  f.close();
#endif

//  kdDebug() << html << endl;
  write(html);
  end();

  m_entry = entry_;
}

void EntryView::setXSLTFile(const QString& file_) {
  if(file_.at(0) == '/') {
    m_xsltFile = file_;
  } else {
    m_xsltFile = KGlobal::dirs()->findResource("appdata", QString::fromLatin1("entry-templates/") + file_);
    if(m_xsltFile.isNull()) {
      // TODO: error message
      kdWarning() << "EntryView::setXSLTFile() - can't locate " << file_ << endl;
      m_xsltFile = KGlobal::dirs()->findResource("appdata", QString::fromLatin1("entry-templates/Default.xsl"));
    }
  }


  KURL url;
  url.setPath(m_xsltFile);
  QDomDocument dom = FileHandler::readXMLFile(url);
  if(dom.isNull()) {
    kdWarning() << "EntryView::setXSLTFile() - null DOM" << endl;
    return;
  }
  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(dom.toString());

  // add system colors to stylesheet
  const QColorGroup& cg = QApplication::palette().active();
  m_xsltHandler->addStringParam("font", KGlobalSettings::generalFont().family().latin1());
  m_xsltHandler->addStringParam("bgcolor", cg.base().name().latin1());
  m_xsltHandler->addStringParam("fgcolor", cg.text().name().latin1());
  m_xsltHandler->addStringParam("color1", cg.highlightedText().name().latin1());
  m_xsltHandler->addStringParam("color2", cg.highlight().name().latin1());

  m_tempDirSet = false;

  showEntry(m_entry);
}

void EntryView::refresh() {
  setXSLTFile(m_xsltFile);
  showEntry(m_entry);
}

void EntryView::slotOpenURL(const KURL& url_) {
//  kdDebug() << "EntryView::slotOpenURL() - " << url_.path() << endl;
  if(url_.isValid()) {
    m_run = new KRun(url_);
  }
}



