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
#include "bcgroupview.h"
#include "configdialog.h"
#include "bclabelaction.h"

#include <kiconloader.h>
#include <kfiledialog.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <kprogress.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstdaction.h>
#include <kaction.h>
#include <kdebug.h>
#include <kwin.h>
#include <kprogress.h>
#include <kstatusbar.h>
#include <kprinter.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kstringhandler.h>

// include files for QT
#include <qdir.h>
#include <qsplitter.h>
#include <qvbox.h>
#include <qpaintdevicemetrics.h>
#include <qpainter.h>
#include <qtextedit.h>

const int ID_STATUS_MSG = 1;
const int ID_STATUS_COUNT = 2;
//const int PRINTED_PAGE_OVERLAP = 10;

Bookcase::Bookcase(QWidget* parent_/*=0*/, const char* name_/*=0*/)
    : KMainWindow(parent_, name_), m_config(kapp->config()),
      m_progress(0), m_configDlg(0) {

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

  // check to see if most recent file should be opened
  m_config->setGroup("General Options");
  if(m_config->readBoolEntry("Reopen Last File", false)
      && !m_config->readEntry("Last Open File").isEmpty()) {
    KURL lastFile = KURL(m_config->readEntry("Last Open File"));
    slotFileOpen(lastFile);
  } else {
    // this is needed to do the initial layout in the widgets, a bit redundant
    slotFileNew();
  }
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
  m_editFindNext = KStdAction::findNext(this, SLOT(slotEditFindNext()),
                                        actionCollection());
  m_toggleToolBar = KStdAction::showToolbar(this, SLOT(slotToggleToolBar()),
                                            actionCollection());
  m_toggleStatusBar = KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()),
                                                actionCollection());
  m_preferences = KStdAction::preferences(this, SLOT(slotShowConfigDialog()),
                                          actionCollection());

  m_fileNewCollection = new KAction(i18n("New &Collection"), 0, this,
                                    SLOT(slotFileNewCollection()),
                                    actionCollection(),
                                    "file_new_collection");

  m_toggleCollectionBar = new KToggleAction(i18n("Show Co&llection ToolBar"), 0, this,
                                            SLOT(slotToggleCollectionBar()),
                                            actionCollection(),
                                            "toggle_collection_bar");

  (void) new BCLabelAction(i18n("Group by: "), 0,
                           actionCollection(),
                           "change_unit_grouping_label");
  m_unitGrouping = new KSelectAction(i18n("Grouping"), 0, this,
                                     SLOT(slotChangeGrouping()),
                                     actionCollection(),
                                     "change_unit_grouping");

  m_fileNew->setStatusText(i18n("Create a new document"));
  m_fileOpen->setStatusText(i18n("Open an existing document"));
  m_fileOpenRecent->setStatusText(i18n("Open a recently used file"));
  m_fileSave->setStatusText(i18n("Save the actual document"));
  m_fileSaveAs->setStatusText(i18n("Save the actual document as..."));
  m_filePrint->setStatusText(i18n("Print the contents of the document..."));
  m_fileQuit->setStatusText(i18n("Quit the application"));
  m_editCut->setStatusText(i18n("Cut the selected section and puts it to the clipboard"));
  m_editCopy->setStatusText(i18n("Copy the selected section to the clipboard"));
  m_editPaste->setStatusText(i18n("Paste the clipboard contents to actual position"));
  m_editFind->setStatusText(i18n("Search in the document..."));
  m_toggleToolBar->setStatusText(i18n("Enable/disable the toolbar"));
  m_toggleStatusBar->setStatusText(i18n("Enable/disable the statusbar"));
  m_preferences->setStatusText(i18n("Configure the options for the application..."));
  m_unitGrouping->setStatusText(i18n("Change the grouping of the collection"));
  m_toggleCollectionBar->setStatusText(i18n("Enable/disable the collection toolbar"));

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

  m_groupView = new BCGroupView(m_split);
//  m_groupView->slotAddItem(m_doc->collectionById(0));

  QVBox* vbox = new QVBox(m_split);
  m_detailedView = new BCDetailedListView(vbox);
//  m_detailedView->slotSetContents(m_doc->collectionById(0));
  m_editWidget = new BCUnitEditWidget(vbox);
