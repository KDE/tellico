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
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
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
#include "xslthandler.h"
#include "finddialog.h"
#include "bcunititem.h"
#include "bookcollection.h"

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

static const int ID_STATUS_MSG = 1;
static const int ID_STATUS_COUNT = 2;
//static const int PRINTED_PAGE_OVERLAP = 10;

Bookcase::Bookcase(QWidget* parent_/*=0*/, const char* name_/*=0*/)
    : KMainWindow(parent_, name_), m_doc(0), m_config(kapp->config()),
      m_progress(0), m_configDlg(0), m_findDlg(0) {

  // do main window stuff like setting the icon
  initWindow();
  
  // initialize the status bar and progress bar
  initStatusBar();
  
  // set up all the actions
  initActions();
  
  // create a document, which also creates an empty book collection
  // must be done before the different widgets are created
  initDocument();
  
  // create the different widgets in the view
  initView();
  initConnections();

  readOptions();

  m_fileSave->setEnabled(false);
  slotEnableOpenedActions(false);
  slotEnableModifiedActions(false);
  
  // not yet implemented
  m_editCut->setEnabled(false);
  m_editCopy->setEnabled(false);
  m_editPaste->setEnabled(false);

  slotUnitCount();
}

void Bookcase::initWindow() {
  setIcon(KGlobal::iconLoader()->loadIcon(QString::fromLatin1("bookcase"), KIcon::Desktop));
}

void Bookcase::initStatusBar() {
  statusBar()->insertItem(i18n("Ready."), ID_STATUS_MSG);
  statusBar()->setItemAlignment(ID_STATUS_MSG, Qt::AlignLeft | Qt::AlignVCenter);

  statusBar()->insertItem(QString(), ID_STATUS_COUNT, 0, true);
  statusBar()->setItemAlignment(ID_STATUS_COUNT, Qt::AlignLeft | Qt::AlignVCenter);

  m_progress = new KProgress(100, statusBar());
  m_progress->setFixedHeight(statusBar()->minimumSizeHint().height());
  statusBar()->addWidget(m_progress);
  m_progress->hide();
}

void Bookcase::initActions() {
#if KDE_VERSION > 305
  setStandardToolBarMenuEnabled(true);
#endif
 
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
#if KDE_VERSION < 306
  m_toggleToolBar = KStdAction::showToolbar(this, SLOT(slotToggleToolBar()),
                                            actionCollection());
#endif
  m_toggleStatusBar = KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()),
                                                actionCollection());
  m_preferences = KStdAction::preferences(this, SLOT(slotShowConfigDialog()),
                                          actionCollection());

  m_importBibtex = new KAction(i18n("Import Bibtex File..."), 0, this,
                                    SLOT(slotImportBibtex()),
                                    actionCollection(),
                                    "import_bibtex");
  m_importBibtexml = new KAction(i18n("Import Bibtexml File..."), 0, this,
                                    SLOT(slotImportBibtexml()),
                                    actionCollection(),
                                    "import_bibtexml");

  m_exportBibtex = new KAction(i18n("Export to Bibtex File..."), 0, this,
                                    SLOT(slotExportBibtex()),
                                    actionCollection(),
                                    "export_bibtex");
  m_exportBibtexml = new KAction(i18n("Export to Bibtexml File..."), 0, this,
                                 SLOT(slotExportBibtexml()),
                                 actionCollection(),
                                 "export_bibtexml");
  m_exportXSLT = new KAction(i18n("Export Using XSL Transform..."), 0, this,
                             SLOT(slotExportXSLT()),
                             actionCollection(),
                             "export_xslt");

//  m_fileNewCollection = new KAction(i18n("New &Collection"), 0, this,
//                                    SLOT(slotFileNewCollection()),
//                                    actionCollection(),
//                                    "file_new_collection");

#if KDE_VERSION < 306
  m_toggleCollectionBar = new KToggleAction(i18n("Show Co&llection ToolBar"), 0, this,
                                            SLOT(slotToggleCollectionBar()),
                                            actionCollection(),
                                            "toggle_collection_bar");
