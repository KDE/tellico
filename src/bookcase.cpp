/***************************************************************************
                                bookcase.cpp
                             -------------------
    begin                : Wed Aug 29 21:00:54 CEST 2001
    copyright            : (C) 2001, 2002, 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#define ENABLE_Z3950R 0

#include "bookcase.h"
#include "bookcasedoc.h"
#include "bcdetailedlistview.h"
#include "bcuniteditwidget.h"
#include "bcgroupview.h"
#include "bccollection.h"
#include "bcunit.h"
#include "configdialog.h"
#include "bclabelaction.h"
#include "finddialog.h"
#include "bcunititem.h"
#include "bcfilter.h"
#include "bcfilterdialog.h"
#include "bccollectionfieldsdialog.h"
#include "bookcasecontroller.h"
#include "bcimportdialog.h"
#include "bcexportdialog.h"
#include "bcfilehandler.h"
#include "stringmacrodialog.h"
#include "translators/htmlexporter.h" // for printing

#include <kapplication.h>
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
#include <ktip.h>
#include <krecentdocument.h>
#include <kedittoolbar.h>
#include <kkeydialog.h>

#include <qdir.h>
#include <qsplitter.h>
#include <qvbox.h>
#include <qpaintdevicemetrics.h>
#include <qpainter.h>
#include <qheader.h>
#include <qsignalmapper.h>

#if ENABLE_Z3950R
#include "lookupdialog.h"
#include <kprotocolinfo.h>
#endif

static const int ID_STATUS_MSG = 1;
static const int ID_STATUS_COUNT = 2;
//static const int PRINTED_PAGE_OVERLAP = 0;
static const char* ready = I18N_NOOP("Ready.");
//static const QString UIFILE = QString::fromLatin1("/home/robby/projects/bookcase/src/bookcaseui.rc");
static const QString UIFILE;

Bookcase::Bookcase(QWidget* parent_/*=0*/, const char* name_/*=0*/) : KMainWindow(parent_, name_),
    m_doc(0),
    m_config(kapp->config()),
    m_progress(0),
    m_controller(0),
    m_configDlg(0),
    m_findDlg(0),
    m_lookupDlg(0),
    m_filterDlg(0),
    m_collFieldsDlg(0),
    m_currentStep(1),
    m_maxSteps(2) {

  // do main window stuff like setting the icon
  initWindow();
  
  // initialize the status bar and progress bar
  initStatusBar();
  
  // create a document, which also creates an empty book collection
  // must be done before the different widgets are created
  initDocument();

  // set up all the actions, some connect to the document, so this must be
  // after initDocument()
  initActions();
  
  // create the different widgets in the view
  initView();
  initConnections();

  readOptions();

  m_fileSave->setEnabled(false);
  slotEnableOpenedActions(true);
  slotEnableModifiedActions(false);

  slotUnitCount(0);
  m_editWidget->slotSetLayout(m_doc->collection());
}

void Bookcase::initWindow() {
  setIcon(KGlobal::iconLoader()->loadIcon(QString::fromLatin1("bookcase"), KIcon::Desktop));
  m_controller = new BookcaseController(this, "controller");
  BCFileHandler::s_bookcase = this;
}

void Bookcase::initStatusBar() {
  statusBar()->insertItem(i18n(ready), ID_STATUS_MSG);
  statusBar()->setItemAlignment(ID_STATUS_MSG, Qt::AlignLeft | Qt::AlignVCenter);

  statusBar()->insertItem(QString(), ID_STATUS_COUNT, 0, true);
  statusBar()->setItemAlignment(ID_STATUS_COUNT, Qt::AlignLeft | Qt::AlignVCenter);

  m_progress = new KProgress(100, statusBar());
  m_progress->setFixedHeight(statusBar()->minimumSizeHint().height());
  statusBar()->addWidget(m_progress);
  m_progress->hide();
}

