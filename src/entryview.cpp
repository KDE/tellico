/***************************************************************************
    Copyright (C) 2003-2020 Robby Stephenson <robby@periapsis.org>
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
#include "utils/datafileregistry.h"
#include "config/tellico_config.h"
#include "gui/drophandler.h"
#include "utils/cursorsaver.h"
#include "document.h"
#include "tellico_debug.h"

#include <KMessageBox>
#include <KLocalizedString>
#include <KStandardAction>
#include <KColorScheme>

#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QClipboard>
#include <QDomDocument>
#include <QTemporaryFile>
#include <QApplication>
#include <QDesktopServices>
#include <QMenu>

#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QPrinter>
#include <QPrinterInfo>
#include <QPrintDialog>
#include <QEventLoop>

using Tellico::EntryView;

using Tellico::EntryViewPage;

EntryViewPage::EntryViewPage(QWidget* parent)
    : QWebEnginePage(parent) {
  // The right way to do this should be to use a transparent color
  // but for non-KDE sessions, QPalette and KColorScheme are not necessarily in-sync
  // In such cases, parent->palette().window().color() would be more appropriate but
  // that breaks on a Plasma session. Using KCOlorScheme is best compromise for now
  setBackgroundColor(KColorScheme().background().color());
//  setBackgroundColor(Qt::transparent);
  settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
  settings()->setAttribute(QWebEngineSettings::PluginsEnabled, false);
  settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
  settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
}

bool EntryViewPage::acceptNavigationRequest(const QUrl& url_, QWebEnginePage::NavigationType type_, bool isMainFrame_) {
  Q_UNUSED(isMainFrame_);

  if(url_.scheme() == QLatin1String("tc")) {
    // handle this internally
    Q_EMIT signalTellicoAction(url_);
    return false;
  }

  if(type_ == QWebEnginePage::NavigationTypeLinkClicked) {
    // do not load this new url inside the entry view, return false
    openExternalLink(url_);
    return false;
  }

  return true;
}

// intercept window open commands, including target=_blank
// see https://bugs.kde.org/show_bug.cgi?id=445871
QWebEnginePage* EntryViewPage::createWindow(QWebEnginePage::WebWindowType type_) {
  Q_UNUSED(type_);
  auto page = new QWebEnginePage(this);
  connect(page, &QWebEnginePage::urlChanged, this, [this](const QUrl& u) {
    openExternalLink(u);
    auto page = static_cast<QWebEnginePage*>(sender());
    page->action(QWebEnginePage::Stop)->trigger(); // stop the loading, further is unnecessary
    page->deleteLater();
  });
  return page;
}

void EntryViewPage::openExternalLink(const QUrl& url_) {
  const QUrl finalUrl = Kernel::self()->URL().resolved(url_);
  QDesktopServices::openUrl(finalUrl);
}

EntryView::EntryView(QWidget* parent_) : QWebEngineView(parent_),
    m_handler(nullptr), m_tempFile(nullptr), m_useGradientImages(true), m_checkCommonFile(true) {
  auto page = new EntryViewPage(this);
  setPage(page);
  if(m_printer.resolution() < 300) {
    m_printer.setResolution(300);
  }

  connect(this, &QWebEngineView::loadFinished, this, [](bool b) {
    if(!b) myDebug() << "EntryView - failed to load view";
  });
  connect(page, &EntryViewPage::signalTellicoAction,
          this, &EntryView::signalTellicoAction);

  setAcceptDrops(true);
  auto drophandler = new DropHandler(this);
  installEventFilter(drophandler);

  clear(); // needed for initial layout
}

EntryView::~EntryView() {
  delete m_handler;
  m_handler = nullptr;
  delete m_tempFile;
  m_tempFile = nullptr;
}

void EntryView::clear() {
  m_entry = nullptr;

  // just clear the view
  setUrl(QUrl());
  if(!m_textToShow.isEmpty()) {
    // the welcome page references local images, which won't load when passing HTML directly
    // so the base Url needs to be set to file://
    // see https://bugreports.qt.io/browse/QTBUG-55902#comment-335945
    // passing "disable-web-security" to QApplication is another option
    page()->setHtml(m_textToShow, QUrl(QStringLiteral("file://")));
  }
}

void EntryView::showEntries(Tellico::Data::EntryList entries_) {
  if(!entries_.isEmpty()) {
    showEntry(entries_.first());
  }
}

void EntryView::showEntry(Tellico::Data::EntryPtr entry_) {
  if(!entry_) {
    clear();
    return;
  }

  m_textToShow.clear();
  if(!m_handler || !m_handler->isValid()) {
    setXSLTFile(m_xsltFile);
  }
  if(!m_handler || !m_handler->isValid()) {
    myWarning() << "no xslt handler";
    return;
  }

  // check if the gradient images need to be written again which might be the case if the collection is different
  // and using local directories for storage
  if(entry_ && (!m_entry || m_entry->collection() != entry_->collection()) &&
     ImageFactory::cacheDir() == ImageFactory::LocalDir) {
    // use entry_ instead of m_entry since that's the new entry to show
    ImageFactory::createStyleImages(entry_->collection()->type());
  }

  m_entry = entry_;

  Export::TellicoXMLExporter exporter(m_entry->collection());
  exporter.setEntries(Data::EntryList() << m_entry);
  long opt = exporter.options();
  // verify images for the view
  opt |= Export::ExportVerifyImages;
  opt |= Export::ExportComplete;
  // use absolute links
  opt |= Export::ExportAbsoluteLinks;
  // on second thought, don't auto-format everything, just clean it
  if(m_entry->collection()->type() == Data::Collection::Bibtex) {
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

  const QString html = m_handler->applyStylesheet(dom.toString());
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
  myWarning() << "EntryView::showEntry() - turn me off!";
  QFile f2(QLatin1String("/tmp/test.html"));
  if(f2.open(QIODevice::WriteOnly)) {
    QTextStream t(&f2);
    t << html;
  }
  f2.close();
#endif

//  myDebug() << html;

  // limit is 2 MB after percent encoding, etc., so give some padding
  if(html.size() > 1200000) {
    delete m_tempFile;
    m_tempFile = new QTemporaryFile(QDir::tempPath() + QLatin1String("/tellicoview_XXXXXX") + QLatin1String(".html"));
    m_tempFile->open();
    QTextStream ts(m_tempFile);
    ts.setEncoding(QStringConverter::Utf8);
    ts << html;
    // TODO: need to handle relative links
    page()->load(QUrl::fromLocalFile(m_tempFile->fileName()));
  } else {
    // by setting the xslt file as the URL, any images referenced in the xslt "theme" can be found
    // by simply using a relative path in the xslt file
    page()->setHtml(html, QUrl::fromLocalFile(m_xsltFile));
  }
}

void EntryView::showText(const QString& text_) {
  m_textToShow = text_;
  clear(); // shows the default text
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
    const QString templateDir = QStringLiteral("entry-templates/");
    m_xsltFile = DataFileRegistry::self()->locate(templateDir + file_);
    if(m_xsltFile.isEmpty()) {
      if(!file_.isEmpty()) {
        myWarning() << "can't locate" << file_;
      }
      m_xsltFile = DataFileRegistry::self()->locate(templateDir + QLatin1String("Fancy.xsl"));
      if(m_xsltFile.isEmpty()) {
        QString str = QStringLiteral("<qt>");
        str += i18n("Tellico is unable to locate the default entry stylesheet.");
        str += QLatin1Char(' ');
        str += i18n("Please check your installation.");
        str += QLatin1String("</qt>");
        KMessageBox::error(this, str);
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
      m_handler = nullptr;
      return;
    }
  }

  m_handler->addStringParam("font",     Config::templateFont(type).family().toLatin1());
  m_handler->addStringParam("fontsize", QByteArray().setNum(Config::templateFont(type).pointSize()));
  m_handler->addStringParam("bgcolor",  Config::templateBaseColor(type).name().toLatin1());
  m_handler->addStringParam("fgcolor",  Config::templateTextColor(type).name().toLatin1());
  m_handler->addStringParam("color1",   Config::templateHighlightedTextColor(type).name().toLatin1());
  m_handler->addStringParam("color2",   Config::templateHighlightedBaseColor(type).name().toLatin1());
  m_handler->addStringParam("linkcolor",Config::templateLinkColor(type).name().toLatin1());

  if(Data::Document::self()->allImagesOnDisk()) {
    m_handler->addStringParam("imgdir", ImageFactory::imageDir().toEncoded());
  } else {
    m_handler->addStringParam("imgdir", ImageFactory::tempDir().toEncoded());
  }
  m_handler->addStringParam("datadir", QUrl::fromLocalFile(Tellico::installationDir()).toEncoded());

  // if we don't have to reload the images, then just show the entry and we're done
  if(reloadImages) {
    // now, have to recreate images and refresh cache
    resetColors();
  } else {
    showEntry(m_entry);
  }
}

void EntryView::copy() {
  pageAction(QWebEnginePage::Copy)->trigger();
}

void EntryView::slotRefresh() {
  setXSLTFile(m_xsltFile);
  showEntry(m_entry);
}

void EntryView::changeEvent(QEvent* event_) {
  // this will delete and reread the default colors, assuming they changed
  if(event_->type() == QEvent::PaletteChange ||
     event_->type() == QEvent::FontChange ||
     event_->type() == QEvent::ApplicationFontChange) {
    resetView();
  }
  QWebEngineView::changeEvent(event_);
}

void EntryView::slotReloadEntry() {
  // this slot should only be connected in setXSLTFile()
  // must disconnect the signal first, otherwise, get an infinite loop
  disconnect(this, &EntryView::loadFinished, this, &EntryView::slotReloadEntry);
  setUpdatesEnabled(true);

  if(m_entry) {
    showEntry(m_entry);
  } else {
    // setXSLTFile() writes some html to clear the image cache
    // but we don't want to see that, so just clear everything
    clear();
  }
  delete m_tempFile;
  m_tempFile = nullptr;
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
  m_handler->addStringParam("linkcolor",opt_.linkColor.name().toLatin1());
  m_handler->addStringParam("imgdir",   QFile::encodeName(opt_.imgDir));
}

void EntryView::resetView() {
  delete m_handler;
  m_handler = nullptr;
  // Many of the template style parameters use default values. The only way that
  // KConfigSkeleton can be updated is to delete the existing config object, which will then be recreated
  delete Config::self();
  setXSLTFile(m_xsltFile); // this ends up calling resetColors()
}

void EntryView::resetColors() {
  // recreate gradients
  ImageFactory::createStyleImages(m_entry ? m_entry->collection()->type() : Data::Collection::Base);

  QString dir = m_handler ? QFile::decodeName(m_handler->param("imgdir")) : QString();
  if(dir.isEmpty()) {
    dir = Data::Document::self()->allImagesOnDisk() ?
            ImageFactory::imageDir().url() :
            ImageFactory::tempDir().url();
  } else {
    // it's a string param, so it has quotes on both sides
    dir = dir.mid(1);
    dir.truncate(dir.length()-1);
  }

  delete m_tempFile;
  m_tempFile = new QTemporaryFile();
  if(!m_tempFile->open()) {
    myDebug() << "failed to open temp file";
    delete m_tempFile;
    m_tempFile = nullptr;
    return;
  }

  // this is a rather bad hack to get around the fact that the image cache is not reloaded when
  // the gradient files are changed on disk. Setting the URLArgs for write() calls doesn't seem to
  // work. So force a reload with a temp file, then catch the completed signal and repaint
  QString s = QStringLiteral("<html><body><img src=\"%1\"><img src=\"%2\"></body></html>")
                             .arg(dir + QLatin1String("gradient_bg.png"),
                                  dir + QLatin1String("gradient_header.png"));
  QTextStream stream(m_tempFile);
  stream << s;
  stream.flush();

  // don't flicker
  setUpdatesEnabled(false);
  load(QUrl::fromLocalFile(m_tempFile->fileName()));
  connect(this, &EntryView::loadFinished, this, &EntryView::slotReloadEntry);
}

void EntryView::contextMenuEvent(QContextMenuEvent* event_) {
  QMenu menu(this);
  // can't use the KStandardAction for copy since I don't know what the receiver or trigger target is
  QAction* standardCopy = KStandardAction::copy(nullptr, nullptr, &menu);
  QAction* pageCopyAction = pageAction(QWebEnginePage::Copy);
  pageCopyAction->setIcon(standardCopy->icon());
  menu.addAction(pageCopyAction);

  QAction* printAction = KStandardAction::print(this, &EntryView::slotPrint, this);
  // remove shortcut since this is specific to the view widget
  printAction->setShortcut(QKeySequence());
  menu.addAction(printAction);
  menu.exec(event_->globalPos());
}

void EntryView::slotPrint() {
  QPointer<QPrintDialog> dialog = new QPrintDialog(&m_printer, this);
  if(dialog->exec() != QDialog::Accepted) {
    return;
  }
  Tellico::GUI::CursorSaver cs(Qt::WaitCursor);
  print(&m_printer);
}
