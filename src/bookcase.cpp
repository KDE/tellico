/***************************************************************************
                                bookcase.cpp
                             -------------------
    begin                : Wed Aug 29 21:00:54 CEST 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// application specific includes
#include "bookcasedoc.h"
#include "bcdetailedlistview.h"
#include "bcunit.h"
#include "bcunititem.h"
#include "bookcase.h"
#include "bcuniteditwidget.h"
#include "bccollectionview.h"
#include "configdialog.h"

#include <kiconloader.h>
//#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h>
#include <ktoolbarradiogroup.h>
#include <kprogress.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstdaction.h>
#include <kaction.h>
#include <kdebug.h>
#include <kwin.h>
#include <kprogress.h>
#include <kstatusbar.h>
//#include <kprinter.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kglobal.h>
#include <kstandarddirs.h>

// include files for QT
#include <qdir.h>
#include <qsplitter.h>
#include <qvbox.h>
//#include <qprinter.h>
//#include <qpaintdevicemetrics.h>
#include <qpainter.h>
#include <qtextedit.h>

const int ID_STATUS_MSG = 1;
const int ID_STATUS_COUNT = 2;

Bookcase::Bookcase(QWidget* parent_/*=0*/, const char* name_/*=0*/)
 : KMainWindow(parent_, name_) {
  m_config = kapp->config();

  // the code checks these to see if they're NULL before allocating
  m_configDlg = 0;
  m_progress = 0;

  initWindow();
  initStatusBar();
  initActions();
  initDocument();
  initView();
  initConnections();

  readOptions();

  m_fileSave->setEnabled(false);
  m_fileSaveAs->setEnabled(false);
  m_editCut->setEnabled(false);
  m_editCopy->setEnabled(false);
  m_editPaste->setEnabled(false);
  m_editFind->setEnabled(false);
  m_editFindNext->setEnabled(false);
  m_reportsFull->setEnabled(false);

  // check to see if most recent file should be opened
  m_config->setGroup("General Options");
  if(m_config->readBoolEntry("Reopen Last File", false)
      && !m_config->readEntry("Last Open File").isEmpty()) {
  	KURL lastFile = KURL(m_config->readEntry("Last Open File"));
    slotFileOpen(lastFile);
  } else {
  	// this is needed to do the initial layout in the widgets, a bit redundant
   	m_doc->newDocument();
  }
}

Bookcase::~Bookcase() {
  // there's probably no need to do all this deleting, but it doesn't hurt
  // and I can't figure out what Qt deletes and what it doesn't.
//  delete m_doc;
//  m_doc = 0;
  delete m_collView;
  m_collView = 0;
  delete m_columnView;
  m_columnView = 0;
  delete m_editWidget;
  m_editWidget = 0;
//  delete m_configDlg;
//  m_configDlg = 0;
//  delete m_progress;
//  m_progress = 0;
}