void Bookcase::initActions() {
#ifdef KDEBUG
  if(!m_controller || !m_doc) {
    kdWarning() << "Bookcase::initActions() - controller and doc must be instantiated first!" << endl;
  }
#endif

  /*************************************************
   * File->New menu
   *************************************************/
  QSignalMapper* collectionMapper = new QSignalMapper(this);
  connect(collectionMapper, SIGNAL(mapped(int)),
          this, SLOT(slotFileNew(int)));

  KActionMenu* fileNewMenu = new KActionMenu(actionCollection(), "file_new_collection");
  fileNewMenu->setText(i18n("New"));
  QPixmap pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("filenew"), KIcon::Toolbar);
  fileNewMenu->setIconSet(QIconSet(pix));
  fileNewMenu->setToolTip(i18n("Create a new document"));
  fileNewMenu->setDelayed(false);

  KAction* action = new KAction(actionCollection(), "new_book_collection");
  action->setText(i18n("New &Book Collection"));
  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("book"), KIcon::User);
  action->setIconSet(QIconSet(pix));
  action->setToolTip(i18n("Create a new book collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, BCCollection::Book);

  action = new KAction(actionCollection(), "new_bibtex_collection");
  action->setText(i18n("New B&ibliography"));
  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("bibtex"), KIcon::User);
  action->setIconSet(QIconSet(pix));
  action->setToolTip(i18n("Create a new bibtex bibliography"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, BCCollection::Bibtex);

//  action = new KAction(actionCollection(), "new_comic_book_collection");
//  action->setText(i18n("New &Comic Book Collection"));
//  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("comic"), KIcon::User);
//  action->setIconSet(QIconSet(pix));
//  action->setToolTip(i18n("Create a new comic book collection"));
//  fileNewMenu->insert(action);
//  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
//  collectionMapper->setMapping(action, BCCollection::ComicBook);

  action = new KAction(actionCollection(), "new_video_collection");
  action->setText(i18n("New &Video Collection"));
  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("video"), KIcon::User);
  action->setIconSet(QIconSet(pix));
  action->setToolTip(i18n("Create a new video collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, BCCollection::Video);

  action = new KAction(actionCollection(), "new_music_collection");
  action->setText(i18n("New &Music Collection"));
  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("album"), KIcon::User);
  action->setIconSet(QIconSet(pix));
  action->setToolTip(i18n("Create a new music collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, BCCollection::Album);

//  action = new KAction(actionCollection(), "new_card_collection");
//  action->setText(i18n("New C&ard Collection"));
//  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("card"), KIcon::User);
//  action->setIconSet(QIconSet(pix));
//  action->setToolTip(i18n("Create a new trading card collection"));
//  fileNewMenu->insert(action);
//  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
//  collectionMapper->setMapping(action, BCCollection::Card);
//
//  action = new KAction(actionCollection(), "new_coin_collection");
//  action->setText(i18n("New C&oin Collection"));
//  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("coin"), KIcon::User);
//  action->setIconSet(QIconSet(pix));
//  action->setToolTip(i18n("Create a new coin collection"));
//  fileNewMenu->insert(action);
//  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
//  collectionMapper->setMapping(action, BCCollection::Coin);
//
//  action = new KAction(actionCollection(), "new_stamp_collection");
//  action->setText(i18n("New &Stamp Collection"));
//  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("stamp"), KIcon::User);
//  action->setIconSet(QIconSet(pix));
//  action->setToolTip(i18n("Create a new stamp collection"));
//  fileNewMenu->insert(action);
//  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
//  collectionMapper->setMapping(action, BCCollection::Stamp);
//
//  action = new KAction(actionCollection(), "new_wine_collection");
//  action->setText(i18n("New &Wine Collection"));
//  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("wine"), KIcon::User);
//  action->setIconSet(QIconSet(pix));
//  action->setToolTip(i18n("Create a new wine collection"));
//  fileNewMenu->insert(action);
//  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
//  collectionMapper->setMapping(action, BCCollection::Wine);

//  action = new KAction(actionCollection(), "new_custom_collection");
//  action->setText(i18n("New C&ustom Collection"));
//  action->setToolTip(i18n("Create a new collection"));
//  fileNewMenu->insert(action);
//  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
//  collectionMapper->setMapping(action, BCCollection::Base);
//  action->setEnabled(false);

  /*************************************************
   * File menu
   *************************************************/
  action = KStdAction::open(this, SLOT(slotFileOpen()), actionCollection());
  action->setToolTip(i18n("Open an existing document"));
  m_fileOpenRecent = KStdAction::openRecent(this, SLOT(slotFileOpenRecent(const KURL&)), actionCollection());
  m_fileOpenRecent->setToolTip(i18n("Open a recently used file"));
  m_fileSave = KStdAction::save(this, SLOT(slotFileSave()), actionCollection());
  m_fileSave->setToolTip(i18n("Save the document"));
  m_fileSaveAs = KStdAction::saveAs(this, SLOT(slotFileSaveAs()), actionCollection());
  m_fileSaveAs->setToolTip(i18n("Save the document as a different file..."));
  m_filePrint = KStdAction::print(this, SLOT(slotFilePrint()), actionCollection());
  m_filePrint->setToolTip(i18n("Print the contents of the document..."));
  action = KStdAction::quit(this, SLOT(slotFileQuit()), actionCollection());
  action->setToolTip(i18n("Quit the application"));
  
/**************** Import Menu ***************************/

  QSignalMapper* importMapper = new QSignalMapper(this);
  connect(importMapper, SIGNAL(mapped(int)),
          this, SLOT(slotFileImport(int)));

  m_fileImportMenu = new KActionMenu(actionCollection(), "file_import");
  m_fileImportMenu->setText(i18n("Import"));
  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("fileimport"), KIcon::Toolbar);
  m_fileImportMenu->setIconSet(QIconSet(pix));
  m_fileImportMenu->setToolTip(i18n("Import collection data from other formats..."));
  m_fileImportMenu->setDelayed(false);

  action = new KAction(actionCollection(), "file_import_bookcase");
  action->setText(i18n("Import Bookcase Data"));
  action->setToolTip(i18n("Import another Bookcase data file"));
  m_fileImportMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, BCImportDialog::BookcaseXML);

  action = new KAction(actionCollection(), "file_import_csv");
  action->setText(i18n("Import CSV Data"));
  action->setToolTip(i18n("Import a CSV file"));
  m_fileImportMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, BCImportDialog::CSV);

  action = new KAction(actionCollection(), "file_import_bibtex");
  action->setText(i18n("Import Bibtex Data"));
  action->setToolTip(i18n("Import a Bibtex bibliography file"));
  m_fileImportMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, BCImportDialog::Bibtex);

  action = new KAction(actionCollection(), "file_import_bibtexml");
  action->setText(i18n("Import Bibtexml Data"));
  action->setToolTip(i18n("Import a Bibtexml bibliography file"));
  m_fileImportMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, BCImportDialog::Bibtexml);

  action = new KAction(actionCollection(), "file_import_xslt");
  action->setText(i18n("Import XSL Transform"));
  action->setToolTip(i18n("Import using an XSL Transform"));
  m_fileImportMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, BCImportDialog::XSLT);

/**************** Export Menu ***************************/
  
  QSignalMapper* exportMapper = new QSignalMapper(this);
  connect(exportMapper, SIGNAL(mapped(int)),
          this, SLOT(slotFileExport(int)));

  m_fileExportMenu = new KActionMenu(actionCollection(), "file_export");
  m_fileExportMenu->setText(i18n("Export"));
  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("fileexport"), KIcon::Toolbar);
  m_fileExportMenu->setIconSet(QIconSet(pix));
  m_fileExportMenu->setToolTip(i18n("Export the collection data to other formats..."));
  m_fileExportMenu->setDelayed(false);

  action = new KAction(actionCollection(), "file_export_html");
  action->setText(i18n("Export to HTML"));
  action->setToolTip(i18n("Export to an HTML file"));
  m_fileExportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, BCExportDialog::HTML);

  action = new KAction(actionCollection(), "file_export_csv");
  action->setText(i18n("Export to CSV"));
  action->setToolTip(i18n("Export to a comma-separated values file"));
  m_fileExportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, BCExportDialog::CSV);

