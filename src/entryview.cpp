/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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
#include "field.h"
#include "filehandler.h"
#include "translators/xslthandler.h"
#include "translators/tellicoxmlexporter.h"
#include "collection.h"
#include "imagefactory.h"
#include "tellico_kernel.h"
#include "tellico_utils.h"
#include "core/tellico_config.h"
#include "newstuff/manager.h"
#include "document.h"
#include "latin1literal.h"
#include "../core/drophandler.h"

#include <kstandarddirs.h>
#include <krun.h>
#include <kmessagebox.h>
#include <khtmlview.h>
#include <dom/dom_element.h>
#include <kapplication.h>
#include <ktempfile.h>
#include <klocale.h>

#include <qfile.h>

using Tellico::EntryView;

EntryView::EntryView(QWidget* parent_, const char* name_) : KHTMLPart(parent_, name_),
    m_entry(0), m_handler(0), m_run(0), m_tempFile(0), m_useGradientImages(true), m_checkCommonFile(true) {
  setJScriptEnabled(false);
  setJavaEnabled(false);
  setMetaRefreshEnabled(false);
  setPluginsEnabled(false);
  clear(); // needed for initial layout

  view()->setAcceptDrops(true);
  DropHandler* drophandler = new DropHandler(this);
  view()->installEventFilter(drophandler);

  connect(browserExtension(), SIGNAL(openURLRequest(const KURL&, const KParts::URLArgs&)),
          SLOT(slotOpenURL(const KURL&)));
  connect(kapp, SIGNAL(kdisplayPaletteChanged()), SLOT(slotResetColors()));
}

EntryView::~EntryView() {
  if(m_run) {
    m_run->abort();
  }
  delete m_handler;
  m_handler = 0;
  delete m_tempFile;
  m_tempFile = 0;
}

void EntryView::clear() {
  m_entry = 0;

  // just clear the view
  begin();
  if(!m_textToShow.isEmpty()) {
    write(m_textToShow);
  }
  end();
  view()->layout(); // I need this because some of the margins and widths may get messed up
}

void EntryView::showEntry(Data::EntryPtr entry_) {
  if(!entry_) {
    clear();
    return;
  }

  m_textToShow = QString();
#if 0
  kdWarning() << "EntryView::showEntry() - turn me off!" << endl;
  m_entry = 0;
  setXSLTFile(m_xsltFile);
#endif
  if(!m_handler || !m_handler->isValid()) {
    setXSLTFile(m_xsltFile);
  }

  m_entry = entry_;

  // by setting the xslt file as the URL, any images referenced in the xslt "theme" can be found
  // by simply using a relative path in the xslt file
  KURL u;
  u.setPath(m_xsltFile);
  begin(u);

  Export::TellicoXMLExporter exporter(entry_->collection());
  exporter.setEntries(entry_);
  long opt = exporter.options();
  // verify images for the view
  opt |= Export::ExportVerifyImages;
  // on second thought, don't auto-format everything, just clean it
//  if(Data::Field::autoFormat()) {
//    opt = Export::ExportFormatted;
//  }
  if(entry_->collection()->type() == Data::Collection::Bibtex) {
    opt |= Export::ExportClean;
  }
  exporter.setOptions(opt);
  QDomDocument dom = exporter.exportXML();

//  myDebug() << dom.toString() << endl;
#if 0
  kdWarning() << "EntryView::showEntry() - turn me off!" << endl;
  QFile f1(QString::fromLatin1("/tmp/test.xml"));
  if(f1.open(IO_WriteOnly)) {
    QTextStream t(&f1);
    t << dom.toString();
  }
  f1.close();
#endif

  QString html = m_handler->applyStylesheet(dom.toString());
  // write out image files
  Data::FieldVec fields = entry_->collection()->imageFields();
  for(Data::FieldVec::Iterator field = fields.begin(); field != fields.end(); ++field) {
    QString id = entry_->field(field);
    if(id.isEmpty()) {
      continue;
    }
    if(Data::Document::self()->allImagesOnDisk()) {
      ImageFactory::writeCachedImage(id, ImageFactory::DataDir);
    } else {
      ImageFactory::writeCachedImage(id, ImageFactory::TempDir);
    }
  }

#if 0
  kdWarning() << "EntryView::showEntry() - turn me off!" << endl;
  QFile f2(QString::fromLatin1("/tmp/test.html"));
  if(f2.open(IO_WriteOnly)) {
    QTextStream t(&f2);
    t << html;
  }
  f2.close();
#endif

//  myDebug() << html << endl;
  write(html);
  end();
  // not need anymore?
  view()->layout(); // I need this because some of the margins and widths may get messed up
}