void Bookcase::initActions() {
  m_fileNew = KStdAction::openNew(this, SLOT(slotFileNew()), actionCollection());
  m_fileOpen = KStdAction::open(this, SLOT(slotFileOpen()), actionCollection());
  m_fileOpenRecent = KStdAction::openRecent(this, SLOT(slotFileOpenRecent(const KURL&)),
                                             actionCollection());
  m_fileSave = KStdAction::save(this, SLOT(slotFileSave()), actionCollection());
  m_fileSaveAs = KStdAction::saveAs(this, SLOT(slotFileSaveAs()), actionCollection());
  m_filePrint = KStdAction::print(this, SLOT(slotFilePrint()), actionCollection());
  m_fileQuit = KStdAction::quit(this, SLOT(slotFileQuit()), actionCollection());
  m_editCut = KStdAction::cut(this, SLOT(slotEditCut()), actionCollection());
  m_editCopy = KStdAction::copy(this, SLOT(slotEditCopy()), actionCollection());
  m_editPaste = KStdAction::paste(this, SLOT(slotEditPaste()), actionCollection());
  m_editFind = KStdAction::find(this, SLOT(slotEditFind()), actionCollection());
  m_editFindNext = KStdAction::find(this, SLOT(slotEditFindNext()), actionCollection());
  m_toggleToolBar = KStdAction::showToolbar(this, SLOT(slotToggleToolBar()),
                                             actionCollection());
  m_toggleStatusBar = KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()),
                                                 actionCollection());
  m_preferences = KStdAction::preferences(this, SLOT(slotShowConfigDialog()), actionCollection());

  m_fileNewCollection = new KAction(i18n("New &Collection"), 0, this,
                        SLOT(slotFileNewCollection()), actionCollection(),
                        "file_new_collection");
  m_reportsFull = new KAction(i18n("Full Contents"), 0, this,
                        SLOT(slotReportsFull()), actionCollection(),
                        "reports_full");

  m_fileNew->setStatusText(i18n("Creates a new document"));
  m_fileOpen->setStatusText(i18n("Opens an existing document"));
  m_fileOpenRecent->setStatusText(i18n("Opens a recently used file"));
  m_fileSave->setStatusText(i18n("Saves the actual document"));
  m_fileSaveAs->setStatusText(i18n("Saves the actual document as..."));
  m_filePrint ->setStatusText(i18n("Prints out the actual document"));
  m_fileQuit->setStatusText(i18n("Quits the application"));
  m_editCut->setStatusText(i18n("Cuts the selected section and puts it to the clipboard"));
  m_editCopy->setStatusText(i18n("Copies the selected section to the clipboard"));
  m_editPaste->setStatusText(i18n("Pastes the clipboard contents to actual position"));
  m_editFind->setStatusText(i18n("Searches in the document"));
  m_toggleToolBar->setStatusText(i18n("Enables/disables the toolbar"));
  m_toggleStatusBar->setStatusText(i18n("Enables/disables the statusbar"));
  m_preferences->setStatusText(i18n("Configure the options for the application"));
  m_reportsFull->setStatusText(i18n("Creates a full report"));

  createGUI();
}

void Bookcase::initStatusBar() {
  statusBar()->insertItem(i18n("Ready."), ID_STATUS_MSG);
  statusBar()->setItemAlignment(ID_STATUS_MSG, Qt::AlignLeft | Qt::AlignVCenter);

  statusBar()->insertItem("", ID_STATUS_COUNT, 0, true);
  statusBar()->setItemAlignment(ID_STATUS_COUNT, Qt::AlignLeft | Qt::AlignVCenter);

  m_progress = new KProgress(100, statusBar());
  m_progress->setFixedHeight(statusBar()->minimumSizeHint().height());
  statusBar()->addWidget(m_progress);
  m_progress->hide();
}

void Bookcase::initWindow() {
  setIcon(KGlobal::iconLoader()->loadIcon("bookcase", KIcon::Desktop));
}

void Bookcase::initView() {

  m_split = new QSplitter(this);
  setCentralWidget(m_split);

	m_collView = new BCCollectionView(m_split);
//  m_collView->slotAddItem(m_doc->collectionById(0));

	QVBox* vbox = new QVBox(m_split);
	m_columnView = new BCDetailedListView(vbox);
//  m_columnView->slotSetContents(m_doc->collectionById(0));
	m_editWidget = new BCUnitEditWidget(vbox);
//  m_editWidget->slotSetLayout(m_doc->collectionById(0));
}

void Bookcase::initDocument() {
  m_doc = new BookcaseDoc(this);
}