#endif
  (void) new BCLabelAction(i18n("Group by: "), 0,
                           actionCollection(),
                           "change_unit_grouping_label");
  m_unitGrouping = new KSelectAction(i18n("Group Books By"), 0, this,
                                     SLOT(slotChangeGrouping()),
                                     actionCollection(),
                                     "change_unit_grouping");

  m_fileNew->setToolTip(i18n("Create a new document"));
  m_fileOpen->setToolTip(i18n("Open an existing document"));
  m_fileOpenRecent->setToolTip(i18n("Open a recently used file"));
  m_fileSave->setToolTip(i18n("Save the actual document"));
  m_fileSaveAs->setToolTip(i18n("Save the actual document as..."));
  m_filePrint->setToolTip(i18n("Print the contents of the document..."));
  m_fileQuit->setToolTip(i18n("Quit the application"));
  m_editCut->setToolTip(i18n("Cut the selected section and puts it to the clipboard"));
  m_editCopy->setToolTip(i18n("Copy the selected section to the clipboard"));
  m_editPaste->setToolTip(i18n("Paste the clipboard contents to actual position"));
  m_editFind->setToolTip(i18n("Search in the document..."));
#if KDE_VERSION < 306
  m_toggleToolBar->setToolTip(i18n("Enable/disable the toolbar"));
#endif
  m_toggleStatusBar->setToolTip(i18n("Enable/disable the statusbar"));
  m_preferences->setToolTip(i18n("Configure the options for the application..."));
  m_unitGrouping->setToolTip(i18n("Change the grouping of the collection"));
#if KDE_VERSION < 306
  m_toggleCollectionBar->setToolTip(i18n("Enable/disable the collection toolbar"));
#endif

  m_importBibtexml->setToolTip(i18n("Import a Bibtexml file..."));
  m_exportBibtex->setToolTip(i18n("Export to Bibtex file..."));
  m_exportBibtexml->setToolTip(i18n("Export to Bibtexml file..."));
  m_exportXSLT->setToolTip(i18n("Export a file using an XSL transform..."));

//  createGUI(QString::fromLatin1("/home/robby/projects/bookcase/src/bookcaseui.rc"));
  createGUI();

  // gets enabled once one search is done
  m_editFindNext->setEnabled(false);
}

void Bookcase::initDocument() {
  m_doc = new BookcaseDoc(this);

  // allow status messages from the document
  connect(m_doc, SIGNAL(signalStatusMsg(const QString&)),
           SLOT(slotStatusMsg(const QString&)));

  // do stuff that changes when the doc is modified
  connect(m_doc, SIGNAL(signalModified()),
          SLOT(slotEnableModifiedActions()));

  // overkill since a modified signal does not always mean a change in unit quantity
  connect(m_doc, SIGNAL(signalModified()),
          SLOT(slotUnitCount()));

  // update the progress bar
  connect(m_doc, SIGNAL(signalFractionDone(float)),
          SLOT(slotUpdateFractionDone(float)));
          
  // update the toolbar, unit count, and so on
  connect(m_doc, SIGNAL(signalCollectionAdded(BCCollection*)),
          SLOT(slotUpdateCollection(BCCollection*)));
}

void Bookcase::initView() {
  m_split = new QSplitter(this);
  setCentralWidget(m_split);

  // m_split is the parent widget for the left side column view
  m_groupView = new BCGroupView(m_split);

  // stack the detailed view and edit widget vertically
  QVBox* vbox = new QVBox(m_split);

  m_detailedView = new BCDetailedListView(m_doc, vbox);

  m_editWidget = new BCUnitEditWidget(vbox);

  // since the new doc is already initialized with one empty book collection
  BCCollection* coll = m_doc->collectionById(0);
  m_groupView->slotAddCollection(coll);
  m_detailedView->slotAddCollection(coll);
  m_editWidget->slotSetLayout(coll);

  // need to connect update signal
  BCCollectionListIterator it(m_doc->collectionList());
  for( ; it.current(); ++it) {
    connect(it.current(), SIGNAL(signalGroupModified(BCCollection*, BCUnitGroup*)),
            m_groupView, SLOT(slotModifyGroup(BCCollection*, BCUnitGroup*)));
  }

  // this gets called in readOptions(), so not neccessary here
  //updateCollectionToolBar();
}