void EntryView::showText(const QString& text_) {
  m_textToShow = text_;
  begin();
  write(text_);
  end();
}

void EntryView::setXSLTFile(const QString& file_) {
  QString oldFile = m_xsltFile;
  // if starts with slash, then absolute path
  if(file_.at(0) == '/') {
    m_xsltFile = file_;
  } else {
    const QString templateDir = QString::fromLatin1("entry-templates/");
    m_xsltFile = locate("appdata", templateDir + file_);
    if(m_xsltFile.isEmpty()) {
      if(!file_.isEmpty()) {
        kdWarning() << "EntryView::setXSLTFile() - can't locate " << file_ << endl;
      }
      m_xsltFile = locate("appdata", templateDir + QString::fromLatin1("Fancy.xsl"));
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

  const int type = m_entry ? m_entry->collection()->type() : Kernel::self()->collectionType();

  // we need to know if the colors changed from last time, in case
  // we need to do that ugly hack to reload the cache
  bool reloadImages = m_useGradientImages;
  // if m_useGradientImages is false, then we don't even need to check
  // if there's no handler, there there's _no way_ to check
  if(m_handler && reloadImages) {
    // the only two colors that matter for the gradients are the base color
    // and highlight base color
    const QCString& oldBase = m_handler->param("bgcolor");
    const QCString& oldHigh = m_handler->param("color2");
    // remember the string params have apostrophes on either side, so we can start search at pos == 1
    reloadImages = oldBase.find(Config::templateBaseColor(type).name().latin1(), 1) == -1
                || oldHigh.find(Config::templateHighlightedBaseColor(type).name().latin1(), 1) == -1;
  }

  if(!m_handler || m_xsltFile != oldFile) {
    delete m_handler;
    // must read the file name to get proper context
    m_handler = new XSLTHandler(QFile::encodeName(m_xsltFile));
    if(m_checkCommonFile && !m_handler->isValid()) {
      NewStuff::Manager::checkCommonFile();
      m_checkCommonFile = false;
      delete m_handler;
      m_handler = new XSLTHandler(QFile::encodeName(m_xsltFile));
    }
    if(!m_handler->isValid()) {
      kdWarning() << "EntryView::setXSLTFile() - invalid xslt handler" << endl;
      clear();
      delete m_handler;
      m_handler = 0;
      return;
    }
  }

  m_handler->addStringParam("font",     Config::templateFont(type).family().latin1());
  m_handler->addStringParam("fontsize", QCString().setNum(Config::templateFont(type).pointSize()));
  m_handler->addStringParam("bgcolor",  Config::templateBaseColor(type).name().latin1());
  m_handler->addStringParam("fgcolor",  Config::templateTextColor(type).name().latin1());
  m_handler->addStringParam("color1",   Config::templateHighlightedTextColor(type).name().latin1());
  m_handler->addStringParam("color2",   Config::templateHighlightedBaseColor(type).name().latin1());

  if(Data::Document::self()->allImagesOnDisk()) {
    m_handler->addStringParam("imgdir", QFile::encodeName(ImageFactory::dataDir()));
  } else {
    m_handler->addStringParam("imgdir", QFile::encodeName(ImageFactory::tempDir()));
  }

  // look for a file that gets installed to know the installation directory
  QString appdir = KGlobal::dirs()->findResourceDir("appdata", QString::fromLatin1("pics/tellico.png"));
  m_handler->addStringParam("datadir", QFile::encodeName(appdir));

  // if we don't have to reload the images, then just show the entry and we're done
  if(!reloadImages) {
    showEntry(m_entry);
    return;
  }

  // now, have to recreate images and refresh khtml cache
  resetColors();
}

void EntryView::slotRefresh() {
  setXSLTFile(m_xsltFile);
  showEntry(m_entry);
  view()->repaint();
}

// do some contortions in case the url is relative
// need to interpret it relative to document URL instead of xslt file
// the current node under the mouse vould be the text node inside
// the anchor node, so iterate up the parents
void EntryView::slotOpenURL(const KURL& url_) {
  if(url_.protocol() == Latin1Literal("tc")) {
    // handle this internally
    emit signalAction(url_);
    return;
  }

  KURL u = url_;
  for(DOM::Node node = nodeUnderMouse(); !node.isNull(); node = node.parentNode()) {
    if(node.nodeType() == DOM::Node::ELEMENT_NODE && static_cast<DOM::Element>(node).tagName() == "a") {
      QString href = static_cast<DOM::Element>(node).getAttribute("href").string();
      if(!href.isEmpty() && KURL::isRelativeURL(href)) {
        // interpet url relative to document url
        u = KURL(Kernel::self()->URL(), href);
      }
      break;
    }
  }
  // open the url, m_run gets auto-deleted
  m_run = new KRun(u);
}

void EntryView::slotReloadEntry() {
  // this slot should only be connected in setXSLTFile()
  // must disconnect the signal first, otherwise, get an infinite loop
  disconnect(SIGNAL(completed()));
  closeURL(); // this is needed to stop everything, for some reason
  view()->setUpdatesEnabled(true);

  if(m_entry) {
    showEntry(m_entry);
  } else {
    // setXSLTFile() writes some html to clear the image cache
    // but we don't want to see that, so just clear everything
    clear();
  }
  delete m_tempFile;
  m_tempFile = 0;
}

void EntryView::setXSLTOptions(const StyleOptions& opt_) {
  m_handler->addStringParam("font",     opt_.fontFamily.latin1());
  m_handler->addStringParam("fontsize", QCString().setNum(opt_.fontSize));
  m_handler->addStringParam("bgcolor",  opt_.baseColor.name().latin1());
  m_handler->addStringParam("fgcolor",  opt_.textColor.name().latin1());
  m_handler->addStringParam("color1",   opt_.highlightedTextColor.name().latin1());
  m_handler->addStringParam("color2",   opt_.highlightedBaseColor.name().latin1());
  m_handler->addStringParam("imgdir",   QFile::encodeName(opt_.imgDir));
}


void EntryView::slotResetColors() {
  // this will delete and reread the default colors, assuming they changed
  // better to do this elsewhere, but do it here for now
  Config::deleteAndReset();
  delete m_handler; m_handler = 0;
  setXSLTFile(m_xsltFile);
}

void EntryView::resetColors() {
  ImageFactory::createStyleImages(); // recreate gradients

  QString dir = m_handler ? m_handler->param("imgdir") : QString();
  if(dir.isEmpty()) {
    dir = Data::Document::self()->allImagesOnDisk() ? ImageFactory::dataDir() : ImageFactory::tempDir();
  } else {
    // it's a string param, so it has quotes on both sides
    dir = dir.mid(1);
    dir.truncate(dir.length()-1);
  }

  // this is a rather bad hack to get around the fact that the image cache is not reloaded when
  // the gradient files are changed on disk. Setting the URLArgs for write() calls doesn't seem to
  // work. So force a reload with a temp file, then catch the completed signal and repaint
  QString s = QString::fromLatin1("<html><body><img src=\"%1\"><img src=\"%2\"></body></html>")
                             .arg(dir + QString::fromLatin1("gradient_bg.png"))
                             .arg(dir + QString::fromLatin1("gradient_header.png"));

  delete m_tempFile;
  m_tempFile = new KTempFile;
  m_tempFile->setAutoDelete(true);
  *m_tempFile->textStream() << s;
  m_tempFile->file()->close(); // have to close it

  KParts::URLArgs args = browserExtension()->urlArgs();
  args.reload = true; // tell the cache to reload images
  browserExtension()->setURLArgs(args);

  // don't flicker
  view()->setUpdatesEnabled(false);
  openURL(m_tempFile->name());
  connect(this, SIGNAL(completed()), SLOT(slotReloadEntry()));
}

#include "entryview.moc"
