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
#include "translators/tellicoxmlexporter.h"
#include "collection.h"
#include "imagefactory.h"
#include "tellico_kernel.h"

#include <kstandarddirs.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <krun.h>
#include <kmessagebox.h>
#include <khtmlview.h>
#include <dom/dom_element.h>

#include <qfile.h>
#include <qapplication.h> // needed for default palette

using Tellico::EntryView;

EntryView::EntryView(QWidget* parent_, const char* name_) : KHTMLPart(parent_, name_),
    m_entry(0), m_handler(0), m_run(0) {
  setJScriptEnabled(false);
  setJavaEnabled(false);
  setMetaRefreshEnabled(false);
  setPluginsEnabled(false);
  clear(); // needed for initial layout

  connect(browserExtension(), SIGNAL(openURLRequest(const KURL&, const KParts::URLArgs&)),
          this, SLOT(slotOpenURL(const KURL&)));
}

EntryView::~EntryView() {
  if(m_run) {
    m_run->abort();
  }
}

void EntryView::clear() {
  m_entry = 0;

  // just clear the view
  begin();
  end();
  view()->layout(); // I need this because some of the margins and widths may get messed up
}

void EntryView::showEntry(const Data::Entry* entry_) {
//  kdDebug() << "EntryView::showEntry()" << endl;
  if(!entry_) {
    clear();
    return;
  }

#if 0
  kdDebug() << "EntryView::showEntry() - turn me off!" << endl;
  clear();
  setXSLTFile(Entry, viewData->xsltFile);
#endif
  if(!m_handler || !m_handler->isValid()) {
    setXSLTFile(m_xsltFile);
  }

  m_entry = entry_;

  // by setting the xslt file as the URL, any images referenced in the xslt "theme" can be found
  // by simply using a relative path in the xslt file
  begin(KURL::fromPathOrURL(m_xsltFile));

  Data::EntryList list;
  list.append(entry_);
  Export::TellicoXMLExporter exporter(entry_->collection());
  exporter.setEntryList(list);
  // exportXML(bool formatValues, bool encodeUTF8);
  QDomDocument dom = exporter.exportXML(false, true);
//  kdDebug() << dom.toString() << endl;

  QString html = m_handler->applyStylesheet(dom.toString(), true);
  // write out image files
  for(Data::FieldListIterator it(entry_->collection()->imageFields()); it.current(); ++it) {
    const QString& id = entry_->field(it.current()->name());
    if(!id.isEmpty() && !ImageFactory::writeImage(id)) {
        kdWarning() << "EntryView::showEntry() - unable to write temporary image file: "
                    << entry_->field(it.current()->name()) << endl;
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
  view()->layout(); // I need this because some of the margins and widths may get messed up
}

void EntryView::setXSLTFile(const QString& file_) {
  static const QString& templateDir = KGlobal::staticQString("entry-templates/");

  // if starts with slash, then absolute path
  if(file_.at(0) == '/') {
    m_xsltFile = file_;
  } else {
    m_xsltFile = KGlobal::dirs()->findResource("appdata", templateDir + file_);
    if(m_xsltFile.isEmpty()) {
      if(!file_.isEmpty()) {
        kdWarning() << "EntryView::setXSLTFile() - can't locate " << file_ << endl;
      }
      m_xsltFile = KGlobal::dirs()->findResource("appdata", templateDir + QString::fromLatin1("Default.xsl"));
      if(m_xsltFile.isEmpty()) {
        QString str = QString::fromLatin1("<qt>");
        str += i18n("Tellico is unable to locate the default entry stylesheet.");
        str += QChar(' ');
        str += i18n("Please check your installation.");
        str += QString::fromLatin1("</qt>");
        KMessageBox::error(view(), str);
        clear();
        return;
      }
    }
  }

  delete m_handler;

  // must read the file to get proper context
  m_handler = new XSLTHandler(QFile::encodeName(m_xsltFile));
  if(!m_handler->isValid()) {
    kdWarning() << "EntryView::setXSLTFile() - invalid xslt handler" << endl;
    clear();
    return;
  }

  // add system colors to stylesheet
  const QColorGroup& cg = QApplication::palette().active();
  m_handler->addStringParam("font", KGlobalSettings::generalFont().family().latin1());
  m_handler->addStringParam("bgcolor", cg.base().name().latin1());
  m_handler->addStringParam("fgcolor", cg.text().name().latin1());
  m_handler->addStringParam("color1", cg.highlightedText().name().latin1());
  m_handler->addStringParam("color2", cg.highlight().name().latin1());

  // look for a file that gets installed to know the installation directory
  QString appdir = KGlobal::dirs()->findResourceDir("appdata", QString::fromLatin1("pics/tellico.png"));
  m_handler->addStringParam("datadir", QFile::encodeName(appdir));
  m_handler->addStringParam("imgdir", QFile::encodeName(ImageFactory::tempDir()));

  // refresh view
  showEntry(m_entry);
}

void EntryView::refresh() {
  setXSLTFile(m_xsltFile);
  showEntry(m_entry);
}

// do some contortions in case the url is relative
// need to interpret it relative to document URL instead of xslt file
// assume the current node under the mouse is the text node inside
// the anchor node, and look at the href attribute
void EntryView::slotOpenURL(const KURL& url_) {
  DOM::Node node = nodeUnderMouse().parentNode();
  if(node.nodeType() == DOM::Node::ELEMENT_NODE && static_cast<DOM::Element>(node).tagName() == "a") {
    QString href = static_cast<DOM::Element>(node).getAttribute("href").string();
    if(!href.isEmpty() && KURL::isRelativeURL(href)) {
      // interpet url relative to document url
      m_run = new KRun(KURL(Kernel::self()->URL(), href));
      return; // done
    }
  }

  // otherwise, just open the url
  m_run = new KRun(url_);
}

#include "entryview.moc"
