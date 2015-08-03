/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include "entryview.h"
#include "entry.h"
#include "field.h"
#include "translators/xslthandler.h"
#include "translators/tellicoxmlexporter.h"
#include "collection.h"
#include "images/imagefactory.h"
#include "images/imageinfo.h"
#include "tellico_kernel.h"
#include "utils/tellico_utils.h"
#include "core/filehandler.h"
#include "core/tellico_config.h"
#include "gui/drophandler.h"
#include "document.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

#include <krun.h>
#include <kmessagebox.h>
#include <khtmlview.h>
#include <dom/dom_element.h>
#include <KLocalizedString>
#include <KGlobalSettings>

#include <QFile>
#include <QTextStream>
#include <QClipboard>
#include <QDomDocument>
#include <QTemporaryFile>
#include <QApplication>

using Tellico::EntryView;
using Tellico::EntryViewWidget;

EntryViewWidget::EntryViewWidget(KHTMLPart* part, QWidget* parent)
    : KHTMLView(part, parent) {}

// for the life of me, I could not figure out how to call the actual
// KHTMLPartBrowserExtension::copy() slot, so this will have to do
void EntryViewWidget::copy() {
  QApplication::clipboard()->setText(part()->selectedText(), QClipboard::Clipboard);
}

EntryView::EntryView(QWidget* parent_) : KHTMLPart(new EntryViewWidget(this, parent_), parent_),
    m_handler(0), m_run(0), m_tempFile(0), m_useGradientImages(true), m_checkCommonFile(true) {
  setJScriptEnabled(false);
  setJavaEnabled(false);
  setMetaRefreshEnabled(false);
  setPluginsEnabled(false);
  clear(); // needed for initial layout

  view()->setAcceptDrops(true);
  DropHandler* drophandler = new DropHandler(this);
  view()->installEventFilter(drophandler);

  connect(browserExtension(), SIGNAL(openUrlRequestDelayed(const QUrl&, const KParts::OpenUrlArguments&, const KParts::BrowserArguments&)),
          SLOT(slotOpenURL(const QUrl&)));
  connect(KGlobalSettings::self(), SIGNAL(kdisplayPaletteChanged()), SLOT(slotResetColors()));

  view()->setWhatsThis(i18n("<qt>The <i>Entry View</i> shows a formatted view of the entry's contents.</qt>"));
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

void EntryView::showEntries(Tellico::Data::EntryList entries_) {
  Q_ASSERT(!entries_.isEmpty());
  showEntry(entries_.first());
}

void EntryView::showEntry(Tellico::Data::EntryPtr entry_) {
  if(!entry_) {
    clear();
    return;
  }

  m_textToShow.clear();
#if 0
  myWarning() << "turn me off!";
  m_entry = 0;
  setXSLTFile(m_xsltFile);
#endif
  if(!m_handler || !m_handler->isValid()) {
    setXSLTFile(m_xsltFile);
  }
  if(!m_handler || !m_handler->isValid()) {
    myWarning() << "no xslt handler";
    return;
  }

  m_entry = entry_;

  // by setting the xslt file as the URL, any images referenced in the xslt "theme" can be found
  // by simply using a relative path in the xslt file
  QUrl u = QUrl::fromLocalFile(m_xsltFile);
  begin(u);

  Export::TellicoXMLExporter exporter(entry_->collection());
  exporter.setEntries(Data::EntryList() << entry_);
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

//  myDebug() << dom.toString();
#if 0
  myWarning() << "turn me off!";
  QFile f1(QLatin1String("/tmp/test.xml"));
  if(f1.open(QIODevice::WriteOnly)) {
    QTextStream t(&f1);
    t << dom.toString();
  }
  f1.close();
#endif

  QString html = m_handler->applyStylesheet(dom.toString());
  // write out image files
  Data::FieldList fields = entry_->collection()->imageFields();
  foreach(Data::FieldPtr field, fields) {
    QString id = entry_->field(field);
    if(id.isEmpty()) {
      continue;
    }
    // only write out image if it's not linked only
    if(!ImageFactory::imageInfo(id).linkOnly) {
      if(Data::Document::self()->allImagesOnDisk()) {
        ImageFactory::writeCachedImage(id, ImageFactory::cacheDir());
      } else {
        ImageFactory::writeCachedImage(id, ImageFactory::TempDir);
      }
    }
  }

#if 0
  myWarning() << "turn me off!";
  QFile f2(QLatin1String("/tmp/test.html"));
  if(f2.open(QIODevice::WriteOnly)) {
    QTextStream t(&f2);
    t << html;
  }
  f2.close();
#endif

//  myDebug() << html;
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
  if(file_.isEmpty()) {
    myWarning() << "empty xslt file";
    return;
  }
  QString oldFile = m_xsltFile;
  // if starts with slash, then absolute path
  if(file_.at(0) == QLatin1Char('/')) {
    m_xsltFile = file_;
  } else {
    const QString templateDir = QLatin1String("entry-templates/");
    m_xsltFile = DataFileRegistry::self()->locate(templateDir + file_);
    if(m_xsltFile.isEmpty()) {
      if(!file_.isEmpty()) {
        myWarning() << "can't locate" << file_;
      }
      m_xsltFile = DataFileRegistry::self()->locate(templateDir + QLatin1String("Fancy.xsl"));
      if(m_xsltFile.isEmpty()) {
        QString str = QLatin1String("<qt>");
        str += i18n("Tellico is unable to locate the default entry stylesheet.");
        str += QLatin1Char(' ');
        str += i18n("Please check your installation.");
        str += QLatin1String("</qt>");
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
    QByteArray oldBase = m_handler->param("bgcolor");
    QByteArray oldHigh = m_handler->param("color2");
    // remember the string params have apostrophes on either side, so we can start search at pos == 1
    reloadImages = oldBase.indexOf(Config::templateBaseColor(type).name().toLatin1(), 1) == -1
                || oldHigh.indexOf(Config::templateHighlightedBaseColor(type).name().toLatin1(), 1) == -1;
  }

  if(!m_handler || m_xsltFile != oldFile) {
    delete m_handler;
    // must read the file name to get proper context
    m_handler = new XSLTHandler(QFile::encodeName(m_xsltFile));
    if(m_checkCommonFile && !m_handler->isValid()) {
      Tellico::checkCommonXSLFile();
      m_checkCommonFile = false;
      delete m_handler;
      m_handler = new XSLTHandler(QFile::encodeName(m_xsltFile));
    }
    if(!m_handler->isValid()) {
      myWarning() << "invalid xslt handler";
      clear();
      delete m_handler;
      m_handler = 0;
      return;
    }
  }

  m_handler->addStringParam("font",     Config::templateFont(type).family().toLatin1());
  m_handler->addStringParam("fontsize", QByteArray().setNum(Config::templateFont(type).pointSize()));
  m_handler->addStringParam("bgcolor",  Config::templateBaseColor(type).name().toLatin1());
  m_handler->addStringParam("fgcolor",  Config::templateTextColor(type).name().toLatin1());
  m_handler->addStringParam("color1",   Config::templateHighlightedTextColor(type).name().toLatin1());
  m_handler->addStringParam("color2",   Config::templateHighlightedBaseColor(type).name().toLatin1());

  if(Data::Document::self()->allImagesOnDisk()) {
    m_handler->addStringParam("imgdir", QFile::encodeName(ImageFactory::imageDir()));
  } else {
    m_handler->addStringParam("imgdir", QFile::encodeName(ImageFactory::tempDir()));
  }

  m_handler->addStringParam("datadir", QFile::encodeName(Tellico::dataDir()));

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
void EntryView::slotOpenURL(const QUrl& url_) {
  if(url_.scheme() == QLatin1String("tc")) {
    // handle this internally
    emit signalAction(url_);
    return;
  }

  QUrl u = url_;
  for(DOM::Node node = nodeUnderMouse(); !node.isNull(); node = node.parentNode()) {
    if(node.nodeType() == DOM::Node::ELEMENT_NODE && static_cast<DOM::Element>(node).tagName() == "a") {
      QString href = static_cast<DOM::Element>(node).getAttribute("href").string();
      if(!href.isEmpty()) {
        // interpet url relative to document url
        u = Kernel::self()->URL().resolved(QUrl(href));
      }
      break;
    }
  }
  // open the url, m_run gets auto-deleted
  m_run = new KRun(u, view());
}

void EntryView::slotReloadEntry() {
  // this slot should only be connected in setXSLTFile()
  // must disconnect the signal first, otherwise, get an infinite loop
  disconnect(SIGNAL(completed()));
  closeUrl(); // this is needed to stop everything, for some reason
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

void EntryView::addXSLTStringParam(const QByteArray& name_, const QByteArray& value_) {
  if(!m_handler) {
    return;
  }
  m_handler->addStringParam(name_, value_);
}

void EntryView::setXSLTOptions(const Tellico::StyleOptions& opt_) {
  if(!m_handler) {
    return;
  }
  m_handler->addStringParam("font",     opt_.fontFamily.toLatin1());
  m_handler->addStringParam("fontsize", QByteArray().setNum(opt_.fontSize));
  m_handler->addStringParam("bgcolor",  opt_.baseColor.name().toLatin1());
  m_handler->addStringParam("fgcolor",  opt_.textColor.name().toLatin1());
  m_handler->addStringParam("color1",   opt_.highlightedTextColor.name().toLatin1());
  m_handler->addStringParam("color2",   opt_.highlightedBaseColor.name().toLatin1());
  m_handler->addStringParam("imgdir",   QFile::encodeName(opt_.imgDir));
}


void EntryView::slotResetColors() {
  // this will delete and reread the default colors, assuming they changed
  // better to do this elsewhere, but do it here for now
//  Config::deleteAndReset();
  delete m_handler;
  m_handler = 0;
  setXSLTFile(m_xsltFile);
}

void EntryView::resetColors() {
  // recreate gradients
  ImageFactory::createStyleImages(m_entry ? m_entry->collection()->type() : Data::Collection::Base);

  QString dir = m_handler ? QFile::decodeName(m_handler->param("imgdir")) : QString();
  if(dir.isEmpty()) {
    dir = Data::Document::self()->allImagesOnDisk() ? ImageFactory::imageDir() : ImageFactory::tempDir();
  } else {
    // it's a string param, so it has quotes on both sides
    dir = dir.mid(1);
    dir.truncate(dir.length()-1);
  }

  // this is a rather bad hack to get around the fact that the image cache is not reloaded when
  // the gradient files are changed on disk. Setting the URLArgs for write() calls doesn't seem to
  // work. So force a reload with a temp file, then catch the completed signal and repaint
  QString s = QString::fromLatin1("<html><body><img src=\"%1\"><img src=\"%2\"></body></html>")
                             .arg(dir + QLatin1String("gradient_bg.png"))
                             .arg(dir + QLatin1String("gradient_header.png"));

  delete m_tempFile;
  m_tempFile = new QTemporaryFile();
  if(!m_tempFile->open()) {
    myDebug() << "failed to open temp file";
    delete m_tempFile;
    m_tempFile = 0;
    return;
  }
  QTextStream stream(m_tempFile);
  stream << s;
  stream.flush();

  KParts::OpenUrlArguments args = arguments();
  args.setReload(true); // tell the cache to reload images
  setArguments(args);

  // don't flicker
  view()->setUpdatesEnabled(false);
  openUrl(QUrl::fromLocalFile(m_tempFile->fileName()));
  connect(this, SIGNAL(completed()), SLOT(slotReloadEntry()));
}