void Bookcase::initConnections() {
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

  connect(m_editWidget, SIGNAL(signalSaveUnit(BCUnit*)),
          m_doc, SLOT(slotSaveUnit(BCUnit*)));

  connect(m_groupView, SIGNAL(signalDeleteUnit(BCUnit*)),
          SLOT(slotDeleteUnit(BCUnit*)));
  connect(m_detailedView, SIGNAL(signalDeleteUnit(BCUnit*)),
          SLOT(slotDeleteUnit(BCUnit*)));
  connect(m_editWidget, SIGNAL(signalDeleteUnit(BCUnit*)),
          SLOT(slotDeleteUnit(BCUnit*)));

  connect(m_doc, SIGNAL(signalCollectionAdded(BCCollection*)),
          m_groupView, SLOT(slotAddCollection(BCCollection*)));
  connect(m_doc, SIGNAL(signalCollectionAdded(BCCollection*)),
          m_editWidget, SLOT(slotSetCollection(BCCollection*)));
  connect(m_doc, SIGNAL(signalCollectionAdded(BCCollection*)),
          m_detailedView, SLOT(slotAddCollection(BCCollection*)));

  connect(m_doc, SIGNAL(signalCollectionDeleted(BCCollection*)),
          this, SLOT(saveCollectionOptions(BCCollection*)));
  connect(m_doc, SIGNAL(signalCollectionDeleted(BCCollection*)),
          m_groupView, SLOT(slotRemoveItem(BCCollection*)));
  connect(m_doc, SIGNAL(signalCollectionDeleted(BCCollection*)),
          m_detailedView, SLOT(slotRemoveItem(BCCollection*)));

  // connect the added signal to both listviews
// the group view receives BCCollection::signalGroupModified() so no add or modify action needed
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
          
  // if the doc wants a unit selected, show it in both detailed and group views
  connect(m_doc, SIGNAL(signalUnitSelected(BCUnit*)),
          m_detailedView, SLOT(slotSetSelected(BCUnit*)));
  connect(m_doc, SIGNAL(signalUnitSelected(BCUnit*)),
          m_groupView, SLOT(slotSetSelected(BCUnit*)));
  connect(m_doc, SIGNAL(signalUnitSelected(BCUnit*)),
          m_editWidget, SLOT(slotSetContents(BCUnit*)));

  connect(m_groupView, SIGNAL(signalRenameCollection(int, const QString&)),
          m_doc, SLOT(slotRenameCollection(int, const QString&)));
}

void Bookcase::slotInitFileOpen() {
  // check to see if most recent file should be opened
  m_config->setGroup("General Options");
  if(m_config->readBoolEntry("Reopen Last File", false)
      && !m_config->readEntry("Last Open File").isEmpty()) {
    KURL lastFile = KURL(m_config->readEntry("Last Open File"));
    slotFileOpen(lastFile);
  }
}

// These are general options.
// The options that can be changed in the "Configuration..." dialog
// are taken care of by the ConfigDialog object.
void Bookcase::saveOptions() {
//  kdDebug() << "Bookcase::saveOptions()" << endl;
  // for some reason, the m_config pointer is getting changed, but
  // I can't figure out where, so just to be on the safe side
  if(m_config != kapp->config()) {
    kdDebug() << "Bookcase::saveOptions() - inconsistent KConfig pointers!" << endl;
    m_config = kapp->config();
  }
  m_config->setGroup("General Options");
  m_config->writeEntry("Geometry", size());
  m_config->writeEntry("Show Statusbar", m_toggleStatusBar->isChecked());

#if KDE_VERSION < 306
  toolBar("mainToolBar")->saveSettings(m_config, QString::fromLatin1("MainToolBar"));
  toolBar("collectionToolBar")->saveSettings(m_config, QString::fromLatin1("CollectionToolBar"));
#endif
  m_fileOpenRecent->saveEntries(m_config, QString::fromLatin1("Recent Files"));
  if(m_doc->URL().fileName() != i18n("Untitled")) {
    m_config->writeEntry("Last Open File", m_doc->URL().url());
  }

  m_config->writeEntry("Main Window Splitter Sizes", m_split->sizes());

  BCCollectionListIterator it(m_doc->collectionList());
  for( ; it.current(); ++it) {
    saveCollectionOptions(it.current());
  }
}