//  action = new KAction(actionCollection(), "file_export_pilotdb");
//  action->setText(i18n("Export to PilotDB"));
//  action->setToolTip(i18n("Export to a PilotDB database"));
//  m_fileExportMenu->insert(action);
//  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
//  exportMapper->setMapping(action, BCExportDialog::PilotDB);

  m_exportBibtex = new KAction(actionCollection(), "file_export_bibtex");
  m_exportBibtex->setText(i18n("Export to Bibtex"));
  m_exportBibtex->setToolTip(i18n("Export to a bibtex file"));
  m_fileExportMenu->insert(m_exportBibtex);
  connect(m_exportBibtex, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(m_exportBibtex, BCExportDialog::Bibtex);

  m_exportBibtexml = new KAction(actionCollection(), "file_export_bibtexml");
  m_exportBibtexml->setText(i18n("Export to Bibtexml"));
  m_exportBibtexml->setToolTip(i18n("Export to a Bibtexml file"));
  m_fileExportMenu->insert(m_exportBibtexml);
  connect(m_exportBibtexml, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(m_exportBibtexml, BCExportDialog::Bibtexml);

  action = new KAction(actionCollection(), "file_export_xslt");
  action->setText(i18n("Export XSL Transform"));
  action->setToolTip(i18n("Export using an XSL Transform"));
  m_fileExportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, BCExportDialog::XSLT);

#if ENABLE_Z3950R
//  m_lookup = new KAction(i18n("Lookup book..."), 0, this,
//                         SLOT(slotShowLookupDialog()), actionCollection(), "lookup_dialog");
//  m_lookup->setToolTip(i18n("Lookup a book on the internet..."));
//  bool z3950Enabled = KProtocolInfo::isKnownProtocol(QString::fromLatin1("z3950r"));
//  m_lookup->setEnabled(z3950Enabled);
#endif

  /*************************************************
   * Edit menu
   *************************************************/
//  m_editCut = KStdAction::cut(this, SLOT(slotEditCut()), actionCollection());
//  m_editCut->setToolTip(i18n("Cut the selected section and puts it to the clipboard"));
//  m_editCopy = KStdAction::copy(this, SLOT(slotEditCopy()), actionCollection());
//  m_editCopy->setToolTip(i18n("Copy the selected section to the clipboard"));
//  m_editPaste = KStdAction::paste(this, SLOT(slotEditPaste()), actionCollection());
//  m_editPaste->setToolTip(i18n("Paste the clipboard contents to actual position"));
  m_editFind = KStdAction::find(this, SLOT(slotEditFind()), actionCollection());
  m_editFind->setToolTip(i18n("Search the collection"));
  m_editFindNext = KStdAction::findNext(this, SLOT(slotEditFindNext()), actionCollection());
  m_editFind->setToolTip(i18n("Find next match in the collection"));
  // gets enabled once one search is done
  m_editFindNext->setEnabled(false);
  action = new KAction(i18n("Delete"), QString::fromLatin1("editdelete"), CTRL + Key_D,
                       m_controller, SLOT(slotDeleteSelectedUnits()),
                       actionCollection(), "edit_delete");
  action->setToolTip(i18n("Delete the selected entries"));
  action = new KAction(i18n("Rename Collection..."), QString::fromLatin1("editclear"), CTRL + Key_R,
                       m_doc, SLOT(slotRenameCollection()),
                       actionCollection(), "edit_rename_collection");
  action->setToolTip(i18n("Rename the collection"));
  action = new KAction(i18n("Collection Fields..."), QString::fromLatin1("edit"), CTRL + Key_U,
                       this, SLOT(slotShowCollectionFieldsDialog()),
                       actionCollection(), "edit_fields");
  action->setToolTip(i18n("Modify the collection fields"));
  m_editConvertBibtex = new KAction(i18n("Convert to Bibliography"), 0,
                                    m_doc, SLOT(slotConvertToBibtex()),
                                    actionCollection(), "edit_convert_bibtex");
  m_editConvertBibtex->setToolTip(i18n("Convert a book collection to a bibliography"));
  m_editBibtexMacros = new KAction(i18n("String Macros..."), 0,
                                   this, SLOT(slotEditStringMacros()),
                                   actionCollection(), "edit_string_macros");
  m_editBibtexMacros->setToolTip(i18n("Edit the Bibtex string macros"));
                             
  /*************************************************
   * Settings menu
   *************************************************/
#if KDE_VERSION > 309
  setStandardToolBarMenuEnabled(true);
#else
  m_toggleToolBar = KStdAction::showToolbar(this, SLOT(slotToggleToolBar()),
                                            actionCollection());
  m_toggleToolBar->setToolTip(i18n("Enable/disable the toolbar"));
  m_toggleCollectionBar = new KToggleAction(i18n("Show Co&llection ToolBar"), 0, this,
                                            SLOT(slotToggleCollectionBar()),
                                            actionCollection(), "toggle_collection_bar");
  m_toggleCollectionBar->setToolTip(i18n("Enable/disable the collection toolbar"));
#endif
#ifdef KDE_IS_VERSION
#if KDE_IS_VERSION(3,1,90)
  createStandardStatusBarAction();
#else
  m_toggleStatusBar = KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()),
                                                actionCollection());
  m_toggleStatusBar->setToolTip(i18n("Enable/disable the statusbar"));
#endif
#else
  m_toggleStatusBar = KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()),
                                                actionCollection());
  m_toggleStatusBar->setToolTip(i18n("Enable/disable the statusbar"));
#endif
  KStdAction::configureToolbars(this, SLOT(slotConfigToolbar()), actionCollection());
  KStdAction::keyBindings(this, SLOT(slotConfigKeys()), actionCollection());
  m_toggleGroupWidget = new KToggleAction(i18n("Show &Group View"), 0,
                                          this, SLOT(slotToggleGroupWidget()),
                                          actionCollection(), "toggle_group_widget");
  m_toggleGroupWidget->setToolTip(i18n("Enable/disable the group view"));
  m_toggleEditWidget = new KToggleAction(i18n("Show Entry &Editor"), 0,
                                         this, SLOT(slotToggleEditWidget()),
                                         actionCollection(), "toggle_edit_widget");
  m_toggleEditWidget->setToolTip(i18n("Enable/disable the editor"));
  action = new KAction(i18n("Advanced Filter..."), QString::fromLatin1("filter"), CTRL + Key_J,
                       this, SLOT(slotShowFilterDialog()),
                       actionCollection(), "filter_dialog");
  action->setToolTip(i18n("Filter the collection"));
                                         
  KStdAction::preferences(this, SLOT(slotShowConfigDialog()), actionCollection());
                                          
  /*************************************************
   * Help menu
   *************************************************/
#if KDE_VERSION > 309
  action = KStdAction::tipOfDay(this, SLOT(slotShowTipOfDay()),
                                actionCollection(), "tipOfDay");
#else
  action = new KAction(i18n("Tip of the &Day"), QString::fromLatin1("ktip"), 0, this,
                         SLOT(slotShowTipOfDay()),
                         actionCollection(), "tipOfDay");
#endif
  action->setToolTip(i18n("Show the \"Tip of the Day\" dialog..."));
  /*************************************************
   * Collection Toolbar
   *************************************************/
  action = new BCLabelAction(i18n("Group By:"), 0,
                             actionCollection(), "change_unit_grouping_label");
  action->setToolTip(i18n("Change the grouping of the collection"));
  m_unitGrouping = new KSelectAction(i18n("Group Selection"), 0, this,
                                     SLOT(slotChangeGrouping()),
                                     actionCollection(), "change_unit_grouping");
  m_unitGrouping->setToolTip(i18n("Change the grouping of the collection"));
                                     
  action = new BCLabelAction(i18n("Quick Filter:"), 0,
                             actionCollection(), "quick_filter_label");
  action->setToolTip(i18n("Filter the collection"));
  m_quickFilter = new BCLineEditAction(i18n("Filter Entry"), 0,
                                       actionCollection(), "quick_filter");
  // want to update every time the filter text changes
  connect(m_quickFilter, SIGNAL(textChanged(const QString&)),
          this, SLOT(slotUpdateFilter(const QString&)));

  if(!UIFILE.isEmpty()) {
    kdWarning() << "Bookcase::initActions() - change createGUI() call!" << endl;
    createGUI(UIFILE);
  } else {
    createGUI();
  }
}