void Bookcase::initConnections() {
  // allow status messages from the document
  connect(m_doc, SIGNAL(signalStatusMsg(const QString&)),
          this, SLOT(slotStatusMsg(const QString&)));

  // the two listviews signal when a unit is selected, pass it to the edit widget
  connect(m_collView, SIGNAL(signalUnitSelected(BCUnit*)),
          m_editWidget,  SLOT(slotSetContents(BCUnit*)));
  connect(m_columnView, SIGNAL(signalUnitSelected(BCUnit*)),
          m_editWidget,  SLOT(slotSetContents(BCUnit*)));

  // synchronize the selected signal between listviews
//  connect(m_collView, SIGNAL(signalUnitSelected(BCUnit*)),
//          m_columnView, SLOT(slotSetSelected(BCUnit*)));
//  connect(m_columnView, SIGNAL(signalUnitSelected(BCUnit*)),
//          m_collView, SLOT(slotSetSelected(BCUnit*)));
  // when one item is selected, clear the other
  connect(m_collView, SIGNAL(signalUnitSelected(BCUnit*)),
          m_columnView, SLOT(slotClearSelection()));
  connect(m_columnView, SIGNAL(signalUnitSelected(BCUnit*)),
          m_collView, SLOT(slotClearSelection()));

  // the two listviews also signal when they're cleared
//  connect(m_collView, SIGNAL(signalClear()),
//          m_editWidget,  SLOT(slotHandleClear()));
//  connect(m_columnView, SIGNAL(signalClear()),
//          m_editWidget,  SLOT(slotHandleClear()));

  connect(m_editWidget, SIGNAL(signalSaveUnit(BCUnit*)),
          m_doc, SLOT(slotSaveUnit(BCUnit*)));

  connect(m_columnView, SIGNAL(signalDeleteUnit(BCUnit*)),
          this, SLOT(slotDeleteUnit(BCUnit*)));
  connect(m_editWidget, SIGNAL(signalDeleteUnit(BCUnit*)),
          this, SLOT(slotDeleteUnit(BCUnit*)));

  connect(m_doc, SIGNAL(signalModified()),
          this, SLOT(slotEnableModifiedActions()));
  // overkill since a modified signal does not always mean a change in unit quantity
  connect(m_doc, SIGNAL(signalModified()),
          this, SLOT(slotUnitCount()));

  // connect the modified signal
//  connect(m_doc, SIGNAL(signalCollectionModified(BCCollection*)),
//          m_collView, SLOT(slotReset()));
//  connect(m_doc, SIGNAL(signalCollectionModified(BCCollection*)),
//          m_columnView, SLOT(slotReset()));
//  connect(m_doc, SIGNAL(signalCollectionModified(BCCollection*)),
//          m_editWidget, SLOT(slotReset()));

  connect(m_doc, SIGNAL(signalCollectionAdded(BCCollection*)),
          m_collView, SLOT(slotAddItem(BCCollection*)));
  connect(m_doc, SIGNAL(signalCollectionAdded(BCCollection*)),
          m_editWidget, SLOT(slotSetCollection(BCCollection*)));
  connect(m_doc, SIGNAL(signalCollectionAdded(BCCollection*)),
          m_columnView, SLOT(slotSetContents(BCCollection*)));

  connect(m_doc, SIGNAL(signalCollectionDeleted(BCCollection*)),
          m_collView, SLOT(slotRemoveItem(BCCollection*)));

  // connect the added signal to both listviews
  connect(m_doc, SIGNAL(signalUnitAdded(BCUnit*)),
          m_collView, SLOT(slotAddItem(BCUnit*)));
  connect(m_doc, SIGNAL(signalUnitAdded(BCUnit*)),
          m_columnView, SLOT(slotAddItem(BCUnit*)));
  connect(m_doc, SIGNAL(signalUnitAdded(BCUnit*)),
          m_editWidget, SLOT(slotUpdateCompletions(BCUnit*)));

  // connect the modified signal to both
  connect(m_doc, SIGNAL(signalUnitModified(BCUnit*)),
          m_collView, SLOT(slotModifyItem(BCUnit*)));
  connect(m_doc, SIGNAL(signalUnitModified(BCUnit*)),
          m_columnView, SLOT(slotModifyItem(BCUnit*)));
  connect(m_doc, SIGNAL(signalUnitModified(BCUnit*)),
          m_editWidget, SLOT(slotUpdateCompletions(BCUnit*)));

  // connect the deleted signal to both listviews
  connect(m_doc, SIGNAL(signalUnitDeleted(BCUnit*)),
          m_collView, SLOT(slotRemoveItem(BCUnit*)));
  connect(m_doc, SIGNAL(signalUnitDeleted(BCUnit*)),
          m_columnView, SLOT(slotRemoveItem(BCUnit*)));

  connect(m_collView, SIGNAL(signalRenameCollection(int, const QString&)),
          m_doc, SLOT(slotRenameCollection(int, const QString&)));

  connect(m_doc, SIGNAL(signalFractionDone(float)),
          this, SLOT(slotUpdateFractionDone(float)));
}