//  m_editWidget->slotSetLayout(m_doc->collectionById(0));
}

void Bookcase::initDocument() {
  m_doc = new BookcaseDoc(this);
}

void Bookcase::initConnections() {
  // allow status messages from the document
  connect(m_doc, SIGNAL(signalStatusMsg(const QString&)),
           SLOT(slotStatusMsg(const QString&)));

  // the two listviews signal when a unit is selected, pass it to the edit widget
  connect(m_groupView, SIGNAL(signalUnitSelected(BCUnit*)),
          m_editWidget, SLOT(slotSetContents(BCUnit*)));
  connect(m_detailedView, SIGNAL(signalUnitSelected(BCUnit*)),
          m_editWidget, SLOT(slotSetContents(BCUnit*)));

  // synchronize the selected signal between listviews
//  connect(m_groupView, SIGNAL(signalUnitSelected(BCUnit*)),
//          m_detailedView, SLOT(slotSetSelected(BCUnit*)));
//  connect(m_detailedView, SIGNAL(signalUnitSelected(BCUnit*)),
//          m_groupView, SLOT(slotSetSelected(BCUnit*)));
  // when one item is selected, clear the other
  connect(m_groupView, SIGNAL(signalUnitSelected(BCUnit*)),
          m_detailedView, SLOT(slotClearSelection()));
  connect(m_detailedView, SIGNAL(signalUnitSelected(BCUnit*)),
          m_groupView, SLOT(slotClearSelection()));

  // the two listviews also signal when they're cleared
//  connect(m_groupView, SIGNAL(signalClear()),
//          m_editWidget,  SLOT(slotHandleClear()));
//  connect(m_detailedView, SIGNAL(signalClear()),
//          m_editWidget,  SLOT(slotHandleClear()));

  connect(m_editWidget, SIGNAL(signalSaveUnit(BCUnit*)),
          m_doc, SLOT(slotSaveUnit(BCUnit*)));

  connect(m_detailedView, SIGNAL(signalDeleteUnit(BCUnit*)),
          SLOT(slotDeleteUnit(BCUnit*)));
  connect(m_editWidget, SIGNAL(signalDeleteUnit(BCUnit*)),
          SLOT(slotDeleteUnit(BCUnit*)));

  connect(m_doc, SIGNAL(signalModified()), SLOT(slotEnableModifiedActions()));
  // overkill since a modified signal does not always mean a change in unit quantity
  connect(m_doc, SIGNAL(signalModified()), SLOT(slotUnitCount()));

//  connect(m_doc, SIGNAL(signalCollectionAdded(BCCollection*)),
//          m_groupView, SLOT(slotAddItem(BCCollection*)));
  connect(m_doc, SIGNAL(signalCollectionAdded(BCCollection*)),
          SLOT(slotUpdateCollection(BCCollection*)));
  connect(m_doc, SIGNAL(signalCollectionAdded(BCCollection*)),
          m_groupView, SLOT(slotAddCollection(BCCollection*)));
  connect(m_doc, SIGNAL(signalCollectionAdded(BCCollection*)),
          m_editWidget, SLOT(slotSetCollection(BCCollection*)));
  connect(m_doc, SIGNAL(signalCollectionAdded(BCCollection*)),
          m_detailedView, SLOT(slotSetContents(BCCollection*)));

  connect(m_doc, SIGNAL(signalCollectionDeleted(BCCollection*)),
          m_groupView, SLOT(slotRemoveItem(BCCollection*)));

  // connect the added signal to both listviews
//  connect(m_doc, SIGNAL(signalUnitAdded(BCUnit*)),
//          m_groupView, SLOT(slotAddItem(BCUnit*)));
  connect(m_doc, SIGNAL(signalUnitAdded(BCUnit*)),
          m_detailedView, SLOT(slotAddItem(BCUnit*)));
  connect(m_doc, SIGNAL(signalUnitAdded(BCUnit*)),
          m_editWidget, SLOT(slotUpdateCompletions(BCUnit*)));

  // connect the modified signal to both
//  connect(m_doc, SIGNAL(signalUnitModified(BCUnit*)),
//          m_groupView, SLOT(slotModifyItem(BCUnit*)));
  connect(m_doc, SIGNAL(signalUnitModified(BCUnit*)),
          m_detailedView, SLOT(slotModifyItem(BCUnit*)));
  connect(m_doc, SIGNAL(signalUnitModified(BCUnit*)),
          m_editWidget, SLOT(slotUpdateCompletions(BCUnit*)));

  // connect the deleted signal to both listviews
//  connect(m_doc, SIGNAL(signalUnitDeleted(BCUnit*)),
//          m_groupView, SLOT(slotRemoveItem(BCUnit*)));
  connect(m_doc, SIGNAL(signalUnitDeleted(BCUnit*)),
          m_detailedView, SLOT(slotRemoveItem(BCUnit*)));

  connect(m_groupView, SIGNAL(signalRenameCollection(int, const QString&)),
          m_doc, SLOT(slotRenameCollection(int, const QString&)));

  connect(m_doc, SIGNAL(signalFractionDone(float)), SLOT(slotUpdateFractionDone(float)));
}