void Bookcase::initDocument() {
  m_doc = new BookcaseDoc(this);

  // allow status messages from the document
  connect(m_doc, SIGNAL(signalStatusMsg(const QString&)),
          SLOT(slotStatusMsg(const QString&)));

  // do stuff that changes when the doc is modified
  connect(m_doc, SIGNAL(signalModified()),
          SLOT(slotEnableModifiedActions()));

  connect(m_doc, SIGNAL(signalCollectionAdded(BCCollection*)),
          m_controller, SLOT(slotCollectionAdded(BCCollection*)));
  connect(m_doc, SIGNAL(signalCollectionDeleted(BCCollection*)),
          m_controller, SLOT(slotCollectionDeleted(BCCollection*)));
  connect(m_doc, SIGNAL(signalCollectionRenamed(const QString&)),
          m_controller, SLOT(slotCollectionRenamed(const QString&)));
}

void Bookcase::initView() {
  m_split = new QSplitter(Qt::Horizontal, this);
  setCentralWidget(m_split);

  // m_split is the parent widget for the left side column view
  m_groupView = new BCGroupView(m_split, "groupview");

  // stack the detailed view and edit widget vertically
  QVBox* vbox = new QVBox(m_split);

  m_detailedView = new BCDetailedListView(vbox, "detailedlistview");

  m_editWidget = new BCUnitEditWidget(vbox, "editwidget");

  m_controller->setWidgets(m_groupView, m_detailedView, m_editWidget);

  // since the new doc is already initialized with one empty book collection
  m_controller->slotCollectionAdded(m_doc->collection());

  // this gets called in readOptions(), so not neccessary here
  //updateCollectionToolBar();
}

void Bookcase::initConnections() {
  connect(m_groupView, SIGNAL(signalUnitSelected(QWidget*, const BCUnitList&)),
          m_controller, SLOT(slotUpdateSelection(QWidget*, const BCUnitList&)));
  connect(m_detailedView, SIGNAL(signalUnitSelected(QWidget*, const BCUnitList&)),
          m_controller, SLOT(slotUpdateSelection(QWidget*, const BCUnitList&)));
  connect(m_editWidget, SIGNAL(signalClearSelection(QWidget*)),
          m_controller, SLOT(slotUpdateSelection(QWidget*)));

  // since the detailed view can take a while to populate,
  // it gets a progress monitor, too
  connect(m_detailedView, SIGNAL(signalFractionDone(float)),
          SLOT(slotUpdateFractionDone(float)));

  connect(m_doc, SIGNAL(signalUnitSelected(BCUnit*, const QString&)),
          m_controller, SLOT(slotUpdateSelection(BCUnit*, const QString&)));
          
  connect(m_editWidget, SIGNAL(signalSaveUnit(BCUnit*)),
          m_doc, SLOT(slotSaveUnit(BCUnit*)));
  connect(m_groupView, SIGNAL(signalDeleteUnit(BCUnit*)),
          m_doc, SLOT(slotDeleteUnit(BCUnit*)));
  connect(m_detailedView, SIGNAL(signalDeleteUnit(BCUnit*)),
          m_doc, SLOT(slotDeleteUnit(BCUnit*)));
  connect(m_editWidget, SIGNAL(signalDeleteUnit(BCUnit*)),
          m_doc, SLOT(slotDeleteUnit(BCUnit*)));

  connect(m_doc, SIGNAL(signalUnitAdded(BCUnit*)),
          m_controller, SLOT(slotUnitAdded(BCUnit*)));
  connect(m_doc, SIGNAL(signalUnitModified(BCUnit*)),
          m_controller, SLOT(slotUnitModified(BCUnit*)));
  connect(m_doc, SIGNAL(signalUnitDeleted(BCUnit*)),
          m_controller, SLOT(slotUnitDeleted(BCUnit*)));
}