void Bookcase::saveCollectionOptions(BCCollection* coll_) {
//  kdDebug() << "Bookcase::saveCollectionOptions()" << endl;
  if(!coll_) {
    return;
  }

  m_config->setGroup(QString::fromLatin1("Options - %1").arg(coll_->unitName()));
  QString groupName = coll_->unitGroups()[m_unitGrouping->currentItem()];
  m_config->writeEntry("Group By", groupName);
    
  m_detailedView->saveLayout(m_config, QString::fromLatin1("Options - %1").arg(coll_->unitName()));
  m_config->writeEntry("ColumnNames", m_detailedView->columnNames());
}

void Bookcase::readOptions() {
//  kdDebug() << "Bookcase::readOptions()" << endl;
  // for some reason, the m_config pointer is getting changed, but
  // I can't figure out where, so just to be on the safe side
  if(m_config != kapp->config()) {
    kdDebug() << "Bookcase::readOptions() - inconsistent KConfig pointers!" << endl;
    m_config = kapp->config();
  }

  m_config->setGroup("General Options");

#if KDE_VERSION < 306
  toolBar("mainToolBar")->applySettings(m_config, QString::fromLatin1("MainToolBar"));
  m_toggleToolBar->setChecked(!toolBar("mainToolBar")->isHidden());
  slotToggleToolBar();

  toolBar("collectionToolBar")->applySettings(m_config, QString::fromLatin1("CollectionToolBar"));
  m_toggleCollectionBar->setChecked(!toolBar("collectionToolBar")->isHidden());
  slotToggleCollectionBar();
#endif

  bool bViewStatusBar = m_config->readBoolEntry("Show Statusbar", true);
  m_toggleStatusBar->setChecked(bViewStatusBar);
  slotToggleStatusBar();

  // initialize the recent file list
  m_fileOpenRecent->loadEntries(m_config, QString::fromLatin1("Recent Files"));

  QSize size = m_config->readSizeEntry("Geometry");
  if(!size.isEmpty()) {
    resize(size);
  }

  QValueList<int> splitList = m_config->readIntListEntry("Main Window Splitter Sizes");
  if(!splitList.empty()) {
    m_split->setSizes(splitList);
  }

  bool autoCapitals = m_config->readBoolEntry("Auto Capitalization", true);
  BCAttribute::setAutoCapitalize(autoCapitals);

  bool autoFormat = m_config->readBoolEntry("Auto Format", true);
  BCAttribute::setAutoFormat(autoFormat);

  bool showCount = m_config->readBoolEntry("Show Group Count", false);
  m_groupView->showCount(showCount);

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

  // TODO: fix iteration
  BCCollectionListIterator collIt(m_doc->collectionList());
  for( ; collIt.current(); ++collIt) {
    BCCollection* coll = collIt.current();
    m_config->setGroup(QString::fromLatin1("Options - %1").arg(coll->unitName()));

    QString defaultGroup = coll->defaultGroupAttribute();
    QString unitGroup = m_config->readEntry("Group By", defaultGroup);
    m_groupView->setGroupAttribute(coll, unitGroup);

    // make sure the right combo element is selected
    updateCollectionToolBar();

    QStringList colNames = m_config->readListEntry("ColumnNames");
    if(colNames.isEmpty()) {
      colNames = BookCollection::defaultViewAttributes();
    }
    m_detailedView->setColumns(coll, colNames);

    m_detailedView->restoreLayout(m_config, QString::fromLatin1("Options - %1").arg(coll->unitName()));
  }
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
  QString filename = cfg_->readEntry(QString::fromLatin1("filename"));
  bool modified = cfg_->readBoolEntry(QString::fromLatin1("modified"), false);
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
  return m_editWidget->queryModified() && m_doc->saveModified();
}