// These are general options.
// The options that can be changed in the "Configuration..." dialog
// are taken care of by the BCConfigDlg object.
void Bookcase::saveOptions() {
//  kdDebug() << "Bookcase::saveOptions()" << endl;
  // for some reason, the m_config pointer is getting changed, but
  // I can't figure out where, so just to be on the safe side
  if(m_config != kapp->config()) {
    m_config = kapp->config();
  }
  m_config->setGroup("General Options");
  m_config->writeEntry("Geometry", size());
  m_config->writeEntry("Show Toolbar", m_toggleToolBar->isChecked());
  m_config->writeEntry("Show Statusbar", m_toggleStatusBar->isChecked());
  m_config->writeEntry("ToolBarPos", static_cast<int>(toolBar("mainToolBar")->barPos()));
  m_fileOpenRecent->saveEntries(m_config, "Recent Files");
  if(m_doc->URL().fileName() != i18n("Untitled")) {
    m_config->writeEntry("Last Open File", doc()->URL().url());
  }

  QListIterator<BCCollection> collIt(doc()->collectionList());
  for( ; collIt.current(); ++collIt) {
    QValueList<int> widthList;
    for(int i = 0; i < m_columnView->columns(); ++i) {
      widthList.append(m_columnView->columnWidth(i));
    }
    m_config->writeEntry("Column Widths - " + collIt.current()->unitName(), widthList);
  }

  m_config->writeEntry("Main Window Splitter Sizes", m_split->sizes());
  
//  kdDebug() << "Bookcase::saveOptions - done" << endl;
}

void Bookcase::readOptions() {
  // for some reason, the m_config pointer is getting changed, but
  // I can't figure out where, so just to be on the safe side
  if(m_config != kapp->config()) {
    m_config = kapp->config();
  }
  m_config->setGroup("General Options");

  // bar status settings
  bool bViewToolBar = m_config->readBoolEntry("Show Toolbar", true);
  m_toggleToolBar->setChecked(bViewToolBar);
  slotToggleToolBar();

  bool bViewStatusBar = m_config->readBoolEntry("Show Statusbar", true);
  m_toggleStatusBar->setChecked(bViewStatusBar);
  slotToggleStatusBar();

  // bar position settings
  KToolBar::BarPosition toolBarPos;
  toolBarPos = static_cast<KToolBar::BarPosition>(m_config->readNumEntry("ToolBarPos",
                                                  KToolBar::Top));
  toolBar("mainToolBar")->setBarPos(toolBarPos);

  // initialize the recent file list
  m_fileOpenRecent->loadEntries(m_config, "Recent Files");

  QSize size = m_config->readSizeEntry("Geometry");
  if(!size.isEmpty()) {
    resize(size);
  }

  QValueList<int> splitList = m_config->readIntListEntry("Main Window Splitter Sizes");
  if(!splitList.empty()) {
    m_split->setSizes(splitList);
  }

  bool autoCapitals = m_config->readBoolEntry("Auto Capitalization", true);
  BCAttribute::setAutoCapitalization(autoCapitals);

  QStringList articles = m_config->readListEntry("Articles", ',');
  if(articles.isEmpty()) {
    articles = BCAttribute::defaultArticleList();
  }
  BCAttribute::setArticleList(articles);

  QStringList suffixes = m_config->readListEntry("Name Suffixes", ',');
  if(suffixes.isEmpty()) {
    suffixes = BCAttribute::defaultSuffixList();
  }
  BCAttribute::setSuffixList(suffixes);
}