// These are general options.
// The options that can be changed in the "Configuration..." dialog
// are taken care of by the ConfigDialog object.
void Bookcase::saveOptions() {
//  kdDebug() << "Bookcase::saveOptions()" << endl;
  // for some reason, the m_config pointer is getting changed, but
  // I can't figure out where, so just to be on the safe side
  if(m_config != kapp->config()) {
    m_config = kapp->config();
  }
  m_config->setGroup("General Options");
  m_config->writeEntry("Geometry", size());
  m_config->writeEntry("Show Statusbar", m_toggleStatusBar->isChecked());

  toolBar("mainToolBar")->saveSettings(m_config, "MainToolBar");
  toolBar("collectionToolBar")->saveSettings(m_config, "CollectionToolBar");

  m_fileOpenRecent->saveEntries(m_config, "Recent Files");
  if(m_doc->URL().fileName() != i18n("Untitled")) {
    m_config->writeEntry("Last Open File", m_doc->URL().url());
  }

  QPtrListIterator<BCCollection> collIt(m_doc->collectionList());
  for( ; collIt.current(); ++collIt) {
    QValueList<int> widthList;
    for(int i = 0; i < m_detailedView->columns(); ++i) {
      widthList.append(m_detailedView->columnWidth(i));
    }
    m_config->writeEntry("Column Widths - " + collIt.current()->unitName(), widthList);
  }

  m_config->writeEntry("Main Window Splitter Sizes", m_split->sizes());

//  kdDebug() << "Bookcase::saveOptions - done" << endl;
// TODO: fix for multiple collections
  BCCollection* coll = m_doc->collectionById(0);
  if(coll) {
    m_config->setGroup(QString("Options - %1").arg(coll->unitName()));
    QString groupName = coll->unitGroups()[m_unitGrouping->currentItem()];
    m_config->writeEntry("Group By", groupName);
  }
}