void Bookcase::slotInitFileOpen() {
  // check to see if most recent file should be opened
  m_config->setGroup("General Options");
  if(m_config->readBoolEntry("Reopen Last File", true)
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
  
  saveMainWindowSettings(m_config, QString::fromLatin1("Main Window Options"));
  
  m_config->setGroup("General Options");
//  m_config->writeEntry("Geometry", size());
#ifdef KDE_MAKE_VERSION
#if KDE_VERSION < KDE_MAKE_VERSION(3,1,90)
  m_config->writeEntry("Show Statusbar", m_toggleStatusBar->isChecked());
#endif
#else
  m_config->writeEntry("Show Statusbar", m_toggleStatusBar->isChecked());
#endif
  m_config->writeEntry("Show Group Widget", m_toggleGroupWidget->isChecked());
  m_config->writeEntry("Show Edit Widget", m_toggleEditWidget->isChecked());

#if KDE_VERSION < 306
  toolBar("mainToolBar")->saveSettings(m_config, QString::fromLatin1("MainToolBar"));
  toolBar("collectionToolBar")->saveSettings(m_config, QString::fromLatin1("CollectionToolBar"));
#endif
  m_fileOpenRecent->saveEntries(m_config, QString::fromLatin1("Recent Files"));
  if(m_doc->URL().fileName() != i18n("Untitled")) {
    m_config->writeEntry("Last Open File", m_doc->URL().url());
  } else {
    // decided I didn't want the entry over-written when exited with an eempty document
//    m_config->writeEntry("Last Open File", QString::null);
  }

  m_config->writeEntry("Main Window Splitter Sizes", m_split->sizes());

  saveCollectionOptions(m_doc->collection());
}

void Bookcase::readCollectionOptions(BCCollection* coll_) {
  m_config->setGroup(QString::fromLatin1("Options - %1").arg(coll_->unitName()));

  QString defaultGroup = coll_->defaultGroupAttribute();
  QString unitGroup = m_config->readEntry("Group By", defaultGroup);
  if(!coll_->unitGroups().contains(unitGroup)) {
    unitGroup = defaultGroup;
  }
  m_groupView->setGroupAttribute(coll_, unitGroup);

  // make sure the right combo element is selected
  slotUpdateCollectionToolBar(coll_);
}

void Bookcase::saveCollectionOptions(BCCollection* coll_) {
//  kdDebug() << "Bookcase::saveCollectionOptions() = " coll_->unitName() << endl;
  if(!coll_ || coll_->unitList().isEmpty()) {
    return;
  }

  m_config->setGroup(QString::fromLatin1("Options - %1").arg(coll_->unitName()));
  QString groupName = coll_->unitGroups()[m_unitGrouping->currentItem()];
  m_config->writeEntry("Group By", groupName);

  m_detailedView->saveConfig(coll_);
}

void Bookcase::readOptions() {
//  kdDebug() << "Bookcase::readOptions()" << endl;
  // for some reason, the m_config pointer is getting changed, but
  // I can't figure out where, so just to be on the safe side
  if(m_config != kapp->config()) {
    kdDebug() << "Bookcase::readOptions() - inconsistent KConfig pointers!" << endl;
    m_config = kapp->config();
  }

  applyMainWindowSettings(m_config, QString::fromLatin1("Main Window Options"));

  m_config->setGroup("General Options");

#if KDE_VERSION < 306
  toolBar("mainToolBar")->applySettings(m_config, QString::fromLatin1("MainToolBar"));
  m_toggleToolBar->setChecked(!toolBar("mainToolBar")->isHidden());
  slotToggleToolBar();

  toolBar("collectionToolBar")->applySettings(m_config, QString::fromLatin1("CollectionToolBar"));
  m_toggleCollectionBar->setChecked(!toolBar("collectionToolBar")->isHidden());
  slotToggleCollectionBar();
#endif

#ifdef KDE_MAKE_VERSION
#if KDE_VERSION < KDE_MAKE_VERSION(3,1,90)
  bool bViewStatusBar = m_config->readBoolEntry("Show Statusbar", true);
  m_toggleStatusBar->setChecked(bViewStatusBar);
  slotToggleStatusBar();
#endif
#else
  bool bViewStatusBar = m_config->readBoolEntry("Show Statusbar", true);
  m_toggleStatusBar->setChecked(bViewStatusBar);
  slotToggleStatusBar();
#endif

  bool bViewGroupWidget = m_config->readBoolEntry("Show Group Widget", true);
  m_toggleGroupWidget->setChecked(bViewGroupWidget);
  slotToggleGroupWidget();

  bool bViewEditWidget = m_config->readBoolEntry("Show Edit Widget", true);
  m_toggleEditWidget->setChecked(bViewEditWidget);
  slotToggleEditWidget();

  // initialize the recent file list
  m_fileOpenRecent->loadEntries(m_config, QString::fromLatin1("Recent Files"));

  QValueList<int> splitList = m_config->readIntListEntry("Main Window Splitter Sizes");
  if(!splitList.empty()) {
    m_split->setSizes(splitList);
  }

  bool autoCapitals = m_config->readBoolEntry("Auto Capitalization", true);
  BCAttribute::setAutoCapitalize(autoCapitals);

  bool autoFormat = m_config->readBoolEntry("Auto Format", true);
  BCAttribute::setAutoFormat(autoFormat);

  bool showCount = m_config->readBoolEntry("Show Group Count", true);
  m_groupView->showCount(showCount);

  QStringList articles;
  if(m_config->hasKey("Articles")) {
    articles = m_config->readListEntry("Articles", ',');
  } else {
    articles = BCAttribute::defaultArticleList();
  }
  BCAttribute::setArticleList(articles);

  QStringList suffixes;
  if(m_config->hasKey("Name Suffixes")) {
    suffixes = m_config->readListEntry("Name Suffixes", ',');
  } else {
    suffixes = BCAttribute::defaultSuffixList();
  }
  BCAttribute::setSuffixList(suffixes);

  QStringList prefixes;
  if(m_config->hasKey("Surname Prefixes")) {
    prefixes = m_config->readListEntry("Surname Prefixes", ',');
  } else {
    prefixes = BCAttribute::defaultSurnamePrefixList();
  }
  BCAttribute::setSurnamePrefixList(prefixes);

  BCCollection* coll = m_doc->collection();
  m_config->setGroup(QString::fromLatin1("Options - %1").arg(coll->unitName()));

  QString defaultGroup = coll->defaultGroupAttribute();
  QString unitGroup = m_config->readEntry("Group By", defaultGroup);
  if(!coll->unitGroups().contains(unitGroup)) {
    unitGroup = defaultGroup;
  }
  m_groupView->setGroupAttribute(coll, unitGroup);

  // make sure the right combo element is selected
  slotUpdateCollectionToolBar(coll);
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
      m_doc->slotSetModified(true);
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
  QListViewItem* item = m_detailedView->currentItem();
  if(!item) {
    item = m_detailedView->firstChild();
  }
  return dynamic_cast<BCUnitItem*>(item);
}

void Bookcase::slotFileNew(int type_) {
  slotStatusMsg(i18n("Creating new document..."));

  if(m_editWidget->queryModified() && m_doc->saveModified()) {
    m_doc->newDocument(type_);
    slotEnableOpenedActions(true);
    slotEnableModifiedActions(false);
  }

  slotStatusMsg(i18n(ready));
}

void Bookcase::slotFileOpen() {
  slotStatusMsg(i18n("Opening file..."));

  if(m_editWidget->queryModified() && m_doc->saveModified()) {
    QString filter = i18n("*.bc|Bookcase files (*.bc)");
    filter += QString::fromLatin1("\n");
    filter += i18n("*.xml|XML files (*.xml)");
    filter += QString::fromLatin1("\n");
    filter += i18n("*|All files");
    // keyword 'open'
    KURL url = KFileDialog::getOpenURL(QString::fromLatin1(":open"), filter,
                                       this, i18n("Open File..."));
    if(!url.isEmpty()) {
      slotFileOpen(url);
    }
  }
  slotStatusMsg(i18n(ready));
}

void Bookcase::slotFileOpen(const KURL& url_) {
  slotStatusMsg(i18n("Opening file..."));
  
  if(m_editWidget->queryModified() && m_doc->saveModified()) {
    bool success = openURL(url_);
    if(success) {
      m_fileOpenRecent->addURL(url_);
    }// else {
     // m_fileOpenRecent->removeURL(url_);
    //}
  }

  slotStatusMsg(i18n(ready));
}

void Bookcase::slotFileOpenRecent(const KURL& url_) {
  slotStatusMsg(i18n("Opening file..."));

  if(m_editWidget->queryModified() && m_doc->saveModified()) {
    bool success = openURL(url_);
    if(!success) {
      m_fileOpenRecent->removeURL(url_);
    }
  }

  slotStatusMsg(i18n(ready));
}

bool Bookcase::openURL(const KURL& url_) {
//  kdDebug() <<  "Bookcase::openURL() - " << url_.url() << endl;

  // try to open document
  kapp->setOverrideCursor(Qt::waitCursor);
  bool success = m_doc->openDocument(url_);
  kapp->restoreOverrideCursor();

  if(success) {
    slotUpdateFilter(QString::null);
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

  slotStatusMsg(i18n(ready));
}

void Bookcase::slotFileSaveAs() {
  if(!m_editWidget->queryModified()) {
    return;
  }
  
//  if(m_doc->isEmpty()) {
//    return;
//  }

  slotStatusMsg(i18n("Saving file with a new filename..."));

  QString filter = i18n("*.bc|Bookcase files (*.bc)");
  filter += QString::fromLatin1("\n");
  filter += i18n("*.xml|XML files (*.xml)");
  filter += QString::fromLatin1("\n");
  filter += i18n("*|All files");
  
  // keyword 'open'
  KFileDialog dlg(QString::fromLatin1(":open"), filter, this, "filedialog", true);
  dlg.setCaption(i18n("Save As"));
  dlg.setOperationMode(KFileDialog::Saving);

  int result = dlg.exec();
  if(result == QDialog::Rejected) {
    slotStatusMsg(i18n(ready));
    return;   
  }
  
  KURL url = dlg.selectedURL();

  if(url.fileName().contains('.') == 0) {
    QString cf = dlg.currentFilter();
    if(cf.length() > 1 && cf[1] == '.') {
      url.setFileName(url.fileName() + cf.mid(1));
    }
  }
  
  if(!url.isEmpty() && url.isValid()) {
    KRecentDocument::add(url);
    m_doc->saveDocument(url);
    m_fileOpenRecent->addURL(url);
    setCaption(m_doc->URL().fileName(), false);
  }

  slotStatusMsg(i18n(ready));
}

void Bookcase::slotFilePrint() {
  slotStatusMsg(i18n("Printing..."));

  kapp->setOverrideCursor(Qt::waitCursor);

  m_config->setGroup(QString::fromLatin1("Printing"));
  bool printGrouped = m_config->readBoolEntry("Print Grouped", true);
  bool printHeaders = m_config->readBoolEntry("Print Field Headers", false);

  // If the collection is being filtered, warn the user
  if(m_detailedView->filter() != 0) {
    QString str = i18n("The collection is currently being filtered to show a limited subset of "
                       "the entries. Only the visible entries will be printed. Continue?");
    int ret = KMessageBox::warningContinueCancel(this, str, QString::null, i18n("Print"),
                                                 QString::fromLatin1("WarnPrintVisible"));
    if(ret == KMessageBox::Cancel) {
      slotStatusMsg(i18n(ready));
      kapp->restoreOverrideCursor();
      return;
    } 
  }

  HTMLExporter exporter(m_doc->collection(), m_detailedView->visibleUnits());
  exporter.setXSLTFile(QString::fromLatin1("bookcase-printing.xsl"));
  exporter.setPrintHeaders(printHeaders);
  exporter.setPrintGrouped(printGrouped);
  exporter.setGroupBy(groupBy());
  exporter.setSortTitles(sortTitles());
  exporter.setColumns(visibleColumns());
  
  slotStatusMsg(i18n("Processing document..."));
  bool printFormatted = m_config->readBoolEntry("Print Formatted", true);
  QString html = exporter.text(printFormatted, true);
  if(html.isEmpty()) {
    XSLTError();
    kapp->restoreOverrideCursor();
    slotStatusMsg(i18n(ready));
    return;
  }

  kapp->restoreOverrideCursor();
  
//  kdDebug() << html << endl;
  slotStatusMsg(i18n("Printing..."));
  doPrint(html);

  slotStatusMsg(i18n(ready));
}

void Bookcase::slotFileQuit() {
  slotStatusMsg(i18n("Exiting..."));

  // this gets called in queryExit() anyway
  //saveOptions();
  close();

  slotStatusMsg(i18n(ready));
}

void Bookcase::slotEditCut() {
  slotStatusMsg(i18n("Cutting selection..."));

  slotStatusMsg(i18n(ready));
}

void Bookcase::slotEditCopy() {
  slotStatusMsg(i18n("Copying selection to clipboard..."));

  slotStatusMsg(i18n(ready));
}

void Bookcase::slotEditPaste() {
  slotStatusMsg(i18n("Inserting clipboard contents..."));

  slotStatusMsg(i18n(ready));
}

void Bookcase::slotEditFind() {
  if(!m_findDlg) {
    m_findDlg = new FindDialog(this);
    m_editFindNext->setEnabled(true);
  } else {
    m_findDlg->updateAttributeList();
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

void Bookcase::slotConfigToolbar() {
  saveMainWindowSettings(m_config, QString::fromLatin1("Main Window Options"));
  KEditToolbar dlg(actionCollection(), UIFILE);
  connect(&dlg, SIGNAL(newToolbarConfig()), this, SLOT(slotNewToolbarConfig()));
  if(dlg.exec()) {
    createGUI(UIFILE);
  }
}

void Bookcase::slotNewToolbarConfig() {
  applyMainWindowSettings(m_config, QString::fromLatin1("Main Window Options"));
}

void Bookcase::slotConfigKeys() {
  KKeyDialog::configure(actionCollection());
}

void Bookcase::slotToggleToolBar() {
#if KDE_VERSION < 306
  slotStatusMsg(i18n("Toggling toolbar..."));

  if(m_toggleToolBar->isChecked()) {
    toolBar("mainToolBar")->show();
  } else {
    toolBar("mainToolBar")->hide();
  }

  slotStatusMsg(i18n(ready));
#endif
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

  slotStatusMsg(i18n(ready));
#endif
}

void Bookcase::slotToggleStatusBar() {
 slotStatusMsg(i18n("Toggle the statusbar..."));

  if(m_toggleStatusBar->isChecked()) {
    statusBar()->show();
  } else {
    statusBar()->hide();
  }

  slotStatusMsg(i18n(ready));
}

void Bookcase::slotToggleGroupWidget() {
  if(m_toggleGroupWidget->isChecked()) {
    m_groupView->show();
  } else {
    m_groupView->hide();
  }
}

void Bookcase::slotToggleEditWidget() {
  if(m_toggleEditWidget->isChecked()) {
    m_editWidget->show();
  } else {
    m_editWidget->hide();
  }
}

void Bookcase::slotShowConfigDialog() {
  if(!m_configDlg) {
    m_configDlg = new ConfigDialog(this);
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

void Bookcase::slotShowTipOfDay(bool force_/*=true*/) {
  QString tipfile = KGlobal::dirs()->findResource("data", QString::fromLatin1("bookcase/bookcase.tips"));
  KTipDialog::showTip(this, tipfile, force_);
}

void Bookcase::slotStatusMsg(const QString& text_) {
  statusBar()->clear();
  // add a space at the beginning and end for asthetic reasons
  statusBar()->changeItem(QString::fromLatin1(" ")+text_+QString::fromLatin1(" "), ID_STATUS_MSG);
  kapp->processEvents();
}

void Bookcase::slotUnitCount(int count_) {
  // I add a space before and after for asthetic reasons
//  QString text = QString::fromLatin1(" ");
//  BCCollectionListIterator it(m_doc->collectionList());
//  for( ; it.current(); ++it) {
//    text += i18n("%1 Total").arg(it.current()->unitTitle());
//    text += QString::fromLatin1(": ");
//    text += QString::number(it.current()->unitCount());
//  }
//  text += QString::fromLatin1(" ");

  BCCollection* coll = m_doc->collection();
  if(!coll) {
    return;
  }

  int count = coll->unitCount();
  QString text = QString::fromLatin1(" Total Entries: %1 ").arg(count);
  // if more than one book is selected, add the number of selected books
  if(count_ > 1) {
    text += i18n("(%1 selected)").arg(count_);
    text += QString::fromLatin1(" ");
  }
    
  statusBar()->changeItem(text, ID_STATUS_COUNT);
}

void Bookcase::slotEnableOpenedActions(bool opened_ /*= true*/) {
  // collapse all the groups (depth=1)
  m_groupView->slotCollapseAll(1);
  // expand the collections
  m_groupView->slotExpandAll(0);

  m_fileSaveAs->setEnabled(opened_);
  m_filePrint->setEnabled(opened_);
  m_fileExportMenu->setEnabled(opened_);
  m_editFind->setEnabled(opened_);

  BCCollection::CollectionType type = m_doc->collection()->collectionType();
  m_exportBibtex->setEnabled(type == BCCollection::Bibtex);
  m_exportBibtexml->setEnabled(type == BCCollection::Bibtex);
  m_editConvertBibtex->setEnabled(type == BCCollection::Book);
  m_editBibtexMacros->setEnabled(type == BCCollection::Bibtex);
}

void Bookcase::slotEnableModifiedActions(bool modified_ /*= true*/) {
  if(m_fileSave->isEnabled() != modified_) {
    setCaption(m_doc->URL().fileName(), modified_);
    m_fileSave->setEnabled(modified_);
  }
  // when a collection is replaced, slotEnableOpenedActions() doesn't get called automatically
  if(m_doc->collection()) {
    BCCollection::CollectionType type = m_doc->collection()->collectionType();
    m_exportBibtex->setEnabled(type == BCCollection::Bibtex);
    m_exportBibtexml->setEnabled(type == BCCollection::Bibtex);
    m_editConvertBibtex->setEnabled(type == BCCollection::Book);
    m_editBibtexMacros->setEnabled(type == BCCollection::Bibtex);
  }
}

void Bookcase::slotUpdateFractionDone(float f_) {
//  kdDebug() << "Bookcase::slotUpdateFractionDone() - " << f_ << endl;
  // first check bounds
  f_ = (f_ < 0.0) ? 0.0 : f_;
  f_ = (f_ > 1.0) ? 1.0 : f_;

  if(m_currentStep == m_maxSteps && f_ == 1.0) {
    m_progress->hide();
    return;
  }

  if(!m_progress->isVisible()) {
    m_progress->show();
  }
//  float progress = (m_currentStep - 1 + f_)/static_cast<float>(m_maxSteps) * 100.0;
//  kdDebug() << "Bookcase::slotUpdateFractionDone() - " << progress << endl;
  m_progress->setValue(static_cast<int>((m_currentStep -1 + f_)/static_cast<float>(m_maxSteps) * 100.0));
  kapp->processEvents();
}

void Bookcase::slotHandleConfigChange() {
  // for some reason, the m_config pointer is getting changed, but
  // I can't figure out where, so just to be on the safe side
  if(m_config != kapp->config()) {
    m_config = kapp->config();
  }

  bool showCount = m_configDlg->configValue(QString::fromLatin1("showCount"));
  m_groupView->showCount(showCount);
  
  m_configDlg->saveConfiguration(m_config);
}

void Bookcase::slotUpdateCollectionToolBar(BCCollection* coll_/*=0*/) {
//  kdDebug() << "Bookcase::updateCollectionToolBar()" << endl;

  //TODO fix this later
  if(!coll_){
    coll_ = m_doc->collection();
  }
  if(!coll_) {
    return;
  }
  
  QString current = m_groupView->groupBy();
  if(current.isEmpty() || !coll_->unitGroups().contains(current)) {
    current = coll_->defaultGroupAttribute();
    m_groupView->setGroupAttribute(coll_, current);
  }

  QStringList groupTitles;
  int index = 0;
  QStringList groups = coll_->unitGroups();
  QStringList::ConstIterator groupIt = groups.begin();
  for(int i = 0; groupIt != groups.end(); ++groupIt, ++i) {
    // special case for people "pseudo-group"
    if(*groupIt == QString::fromLatin1("_people")) {
      groupTitles << (QString::fromLatin1("<") + i18n("People") + QString::fromLatin1(">"));
    } else {
      groupTitles << coll_->attributeTitleByName(*groupIt);
    }
    if(*groupIt == current) {
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

  BCCollection* coll = m_doc->collection();
  QString groupName;
  if(idx < coll->unitGroups().count()) {
    groupName = coll->unitGroups()[idx];
  } else {
    groupName = coll->defaultGroupAttribute();
  }
//  kdDebug() << "\tchange to " << groupName << endl;
  m_groupView->setGroupAttribute(coll, groupName);
}

void Bookcase::doPrint(const QString& html_) {
  KHTMLPart* w = new KHTMLPart();
  w->setJScriptEnabled(false);
  w->setJavaEnabled(false);
  w->setMetaRefreshEnabled(false);
  w->setPluginsEnabled(false);
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

  if(printer->setup(this, i18n("Print %1").arg(m_doc->URL().prettyURL()))) {
    //viewport()->setCursor(waitCursor);
    printer->setFullPage(false);
    printer->setCreator(QString::fromLatin1("Bookcase"));
    printer->setDocName(m_doc->URL().url());

    QPainter *p = new QPainter;
    p->begin(printer);
    
    // mostly taken from KHTMLView::print()
    QString headerLeft = KGlobal::locale()->formatDate(QDate::currentDate(), false);
    QString headerRight = m_doc->URL().url();
    QString footerMid;
    
    QFont headerFont(QString::fromLatin1("helvetica"), 8);
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

      p->drawText(0,                       0, pageWidth, lspace, Qt::AlignLeft,    headerLeft);
      p->drawText(0,                       0, pageWidth, lspace, Qt::AlignRight,   headerRight);
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

void Bookcase::XSLTError() {
  QString str = i18n("Bookcase encountered an error in XSLT processing.\n");
  str += i18n("Please check your installation.");
  KMessageBox::sorry(this, str);
}

void Bookcase::FileError(const QString& filename) {
  QString str = i18n("Bookcase is unable to find a required file - %1.\n").arg(filename);
  str += i18n("Please check your installation.");
  KMessageBox::sorry(this, str);
}

void Bookcase::slotShowLookupDialog() {
#if ENABLE_Z3950R
  if(!m_lookupDlg) {
    m_lookupDlg = new LookupDialog(this);
    connect(m_lookupDlg, SIGNAL(finished()),
            SLOT(slotHideLookupDialog()));
  } else {
    KWin::setActiveWindow(m_lookupDlg->winId());
  }
  m_lookupDlg->show();
#endif
}

void Bookcase::slotHideLookupDialog() {
#if ENABLE_Z3950R
  if(m_lookupDlg) {
    m_lookupDlg->delayedDestruct();
    m_lookupDlg = 0;
  }
#endif
}

void Bookcase::slotShowFilterDialog() {
  if(!m_filterDlg) {
    m_filterDlg = new BCFilterDialog(m_detailedView, this);
    m_filterDlg->setFilter(m_detailedView->filter());
//    connect(m_lookupDlg, SIGNAL(signalConfigChanged()),
//            SLOT(slotHandleConfigChange()));
    m_quickFilter->blockSignals(true);
    connect(m_filterDlg, SIGNAL(filterApplied()),
            m_quickFilter, SLOT(clear()));
    connect(m_filterDlg, SIGNAL(finished()),
            SLOT(slotHideFilterDialog()));
  } else {
    KWin::setActiveWindow(m_filterDlg->winId());
  }
  m_filterDlg->show();
}

void Bookcase::slotHideFilterDialog() {
  m_quickFilter->blockSignals(false);
  if(m_filterDlg) {
    m_filterDlg->delayedDestruct();
    m_filterDlg = 0;
  }
}

void Bookcase::slotUpdateFilter(const QString& text_) {
  if(m_detailedView) {
    QString text = text_.stripWhiteSpace();
    if(text.isEmpty()) {
      // passing a null pointer just shows everything
      m_detailedView->setFilter(static_cast<BCFilter*>(0));
      if(m_filterDlg) {
        m_filterDlg->slotClear();
      }
    } else {
      BCFilter* filter = new BCFilter(BCFilter::MatchAny);
      // if the text contains any non-Word characters, assume it's a regexp
      QRegExp rx(QString::fromLatin1("\\W"));
      BCFilterRule* rule;
      if(text.find(rx) < 0) {
        // an empty attribute string means check every attribute
        rule = new BCFilterRule(QString::null, text, BCFilterRule::FuncContains);
      } else {
        // if it isn't valid, hold off on applying the filter
        QRegExp tx(text);
        if(!tx.isValid()) {
          return;
        }
        rule = new BCFilterRule(QString::null, text, BCFilterRule::FuncRegExp);
      }
      filter->append(rule);
      m_detailedView->setFilter(filter);
      // since filter dialog isn't modal
      if(m_filterDlg) {
        m_filterDlg->setFilter(filter);
      }
    }
  }
}

void Bookcase::slotShowCollectionFieldsDialog() {
  if(!m_collFieldsDlg) {
    BCCollection* coll = m_doc->collection();
    m_collFieldsDlg = new BCCollectionFieldsDialog(coll, this);
    connect(m_collFieldsDlg, SIGNAL(finished()),
            SLOT(slotHideCollectionFieldsDialog()));
    connect(m_collFieldsDlg, SIGNAL(signalCollectionModified()),
            m_doc, SLOT(slotSetModified()));
  } else {
    KWin::setActiveWindow(m_collFieldsDlg->winId());
  }
  m_collFieldsDlg->show();
}

void Bookcase::slotHideCollectionFieldsDialog() {
  if(m_collFieldsDlg) {
    m_collFieldsDlg->delayedDestruct();
    m_collFieldsDlg = 0;
  }
}

void Bookcase::slotFileImport(int format_) {
  slotStatusMsg(i18n("Importing data..."));
  slotUpdateFilter(QString::null);

  BCImportDialog::ImportFormat format = static_cast<BCImportDialog::ImportFormat>(format_);
  KURL url = KFileDialog::getOpenURL(QString::fromLatin1(":import"), BCImportDialog::fileFilter(format),
                                     this, i18n("Import File..."));
  if(url.isEmpty() || !url.isValid()) {
    slotStatusMsg(i18n(ready));
    return;
  }

  BCImportDialog dlg(format, url, this, "importdlg");

  if(dlg.exec() != QDialog::Accepted) {
    slotStatusMsg(i18n(ready));
    return;
  }
    
  if(m_editWidget->queryModified() && (!dlg.replaceCollection() || m_doc->saveModified())) {
    kapp->setOverrideCursor(Qt::waitCursor);
    BCCollection* coll = dlg.collection();
    if(!coll) {
      kapp->restoreOverrideCursor();
      KMessageBox::sorry(this, dlg.statusMessage());
      slotStatusMsg(i18n(ready));
      return;
    }

    if(!dlg.replaceCollection()) {
      if(m_doc->collection()->collectionType() != coll->collectionType()) {
        // The progress bar is hidden by the BookcaseController when a new collection is added
        // that doesn't happen here, so need to clear it
        m_currentStep = m_maxSteps;
        slotUpdateFractionDone(1.0);
        m_currentStep = 1;
        kapp->restoreOverrideCursor();
        KMessageBox::sorry(this, i18n("Only collections with the same type of entries as the current "
                                      "one can be appended. No changes are being made to the current "
                                      "collection"));
        slotStatusMsg(i18n(ready));
        return;
      }
      m_doc->appendCollection(coll);
      delete coll;
      slotEnableModifiedActions(true);
    } else {
      m_doc->replaceCollection(coll);
      m_fileOpenRecent->setCurrentItem(-1);
      slotEnableOpenedActions(true);
      slotEnableModifiedActions(false);
    }
    kapp->restoreOverrideCursor();
  }

  slotStatusMsg(i18n(ready));
}

void Bookcase::slotFileExport(int format_) {
  slotStatusMsg(i18n("Exporting data..."));
  
  BCExportDialog dlg(static_cast<BCExportDialog::ExportFormat>(format_),
                     m_doc->collection(), this, "exportdialog");

  if(dlg.exec() == QDialog::Accepted) {
    KFileDialog fileDlg(QString::fromLatin1(":export"), dlg.fileFilter(), this, "filedialog", true);
    fileDlg.setCaption(i18n("Export As"));
    fileDlg.setOperationMode(KFileDialog::Saving);

    int result = fileDlg.exec();
    if(result == QDialog::Rejected) {
      slotStatusMsg(i18n(ready));
      return;
    }

    KURL url = fileDlg.selectedURL();

    if(url.fileName().contains('.') == 0) {
      QString cf = fileDlg.currentFilter();
      if(cf.length() > 1 && cf[1] == '.') {
        url.setFileName(url.fileName() + cf.mid(1));
      }
    }

    if(!url.isEmpty()) {
      kapp->setOverrideCursor(Qt::waitCursor);
      QString text = dlg.text();
      if(!text.isEmpty()) {
        BCFileHandler::writeURL(url, text, !dlg.encodeUTF8());
//        kdDebug() << text << endl;
      }
      kapp->restoreOverrideCursor();
    }
  }

  slotStatusMsg(i18n(ready));
}

void Bookcase::slotEditStringMacros() {
  slotStatusMsg(i18n("Editing string macros..."));

  StringMacroDialog* dlg = new StringMacroDialog(m_doc->collection(), this);
  dlg->show();
  // this might not be true, but assume for now
  m_doc->slotSetModified(true);

  slotStatusMsg(i18n(ready));
}

QStringList Bookcase::groupBy() const {
  QStringList g = m_groupView->groupBy();
  // special case for pseudo-group
  if(g[0] == QString::fromLatin1("_people")) {
    g.clear();
    BCAttributeListIterator it(m_doc->collection()->peopleAttributeList());
    for( ; it.current(); ++it) {
      g << it.current()->name();
    }
  }
  return g;
}

QStringList Bookcase::sortTitles() const {
  QStringList list;
  list << m_detailedView->sortColumnTitle1();
  list << m_detailedView->sortColumnTitle2();
  list << m_detailedView->sortColumnTitle3();
  return list;
}

QStringList Bookcase::visibleColumns() const {
  return m_detailedView->visibleColumns();
}