QValueList<int> Bookcase::readColumnWidths(const QString& unitName_) {
  // for some reason, the m_config pointer is getting changed, but
  // I can't figure out where, so just to be on the safe side
  if(m_config != kapp->config()) {
    m_config = kapp->config();
  }
  m_config->setGroup("General Options");
  return m_config->readIntListEntry("Column Widths - " + unitName_);
}

void Bookcase::saveProperties(KConfig* cfg_) {
  if(m_doc->URL().fileName() != i18n("Untitled") && !m_doc->isModified()) {
    // saving to tempfile not necessary
  } else {
    KURL url = m_doc->URL();
    cfg_->writeEntry("filename", url.url());
    cfg_->writeEntry("modified", m_doc->isModified());
    QString tempname = KURL::encode_string(kapp->tempSaveName(url.url()));
    KURL tempurl(tempname);
    m_doc->saveDocument(tempurl);
  }
}


void Bookcase::readProperties(KConfig* cfg_) {
  QString filename = cfg_->readEntry("filename", "");
  KURL url(filename);
  bool modified = cfg_->readBoolEntry("modified", false);
  if(modified) {
    bool canRecover;
    QString tempname = kapp->checkRecoverFile(filename, canRecover);
    KURL tempurl(tempname);

    if(canRecover) {
      m_doc->openDocument(tempurl);
      m_doc->setModified(true);
      setCaption(tempurl.fileName(), true);
      QFile::remove(tempname);
    }
  } else {
    if(!filename.isEmpty()) {
      m_doc->openDocument(url);
      setCaption(url.fileName(), false);
    }
  }
}

bool Bookcase::queryClose() {
//  kdDebug() << "Bookcase::queryClose()" << endl;
  return m_doc->saveModified();
}

bool Bookcase::queryExit() {
//  kdDebug() << "Bookcase::queryExit()" << endl;
  saveOptions();
  return true;
}

BookcaseDoc* Bookcase::doc() {
  return m_doc;
}