void Bookcase::readOptions() {
  // for some reason, the m_config pointer is getting changed, but
  // I can't figure out where, so just to be on the safe side
  if(m_config != kapp->config()) {
    m_config = kapp->config();
  }

  m_config->setGroup("General Options");

  toolBar("mainToolBar")->applySettings(m_config, "MainToolBar");
  m_toggleToolBar->setChecked(!toolBar("mainToolBar")->isHidden());
  slotToggleToolBar();

  toolBar("collectionToolBar")->applySettings(m_config, "CollectionToolBar");
  m_toggleCollectionBar->setChecked(!toolBar("collectionToolBar")->isHidden());
  slotToggleCollectionBar();

  bool bViewStatusBar = m_config->readBoolEntry("Show Statusbar", true);
  m_toggleStatusBar->setChecked(bViewStatusBar);
  slotToggleStatusBar();

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

  bool showCount = m_config->readBoolEntry("Show Group Count", false);
  m_groupView->slotShowCount(showCount);

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

  // TODO:fix this stupid hack, there's no collection ye
//  QPtrListIterator<BCCollection> collIt(m_doc->collectionList());
//  for( ; collIt.current(); ++collIt) {
//    BCCollection* coll = collIt.current();
//    m_config->setGroup(QString("Options - %1").arg(coll->unitName()));
    m_config->setGroup(QString("Options - book"));
//    QString defaultGroup = coll->defaultUnitGroup();
//    QString unitGroup = m_config->readEntry("Group By", defaultGroup);
    QString unitGroup = m_config->readEntry("Group By", QString::null);
    m_groupView->setGroupAttribute(0, unitGroup);
//    kdDebug() << unitGroup << endl;
//  }
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
      KURL url(filename);
      m_doc->openDocument(url);
      setCaption(filename, false);
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
  kdDebug() <<  "Bookcase::openURL" << endl;
 // since a lot of updates will happen with a large file, disable them
  m_detailedView->setUpdatesEnabled(false);
  m_groupView->setUpdatesEnabled(false);
  // disable sorting
  m_detailedView->setSorting(-1);
  m_groupView->setSorting(-1);

  // try to open document
  bool success = m_doc->openDocument(url_);

  // re-enable updates
  m_detailedView->setUpdatesEnabled(true);
  m_groupView->setUpdatesEnabled(true);
  // re-enable sorting
  m_detailedView->setSorting(0, true);
  m_groupView->setSorting(0, true);

  if(success) {
    m_doc->setModified(false);
    setCaption(m_doc->URL().fileName(), false);
    // collapse all the groups (depth=1)
    m_groupView->slotCollapseAll(1);
    // expand the collections
    m_groupView->slotExpandAll(0);
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

  if(m_doc->isEmpty()) {
    slotStatusMsg(i18n("Ready."));
    return;
  }

  QString filename("bookcase-printing.xsl");
  QString xsltfile = KGlobal::dirs()->findResource("appdata", filename);
  if(xsltfile.isNull()) {
    QString str = i18n("Unable to find a file needed for printing - %1.\n"
                       "Please check your installation.").arg(filename);
    KMessageBox::sorry(this, str);
    return;
  } else {
    // third parameter is whether to run the domtree through BCAttribute::format
    QString html = m_doc->exportHTML(xsltfile, "author", false);
    if(html.isEmpty()) {
      QString str = i18n("Error in XSLT processing.\n"
                         "Please check your installation.");
      KMessageBox::sorry(this, str);
      return;
    }
    
//    kdDebug() << html << endl;
    doPrint(html);
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

void Bookcase::slotToggleCollectionBar() {
  slotStatusMsg(i18n("Toggling collection toolbar..."));

  KToolBar* tb = toolBar("collectionToolBar");

  if(m_toggleCollectionBar->isChecked()) {
    tb->show();
  } else {
    tb->hide();
  }

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotShowConfigDialog() {
  if(!m_configDlg) {
    m_configDlg = new ConfigDialog(this);
    m_configDlg->readConfiguration(m_config);
    connect(m_configDlg, SIGNAL(signalConfigChanged()),
             SLOT(slotHandleConfigChange()));
    connect(m_configDlg, SIGNAL(finished()),
             SLOT(slotHideConfigDialog()));
    connect(m_configDlg, SIGNAL(signalShowCount(bool)),
             m_groupView, SLOT(slotShowCount(bool)));
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
  QPtrListIterator<BCCollection> it(m_doc->collectionList());
  for( ; it.current(); ++it) {
    text += i18n("%1 %2").arg(it.current()->unitTitle()).arg(i18n("Total"));
    text += ": " + QString::number(it.current()->unitCount()) + " ";
  }
  statusBar()->changeItem(text, ID_STATUS_COUNT);
}

void Bookcase::slotDeleteUnit(BCUnit* unit_) {
  m_doc->slotDeleteUnit(unit_);
  m_editWidget->slotSetContents(0);
  m_detailedView->slotSetSelected(0);
  m_groupView->slotSetSelected(0);
}

void Bookcase::slotFileNewCollection() {
  kdDebug() << "Bookcase::slotFileNewCollection()" << endl;
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

void Bookcase::updateCollectionToolBar() {
  kdDebug() << "Bookcase::updateCollectionToolBar" << endl;

  BCCollection* coll = m_doc->collectionById(0);
  if(!coll) {
    return;
  }
  
  QString current = m_groupView->groupAttribute();
  if(current.isEmpty()) {
    current = coll->defaultGroupAttribute();
    m_groupView->setGroupAttribute(coll, current);
  }

  QStringList groupTitles;
  int index = 0;
  QStringList groups = coll->unitGroups();
  QStringList::Iterator it = groups.begin();
  for(int i = 0; it != groups.end(); ++it, ++i) {
    QString groupName = static_cast<QString>(*it);
    BCAttribute* att = coll->attributeByName(groupName);
    groupTitles << att->title();
    if(groupName == current) {
      index = i;
    }
  }
// is it more of a performance hit to compare two stringlists then to repopulate needlessly?
//  if(groupTitles != m_unitGrouping->items()) {
    m_unitGrouping->setItems(groupTitles);
//  }
  m_unitGrouping->setCurrentItem(index);
//  kdDebug() << "Bookcase::updateCollectionToolBar - setting index " << index << " for " << groupTitles[index] << endl;
}

void Bookcase::slotChangeGrouping() {
  kdDebug() << "Bookcase::slotChangeGrouping" << endl;
  unsigned idx = m_unitGrouping->currentItem();

  QPtrListIterator<BCCollection> collIt(m_doc->collectionList());
  for( ; collIt.current(); ++collIt) {
    BCCollection* coll = collIt.current();
    QString groupName;
    if(idx < coll->unitGroups().count()) {
      groupName = coll->unitGroups()[idx];
    } else {
      groupName = coll->defaultGroupAttribute();
    }
//    kdDebug() << "\tchange to " << groupName << endl;
    m_groupView->setGroupAttribute(coll, groupName);
  }
}

void Bookcase::slotUpdateCollection(BCCollection* coll_) {
  updateCollectionToolBar();
  slotUnitCount();
  connect(coll_, SIGNAL(signalGroupModified(BCCollection*, BCUnitGroup*)),
          m_groupView, SLOT(slotModifyGroup(BCCollection*, BCUnitGroup*)));
}

void Bookcase::doPrint(const QString& html_) {
  KHTMLPart* w = new KHTMLPart();
  w->begin();
  w->write(html_);
  w->end();
  
// the problem with doing my own layout is that the text gets truncated, both at the
// top and at the bottom. Even adding the overlap parameter, there were problems.
// KHTMLView takes care of that with a truncatedAt() parameter, but that's hidden in
// the khtml::render_root class. So for now, just use the KHTMLView::print() method.
// in KDE 3.1, there's an added option for printing a header with the date, url, and
// page number
#if 1
  w->view()->print();
#else
  KPrinter* printer = new KPrinter(QPrinter::PrinterResolution);

  if(printer->setup(this)) {
    //viewport()->setCursor(waitCursor);
    printer->setFullPage(false);
    printer->setCreator("Bookcase");
    printer->setDocName(m_doc->URL().url());

    QPainter *p = new QPainter;
    p->begin(printer);
    
    // mostly taken from KHTMLView::print()
    QString headerLeft = KGlobal::locale()->formatDate(QDate::currentDate(), false);
    QString headerRight = m_doc->URL().url();
    QString footerMid;
    
    QFont headerFont("helvetica", 8);
    p->setFont(headerFont);
    const int lspace = p->fontMetrics().lineSpacing();
    const int headerHeight = (lspace * 3) / 2;
    
    QPaintDeviceMetrics metrics(printer);
    const int pageHeight = metrics.height() - 2*headerHeight;
    const int pageWidth = metrics.width();
            
//    kdDebug() << "Bookcase::doPrint() - pageHeight = " << pageHeight << ""
//                 "; contentsHeight = " << w->view()->contentsHeight() << endl;

    int top = 0;
    int page = 1;

    bool more = true;
    while(more) {
      p->setPen(Qt::black);
      p->setFont(headerFont);

      footerMid = i18n("Page %1").arg(page);

      p->drawText(0, 0, pageWidth, lspace, Qt::AlignLeft, headerLeft);
      p->drawText(0, 0, pageWidth, lspace, Qt::AlignRight, headerRight);
      p->drawText(0, pageHeight+headerHeight, pageWidth, lspace, Qt::AlignHCenter, footerMid);

      w->paint(p, QRect(0, -top + 2*headerHeight, pageWidth, pageHeight+top), top, &more);

      top += pageHeight - PRINTED_PAGE_OVERLAP;

      if(more) {
        printer->newPage();
        page++;
      }
//      p->resetXForm();
    }
    // stop painting, this will automatically send the print data to the printer
    p->end();
    delete p;
  }

  delete printer;
#endif
  delete w;
}