bool Bookcase::queryExit() {
//  kdDebug() << "Bookcase::queryExit()" << endl;
  saveOptions();
  return true;
}

BookcaseDoc* Bookcase::doc() {
  return m_doc;
}

BCUnitItem* Bookcase::selectedOrFirstItem() {
  QListViewItem* item = m_detailedView->selectedItem();
  if(!item) {
    item = m_detailedView->firstChild();
  }
  return static_cast<BCUnitItem*>(item);
}

void Bookcase::slotFileNew() {
  slotStatusMsg(i18n("Creating new document..."));

  if(m_editWidget->queryModified() && m_doc->saveModified()) {
    m_doc->newDocument();
    slotEnableOpenedActions(false);
    slotEnableModifiedActions(false);
  }

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotFileOpen() {
  slotStatusMsg(i18n("Opening file..."));

  if(m_editWidget->queryModified() && m_doc->saveModified()) {
    QString filter = i18n("*.bc|Bookcase files (*.bc)");
    filter += QString::fromLatin1("\n");
    filter += i18n("*.xml|XML files (*.xml)");
    filter += QString::fromLatin1("\n");
    filter += i18n("*|All files");
    KURL url = KFileDialog::getOpenURL(QString::null, filter,
                                       this, i18n("Open File..."));
    if(!url.isEmpty()) {
      slotFileOpen(url);
    }
  }
  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotFileOpen(const KURL& url_) {
  slotStatusMsg(i18n("Opening file..."));
  
  if(m_editWidget->queryModified() && m_doc->saveModified()) {
    bool success = openURL(url_);
    if(success) {
      m_fileOpenRecent->addURL(url_);
    }
  }

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotFileOpenRecent(const KURL& url_) {
  slotStatusMsg(i18n("Opening file..."));

  if(m_editWidget->queryModified() && m_doc->saveModified()) {
    bool success = openURL(url_);
    if(!success) {
      m_fileOpenRecent->removeURL(url_);
    }
  }

  slotStatusMsg(i18n("Ready."));
}

bool Bookcase::openURL(const KURL& url_) {
  if(!m_editWidget->queryModified()) {
    return false;
  }
//  kdDebug() <<  "Bookcase::openURL() - " << url_.url() << endl;
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
    slotEnableOpenedActions(true);
    slotEnableModifiedActions(false);
  }

  return success;
}

void Bookcase::slotFileSave() {
  if(!m_editWidget->queryModified()) {
    return;
  }
  slotStatusMsg(i18n("Saving file..."));

  if(m_doc->URL().fileName() == i18n("Untitled")) {
    slotFileSaveAs();
  } else {
    m_doc->saveDocument(m_doc->URL());
  }
  m_fileSave->setEnabled(false);
  setCaption(m_doc->URL().fileName(), false);

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotFileSaveAs() {
  if(!m_editWidget->queryModified()) {
    return;
  }
  
  if(m_doc->isEmpty()) {
    return;
  }

  slotStatusMsg(i18n("Saving file with a new filename..."));

  QString filter = i18n("*.bc|Bookcase files (*.bc)");
  filter += QString::fromLatin1("\n");
  filter += i18n("*.xml|XML files (*.xml)");
  filter += QString::fromLatin1("\n");
  filter += i18n("*|All files");
  KURL url = KFileDialog::getSaveURL(QString::null, filter,
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

  QString filename(QString::fromLatin1("bookcase-printing.xsl"));
  QString xsltfile = KGlobal::dirs()->findResource("appdata", filename);
  if(xsltfile.isNull()) {
    FileError(filename);
    return;
  }

  XSLTHandler handler(xsltfile);

  m_config->setGroup("Printing");
  bool printGrouped = m_config->readBoolEntry("Print Grouped", true);
  QString sortby;
  if(printGrouped) {
   sortby = m_config->readEntry("Print Grouped Attribute", QString::fromLatin1("author"));
   // make sure to add "bc" namespace
   handler.addStringParam("sort-name", QString(QString::fromLatin1("bc:")+sortby).utf8());
  } else {
   // this is needed since the styelsheet has a default value
   handler.addStringParam("sort-name", "");
  }
  
  handler.addStringParam("doc-url", m_doc->URL().fileName().utf8());

  bool printHeaders = m_config->readBoolEntry("Print Field Headers", false);
  if(printHeaders) {
    handler.addParam("show-headers", "true()");
  } else {
    handler.addParam("show-headers", "false()");
  }

  QStringList printFields = m_config->readListEntry("Print Fields - book");
  if(printFields.isEmpty()) {
    //TODO fix me for other types
    printFields = BookCollection::defaultPrintAttributes();
  }
  handler.addStringParam("column-names", printFields.join(QString::fromLatin1(" ")).utf8());
  
  bool printFormatted = m_config->readBoolEntry("Print Formatted", true);

  //TODO these two functions should really be rolled into one
  QDomDocument dom;
  if(printGrouped) {
    // first parameter is what attribute to group by
    // second parameter is whether to run the dom through BCAttribute::format
    dom = m_doc->exportXML(sortby, printFormatted);
  } else {
    dom = m_doc->exportXML(printFormatted);
  }
  
//  kdDebug() << dom.toString() << endl;
  
  slotStatusMsg(i18n("Processing document..."));
  QString html = handler.applyStylesheet(dom.toString());
  if(html.isEmpty()) {
    XSLTError();
    slotStatusMsg(i18n("Ready."));
    return;
  }

//  kdDebug() << html << endl;
  slotStatusMsg(i18n("Printing..."));
  doPrint(html);

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
  if(!m_findDlg) {
    m_findDlg = new FindDialog(this);
    m_editFindNext->setEnabled(true);
  }
  m_findDlg->show();
}

void Bookcase::slotEditFindNext() {
  if(m_doc->isEmpty()) {
    return;
  }
  
  if(m_findDlg) {
    m_findDlg->slotFindNext();
  }
}

void Bookcase::slotToggleToolBar() {
#if KDE_VERION < 306
  slotStatusMsg(i18n("Toggling toolbar..."));

  if(m_toggleToolBar->isChecked()) {
    toolBar("mainToolBar")->show();
  } else {
    toolBar("mainToolBar")->hide();
  }

  slotStatusMsg(i18n("Ready."));
#endif
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
#if KDE_VERSION < 306
  slotStatusMsg(i18n("Toggling collection toolbar..."));

  KToolBar* tb = toolBar("collectionToolBar");

  if(m_toggleCollectionBar->isChecked()) {
    tb->show();
  } else {
    tb->hide();
  }

  slotStatusMsg(i18n("Ready."));
#endif
}

void Bookcase::slotShowConfigDialog() {
  if(!m_configDlg) {
    m_configDlg = new ConfigDialog(m_doc, this);
    m_configDlg->readConfiguration(m_config);
    connect(m_configDlg, SIGNAL(signalConfigChanged()),
            SLOT(slotHandleConfigChange()));
    connect(m_configDlg, SIGNAL(finished()),
            SLOT(slotHideConfigDialog()));
  } else {
    KWin::setActiveWindow(m_configDlg->winId());
  }
  m_configDlg->show();
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
  statusBar()->changeItem(QString::fromLatin1(" ")+text_+QString::fromLatin1(" "), ID_STATUS_MSG);
  kapp->processEvents();
}

void Bookcase::slotUnitCount() {
  // I add a space before and after for asthetic reasons
  QString text = QString::fromLatin1(" ");
  BCCollectionListIterator it(m_doc->collectionList());
  for( ; it.current(); ++it) {
    text += i18n("%1 Total").arg(it.current()->unitTitle());
    text += QString::fromLatin1(": ");
    text += QString::number(it.current()->unitCount());
  }
  text += QString::fromLatin1(" ");
  statusBar()->changeItem(text, ID_STATUS_COUNT);
}

void Bookcase::slotDeleteUnit(BCUnit* unit_) {
  m_doc->slotDeleteUnit(unit_);
  m_editWidget->slotSetContents(0);
  m_detailedView->slotSetSelected(0);
  m_groupView->slotSetSelected(0);
}

//void Bookcase::slotFileNewCollection() {
//  kdDebug() << "Bookcase::slotFileNewCollection()" << endl;
//}

void Bookcase::slotEnableOpenedActions(bool opened_ /*= true*/) {
  // collapse all the groups (depth=1)
  m_groupView->slotCollapseAll(1);
  // expand the collections
  m_groupView->slotExpandAll(0);
  m_fileSaveAs->setEnabled(opened_);
  m_filePrint->setEnabled(opened_);
  m_exportBibtex->setEnabled(opened_);
  m_exportBibtexml->setEnabled(opened_);
  m_exportXSLT->setEnabled(opened_);
  m_editFind->setEnabled(opened_);
}

void Bookcase::slotEnableModifiedActions(bool modified_ /*= true*/) {
  setCaption(m_doc->URL().fileName(), modified_);
  m_fileSave->setEnabled(modified_);
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

  bool showCount = m_configDlg->configValue(QString::fromLatin1("showCount"));
  m_groupView->showCount(showCount, m_doc->collectionList());
  
  m_configDlg->saveConfiguration(m_config);
}

void Bookcase::updateCollectionToolBar() {
//  kdDebug() << "Bookcase::updateCollectionToolBar()" << endl;

  //TODO fix this later
  BCCollection* coll = m_doc->collectionById(0);
  if(!coll) {
    return;
  }
  
  QString current = m_groupView->collGroupBy(coll->unitName());
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
//  kdDebug() << "Bookcase::slotChangeGrouping()" << endl;
  unsigned idx = m_unitGrouping->currentItem();

  BCCollectionListIterator collIt(m_doc->collectionList());
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
//  kdDebug() << "Bookcase::slotUpdateCollection()" << endl;
  
  updateCollectionToolBar();
  slotUnitCount();
  connect(coll_, SIGNAL(signalGroupModified(BCCollection*, BCUnitGroup*)),
          m_groupView, SLOT(slotModifyGroup(BCCollection*, BCUnitGroup*)));
}

void Bookcase::doPrint(const QString& html_) {
  KHTMLPart* w = new KHTMLPart();
  w->begin(m_doc->URL());
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

inline
void Bookcase::XSLTError() {
  QString str = i18n("Bookcase encountered an error in XSLT processing.\n");
  str += i18n("Please check your installation.");
  KMessageBox::sorry(this, str);
}

inline
void Bookcase::FileError(const QString& filename) {
  QString str = i18n("Bookcase is unable to find a required file - %1.\n").arg(filename);
  str += i18n("Please check your installation.");
  KMessageBox::sorry(this, str);
}

void Bookcase::slotExportBibtex() {
  slotStatusMsg(i18n("Exporting..."));

  if(m_doc->isEmpty()) {
    slotStatusMsg(i18n("Ready."));
    return;
  }

  QString filename(QString::fromLatin1("bookcase2bibtex.xsl"));
  
  QString filter = i18n("*.bib|Bibtex files (*.bib)");
  filter += QString::fromLatin1("\n");
  filter += i18n("*|All files");

  exportUsingXSLT(filename, filter);

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotExportBibtexml() {
  slotStatusMsg(i18n("Exporting..."));

  if(m_doc->isEmpty()) {
    slotStatusMsg(i18n("Ready."));
    return;
  }

  QString filename(QString::fromLatin1("bookcase2bibtexml.xsl"));
  
  QString filter = i18n("*.xml|Bibtexml files (*.xml)");
  filter += QString::fromLatin1("\n");
  filter += i18n("*|All files");

  exportUsingXSLT(filename, filter);

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotImportBibtex() {
  slotStatusMsg(i18n("Importing from Bibtex..."));

  QString filter = i18n("*.bib|Bibtex files (*.bib)");
  filter += QString::fromLatin1("\n");
  filter += i18n("*|All files");
  KURL infile = KFileDialog::getOpenURL(QString::null, filter,
                                        this, i18n("Import from Bibtex..."));
  if(infile.isEmpty()) {
    return;
  }

  QDomDocument* dom = BookcaseDoc::importBibtex(infile);

  KURL url;
  url.setFileName(i18n("Untitled"));
  bool success = m_doc->loadDomDocument(url, *dom);
  delete dom;

  if(success) {
    slotEnableOpenedActions(true);
    slotEnableModifiedActions(true);
  }

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotImportBibtexml() {
  slotStatusMsg(i18n("Importing from Bibtexml..."));

  QString filename(QString::fromLatin1("bibtexml2bookcase.xsl"));
  QString xsltfile = KGlobal::dirs()->findResource("appdata", filename);
  if(xsltfile.isEmpty()) {
    FileError(filename);
    return;
  }

  QString filter = i18n("*.xml|Bibtexml files (*.xml)");
  filter += QString::fromLatin1("\n");
  filter += i18n("*|All files");
  KURL infile = KFileDialog::getOpenURL(QString::null, filter,
                                        this, i18n("Import from Bibtexml..."));

  if(infile.isEmpty()) {
    return;
  }
  
  XSLTHandler handler(xsltfile);
  
  QDomDocument* inputDom = m_doc->readDocument(infile);
  QString text = handler.applyStylesheet(inputDom->toString());
  delete inputDom;
  if(text.isEmpty()) {
    XSLTError();
    return;
  }

  QDomDocument dom;
  if(!dom.setContent(text)) {
    XSLTError();
  }
  
  KURL url;
  url.setFileName(i18n("Untitled"));
  bool success = (m_editWidget->queryModified()
                  && m_doc->saveModified()
                  && m_doc->loadDomDocument(url, dom));

  if(success) {
    slotEnableOpenedActions(true);
    slotEnableModifiedActions(true);
  }

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotExportXSLT() {
  slotStatusMsg(i18n("Exporting..."));

  if(m_doc->isEmpty()) {
    slotStatusMsg(i18n("Ready."));
    return;
  }

  QString filter = i18n("*.xsl|XSLT files (*.xsl)");
  filter += QString::fromLatin1("\n");
  filter += i18n("*|All files");
  KURL xsltfile = KFileDialog::getOpenURL(QString::null, filter,
                                          this, i18n("Select XSLT file..."));
  if(xsltfile.isEmpty()) {
    slotStatusMsg(i18n("Ready."));
    return;
  }
  
  XSLTHandler handler(xsltfile);
  handler.addStringParam(QCString("version"), QCString(VERSION));

  QDomDocument dom = m_doc->exportXML();
  QString text = handler.applyStylesheet(dom.toString());
  if(text.isEmpty()) {
    XSLTError();
    return;
  }

  KURL url = KFileDialog::getSaveURL(QString::null,
                                     i18n("*|All files"),
                                     this, i18n("Export..."));
  if(!url.isEmpty()) {
    m_doc->writeURL(url, text);
  }

  slotStatusMsg(i18n("Ready."));
}

bool Bookcase::exportUsingXSLT(const QString& xsltFileName_, const QString& filter_) {
  QString xsltfile = KGlobal::dirs()->findResource("appdata", xsltFileName_);
  if(xsltfile.isEmpty()) {
    FileError(xsltFileName_);
    return false;
  }

  XSLTHandler handler(xsltfile);
  handler.addStringParam(QCString("version"), QCString(VERSION));

  QDomDocument dom = m_doc->exportXML();
  QString text = handler.applyStylesheet(dom.toString());
  if(text.isEmpty()) {
    XSLTError();
    return false;
  }

  KURL url = KFileDialog::getSaveURL(QString::null, filter_,
                                     this, i18n("Export..."));
  if(url.isEmpty()) {
    return false;
  }

  return m_doc->writeURL(url, text);
}