void Bookcase::slotFileNew() {
  slotStatusMsg(i18n("Creating new document..."));

  if(!m_doc->saveModified()) {
    // here saving wasn't successful
  } else {
    m_doc->newDocument();
    setCaption(m_doc->URL().fileName(), false);
  }

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotFileOpen() {
  slotStatusMsg(i18n("Opening file..."));

  if(!m_doc->saveModified()) {
     // here saving wasn't successful
  } else {
    KURL url = KFileDialog::getOpenURL(QString::null,
        i18n("*.bc|Bookcase files (*.bc)\n*.xml|XML files (*.xml)\n*|All files"),
        this, i18n("Open File..."));
    if(!url.isEmpty()) {
      slotFileOpen(url);
    }
  }
  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotFileOpen(const KURL& url_) {
  slotStatusMsg(i18n("Opening file..."));
  bool success = openURL(url_);
  if(success) {
    m_fileOpenRecent->addURL(url_);
  }

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotFileOpenRecent(const KURL& url_) {
  slotStatusMsg(i18n("Opening file..."));

  if(!m_doc->saveModified()) {
    // here saving wasn't successful
  } else {
    bool success = openURL(url_);
    if(!success) {
      m_fileOpenRecent->removeURL(url_);
    }
  }

  slotStatusMsg(i18n("Ready."));
}

bool Bookcase::openURL(const KURL& url_) {
  // since a lot of updates will happen with a large file, disable them
  m_columnView->setUpdatesEnabled(false);
  m_collView->setUpdatesEnabled(false);
  // disable sorting
  m_columnView->setSorting(-1);
  m_collView->setSorting(-1);

  // try to open document
  bool success = m_doc->openDocument(url_);

  // re-enable updates
  m_columnView->setUpdatesEnabled(true);
  m_collView->setUpdatesEnabled(true);
  // re-enable sorting
  m_columnView->setSorting(0, true);
  m_collView->setSorting(0, true);

  if(success) {
    m_doc->setModified(false);
    setCaption(m_doc->URL().fileName(), false);
    // collapse all the groups (depth=1)
    m_collView->slotCollapseAll(1);
    // expand the collections
    m_collView->slotExpandAll(0);
    // disable save action since the file is just opened
    m_fileSave->setEnabled(false);
    m_fileSaveAs->setEnabled(true);
  }

  return success;
}

void Bookcase::slotFileSave() {
  slotStatusMsg(i18n("Saving file..."));

  if(m_doc->URL().filename() != i18n("Untitled")) {
    m_doc->saveDocument(m_doc->URL());
  } else {
    slotFileSaveAs();
  }
  m_fileSave->setEnabled(false);
  setCaption(m_doc->URL().fileName(), false);

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotFileSaveAs() {
  slotStatusMsg(i18n("Saving file with a new filename..."));

  KURL url = KFileDialog::getSaveURL(QDir::currentDirPath(),
        i18n("*.bc|Bookcase files (*.bc)\n*.xml|XML files (*.xml)\n*|All files"),
        this, i18n("Save as..."));
  if(!url.isEmpty()) {
    m_doc->saveDocument(url);
    m_fileOpenRecent->addURL(url);
    setCaption(m_doc->URL().fileName(), false);
  }

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotFilePrint() {
  slotStatusMsg(i18n("Printing..."));

  QString filename("bookcase-by-author.xsl");
  QString xsltfile = KGlobal::dirs()->findResource("appdata", filename);
  if(xsltfile.isNull()) {
    kdWarning() << i18n("Unable to find %1 in standard KDE directories.").arg(filename) << endl;
  } else {
    QString html = doc()->exportHTML(xsltfile, true);
    KHTMLPart *w = new KHTMLPart();
    w->begin();
    w->write(html);
    w->end();
    w->view()->print();
    delete w;
    
//  KPrinter* printer = new KPrinter();
//
//  if(printer->setup(this)) {
//    QTextEdit* view = new QTextEdit(this);
//    view->setTextFormat(Qt::RichText);
//    view->setText(html);
//
//    QPainter painter;
//
//    // start painting
//    painter.begin(printer);
//    painter.drawPixmap(0, 0, QPixmap::grabWidget(view));
//
//    // stop painting, this will automatically send the print data to the printer
//    painter.end();
//  }
//
//  delete printer;

  }
  
  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotFileQuit() {
  slotStatusMsg(i18n("Exiting..."));

  // this gets called in queryExit() anyway
  //saveOptions();
  close();

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotEditCut() {
  slotStatusMsg(i18n("Cutting selection..."));

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotEditCopy() {
  slotStatusMsg(i18n("Copying selection to clipboard..."));

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotEditPaste() {
  slotStatusMsg(i18n("Inserting clipboard contents..."));

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotEditFind() {
}

void Bookcase::slotEditFindNext() {
}

void Bookcase::slotToggleToolBar() {
  slotStatusMsg(i18n("Toggling toolbar..."));

  if(m_toggleToolBar->isChecked()) {
    toolBar("mainToolBar")->show();
  } else {
    toolBar("mainToolBar")->hide();
  }

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotToggleStatusBar() {
  slotStatusMsg(i18n("Toggle the statusbar..."));

  if(m_toggleStatusBar->isChecked()) {
    statusBar()->show();
  } else {
    statusBar()->hide();
  }

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotShowConfigDialog() {
  if(!m_configDlg) {
    m_configDlg = new ConfigDialog(this);
    m_configDlg->readConfiguration(m_config);
    connect(m_configDlg, SIGNAL(signalConfigChanged()), this, SLOT(slotHandleConfigChange()));
    connect(m_configDlg, SIGNAL(finished()), this, SLOT(slotHideConfigDialog()));
    m_configDlg->show();
  } else {
    KWin::setActiveWindow(m_configDlg->winId());
  }
}

void Bookcase::slotHideConfigDialog() {
  if(m_configDlg) {
    m_configDlg->delayedDestruct();
    m_configDlg = 0;
  }
}

void Bookcase::slotStatusMsg(const QString& text_) {
  statusBar()->clear();
  // add a space at the beginning and end for asthetic reasons
  statusBar()->changeItem(" "+text_+" ", ID_STATUS_MSG);
}

void Bookcase::slotUnitCount() {
  QString text(" ");
  QListIterator<BCCollection> it(m_doc->collectionList());
  for( ; it.current(); ++it) {
    text += i18n("%1 %2").arg(it.current()->unitTitle()).arg(i18n("Total"));
    text += ": " + QString::number(it.current()->unitCount()) + " ";
  }
  statusBar()->changeItem(text, ID_STATUS_COUNT);
}

void Bookcase::slotDeleteUnit(BCUnit* unit_) {
  m_doc->slotDeleteUnit(unit_);
  m_editWidget->slotSetContents(0);
  m_columnView->slotSetSelected(0);
  m_collView->slotSetSelected(0);
}

void Bookcase::slotFileNewCollection() {
  kdDebug() << "Bookcase::slotFileNewCollection()" << endl;
}

void Bookcase::slotReportsFull() {
  kdDebug() << "Bookcase::slotReportsFull()" << endl;
}

void Bookcase::slotEnableModifiedActions() {
  setCaption(m_doc->URL().fileName(), true);
  m_fileSave->setEnabled(true);
}

void Bookcase::slotUpdateFractionDone(float f_) {
  // first check bounds
  f_ = (f_ < 0.0) ? 0.0 : f_;
  f_ = (f_ > 1.0) ? 1.0 : f_;

  if(!m_progress->isVisible()) {
    m_progress->show();
  }
  m_progress->setValue(static_cast<int>(f_ * 100.0));
  kapp->processEvents();

  if(f_ == 1.0) {
    m_progress->hide();
  }
}

void Bookcase::slotHandleConfigChange() {
  // for some reason, the m_config pointer is getting changed, but
  // I can't figure out where, so just to be on the safe side
  if(m_config != kapp->config()) {
    m_config = kapp->config();
  }
  m_configDlg->saveConfiguration(m_config);
}

QDomDocument Bookcase::collectionViewTree() {
  QDomDocument doc("bookcase");
  doc.appendChild(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""));
  QDomElement bcelem = doc.createElement("bookcase");
  doc.appendChild(bcelem);

  ParentItem* collItem = (ParentItem*)m_collView->firstChild();
  for( ; collItem; collItem = (ParentItem*)collItem->nextSibling()) {
    QDomElement collElem = doc.createElement("collection");
    doc.documentElement().appendChild(collElem);
    collElem.setAttribute("title", collItem->text(0));

    ParentItem* authorItem = (ParentItem*)collItem->firstChild();
    for( ; authorItem; authorItem = (ParentItem*)authorItem->nextSibling()) {
    
      BCUnitItem* bookItem = (BCUnitItem*)authorItem->firstChild();
      for( ; bookItem; bookItem = (BCUnitItem*)bookItem->nextSibling()) {
      
        QString unitName = m_doc->collectionById(collItem->id())->unitName();
        QDomElement unitElem = doc.createElement(unitName);
        
        QDomElement titleElem = doc.createElement("title");
        titleElem.appendChild(doc.createTextNode(bookItem->text(0)));
        
        QDomElement authorElem = doc.createElement("author");
        authorElem.appendChild(doc.createTextNode(authorItem->text(0)));
        
        unitElem.appendChild(titleElem);
        unitElem.appendChild(authorElem);
        collElem.appendChild(unitElem);
      } // end book loop
    } // end author loop
  } // end collection loop

  return doc;
}
