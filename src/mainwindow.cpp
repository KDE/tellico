/***************************************************************************
    copyright            : (C) 2001-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "mainwindow.h"
#include "document.h"
#include "detailedlistview.h"
#include "entryeditdialog.h"
#include "groupview.h"
#include "collection.h"
#include "entry.h"
#include "configdialog.h"
#include "labelaction.h"
#include "finddialog.h"
#include "entryitem.h"
#include "filter.h"
#include "filterdialog.h"
#include "collectionfieldsdialog.h"
#include "controller.h"
#include "importdialog.h"
#include "exportdialog.h"
#include "filehandler.h" // needed so static mainWindow variable can be set
#include "stringmapdialog.h"
#include "translators/htmlexporter.h" // for printing
#include "entryview.h"
#include "utils.h" // needed for macro definitions
#include "imagefactory.h" // needed so tmp files can get cleaned
#include "collections/bibtexcollection.h" // needed for bibtex string macro dialog

#include <kapplication.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <kprogress.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstdaction.h>
#include <kdebug.h>
#include <kwin.h>
#include <kprogress.h>
#include <kstatusbar.h>
#include <kprinter.h>
#include <khtmlview.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <ktip.h>
#include <krecentdocument.h>
#include <kedittoolbar.h>
#include <kkeydialog.h>
#include <kio/netaccess.h>

#include <qsplitter.h>
//#include <qpainter.h>
#include <qsignalmapper.h>
#include <qmetaobject.h> // needed for copy, cut, paste slots

//#define UIFILE QString::fromLatin1("/home/robby/projects/bookcase/src/bookcaseui.rc")

using Bookcase::MainWindow;

static const int ID_STATUS_MSG = 1;
static const int ID_STATUS_COUNT = 2;
//static const int PRINTED_PAGE_OVERLAP = 0;
static const char* ready = I18N_NOOP("Ready.");

MainWindow::MainWindow(QWidget* parent_/*=0*/, const char* name_/*=0*/) : KMainWindow(parent_, name_),
    m_doc(0),
    m_config(kapp->config()),
    m_progress(0),
    m_controller(0),
    m_configDlg(0),
    m_findDlg(0),
    m_filterDlg(0),
    m_collFieldsDlg(0),
    m_stringMacroDlg(0),
    m_currentStep(1),
    m_maxSteps(2),
    m_newDocument(true) {

  // do main window stuff like setting the icon
  initWindow();
  
  // initialize the status bar and progress bar
  initStatusBar();

  // create a document, which also creates an empty book collection
  // must be done before the different widgets are created
  initDocument();

  // set up all the actions, some connect to the document, so this must be after initDocument()
  initActions();

  // create the different widgets in the view, some widgets connect to actions, so must be after initActions()
  initView();

  // The edit dialog is not created until after the main window is initialized, so it can be a child.
  // So don't make any connections, don't read options for it until slotInitFileOpen

  readOptions();
}

void MainWindow::init() {
  m_editDialog = new EntryEditDialog(this, "editdialog");

  bool bViewEditWidget = m_config->readBoolEntry("Show Edit Widget", false);
  m_toggleEntryEditor->setChecked(bViewEditWidget);
  slotToggleEntryEditor();

  m_controller->setWidgets(m_groupView, m_detailedView, m_editDialog, m_entryView);

  initConnections();
}

void MainWindow::initWindow() {
  setIcon(KGlobal::iconLoader()->loadIcon(QString::fromLatin1("bookcase"), KIcon::Desktop));
  m_controller = new Controller(this, "controller");
  FileHandler::s_mainWindow = this;
}

void MainWindow::initStatusBar() {
  statusBar()->insertItem(i18n(ready), ID_STATUS_MSG);
  statusBar()->setItemAlignment(ID_STATUS_MSG, Qt::AlignLeft | Qt::AlignVCenter);

  statusBar()->insertItem(QString(), ID_STATUS_COUNT, 0, true);
  statusBar()->setItemAlignment(ID_STATUS_COUNT, Qt::AlignLeft | Qt::AlignVCenter);

  m_progress = new KProgress(100, statusBar());
  m_progress->setFixedHeight(statusBar()->minimumSizeHint().height());
  statusBar()->addWidget(m_progress);
  m_progress->hide();
}

void MainWindow::initActions() {
#ifndef NDEBUG
  if(!m_controller || !m_doc) {
    kdWarning() << "MainWindow::initActions() - controller and doc must be instantiated first!" << endl;
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
  fileNewMenu->setToolTip(i18n("Create a new collection"));
  fileNewMenu->setDelayed(false);

  KAction* action = new KAction(actionCollection(), "new_book_collection");
  action->setText(i18n("New &Book Collection"));
  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("book"), KIcon::User);
  action->setIconSet(QIconSet(pix));
  action->setToolTip(i18n("Create a new book collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::Book);

  action = new KAction(actionCollection(), "new_bibtex_collection");
  action->setText(i18n("New B&ibliography"));
  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("bibtex"), KIcon::User);
  action->setIconSet(QIconSet(pix));
  action->setToolTip(i18n("Create a new bibtex bibliography"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::Bibtex);

  action = new KAction(actionCollection(), "new_comic_book_collection");
  action->setText(i18n("New &Comic Book Collection"));
  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("comic"), KIcon::User);
  action->setIconSet(QIconSet(pix));
  action->setToolTip(i18n("Create a new comic book collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::ComicBook);

  action = new KAction(actionCollection(), "new_video_collection");
  action->setText(i18n("New &Video Collection"));
  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("video"), KIcon::User);
  action->setIconSet(QIconSet(pix));
  action->setToolTip(i18n("Create a new video collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::Video);

  action = new KAction(actionCollection(), "new_music_collection");
  action->setText(i18n("New &Music Collection"));
  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("album"), KIcon::User);
  action->setIconSet(QIconSet(pix));
  action->setToolTip(i18n("Create a new music collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::Album);

  action = new KAction(actionCollection(), "new_card_collection");
  action->setText(i18n("New C&ard Collection"));
  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("card"), KIcon::User);
  action->setIconSet(QIconSet(pix));
  action->setToolTip(i18n("Create a new trading card collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::Card);

  action = new KAction(actionCollection(), "new_coin_collection");
  action->setText(i18n("New C&oin Collection"));
  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("coin"), KIcon::User);
  action->setIconSet(QIconSet(pix));
  action->setToolTip(i18n("Create a new coin collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::Coin);

  action = new KAction(actionCollection(), "new_stamp_collection");
  action->setText(i18n("New &Stamp Collection"));
  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("stamp"), KIcon::User);
  action->setIconSet(QIconSet(pix));
  action->setToolTip(i18n("Create a new stamp collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::Stamp);

  action = new KAction(actionCollection(), "new_wine_collection");
  action->setText(i18n("New &Wine Collection"));
  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("wine"), KIcon::User);
  action->setIconSet(QIconSet(pix));
  action->setToolTip(i18n("Create a new wine collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::Wine);

  action = new KAction(actionCollection(), "new_custom_collection");
  action->setText(i18n("New C&ustom Collection"));
  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("filenew"), KIcon::Toolbar, 16);
  action->setIconSet(QIconSet(pix));
  action->setToolTip(i18n("Create a new custom collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::Base);

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
  m_fileImportMenu->setText(i18n("&Import"));
  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("fileimport"), KIcon::Toolbar);
  m_fileImportMenu->setIconSet(QIconSet(pix));
  m_fileImportMenu->setToolTip(i18n("Import collection data from other formats..."));
  m_fileImportMenu->setDelayed(false);

  action = new KAction(actionCollection(), "file_import_bookcase");
  action->setText(i18n("Import Bookcase Data"));
  action->setToolTip(i18n("Import another Bookcase data file"));
  m_fileImportMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, ImportDialog::BookcaseXML);

  action = new KAction(actionCollection(), "file_import_csv");
  action->setText(i18n("Import CSV Data"));
  action->setToolTip(i18n("Import a CSV file"));
  m_fileImportMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, ImportDialog::CSV);

  action = new KAction(actionCollection(), "file_import_bibtex");
  action->setText(i18n("Import Bibtex Data"));
  action->setToolTip(i18n("Import a Bibtex bibliography file"));
  m_fileImportMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, ImportDialog::Bibtex);

  action = new KAction(actionCollection(), "file_import_bibtexml");
  action->setText(i18n("Import Bibtexml Data"));
  action->setToolTip(i18n("Import a Bibtexml bibliography file"));
  m_fileImportMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, ImportDialog::Bibtexml);

  action = new KAction(actionCollection(), "file_import_xslt");
  action->setText(i18n("Import XSL Transform"));
  action->setToolTip(i18n("Import using an XSL Transform"));
  m_fileImportMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, ImportDialog::XSLT);

/**************** Export Menu ***************************/
  
  QSignalMapper* exportMapper = new QSignalMapper(this);
  connect(exportMapper, SIGNAL(mapped(int)),
          this, SLOT(slotFileExport(int)));

  m_fileExportMenu = new KActionMenu(actionCollection(), "file_export");
  m_fileExportMenu->setText(i18n("&Export"));
  pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("fileexport"), KIcon::Toolbar);
  m_fileExportMenu->setIconSet(QIconSet(pix));
  m_fileExportMenu->setToolTip(i18n("Export the collection data to other formats..."));
  m_fileExportMenu->setDelayed(false);

  action = new KAction(actionCollection(), "file_export_xml");
  action->setText(i18n("Export to XML"));
  action->setToolTip(i18n("Export to a Bookcase XML file"));
  m_fileExportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, ExportDialog::XML);

  action = new KAction(actionCollection(), "file_export_html");
  action->setText(i18n("Export to HTML"));
  action->setToolTip(i18n("Export to an HTML file"));
  m_fileExportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, ExportDialog::HTML);

  action = new KAction(actionCollection(), "file_export_csv");
  action->setText(i18n("Export to CSV"));
  action->setToolTip(i18n("Export to a comma-separated values file"));
  m_fileExportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, ExportDialog::CSV);

  action = new KAction(actionCollection(), "file_export_pilotdb");
  action->setText(i18n("Export to PilotDB"));
  action->setToolTip(i18n("Export to a PilotDB database"));
  m_fileExportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, ExportDialog::PilotDB);

  m_exportBibtex = new KAction(actionCollection(), "file_export_bibtex");
  m_exportBibtex->setText(i18n("Export to Bibtex"));
  m_exportBibtex->setToolTip(i18n("Export to a bibtex file"));
  m_fileExportMenu->insert(m_exportBibtex);
  connect(m_exportBibtex, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(m_exportBibtex, ExportDialog::Bibtex);

  m_exportBibtexml = new KAction(actionCollection(), "file_export_bibtexml");
  m_exportBibtexml->setText(i18n("Export to Bibtexml"));
  m_exportBibtexml->setToolTip(i18n("Export to a Bibtexml file"));
  m_fileExportMenu->insert(m_exportBibtexml);
  connect(m_exportBibtexml, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(m_exportBibtexml, ExportDialog::Bibtexml);

  action = new KAction(actionCollection(), "file_export_xslt");
  action->setText(i18n("Export XSL Transform"));
  action->setToolTip(i18n("Export using an XSL Transform"));
  m_fileExportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, ExportDialog::XSLT);

  /*************************************************
   * Edit menu
   *************************************************/
  action = KStdAction::cut(this, SLOT(slotEditCut()), actionCollection());
  action->setToolTip(i18n("Cut the selected text and puts it in the clipboard"));
  action = KStdAction::copy(this, SLOT(slotEditCopy()), actionCollection());
  action->setToolTip(i18n("Copy the selected text to the clipboard"));
  action = KStdAction::paste(this, SLOT(slotEditPaste()), actionCollection());
  action->setToolTip(i18n("Paste the clipboard contents"));
  action = KStdAction::selectAll(this, SLOT(slotEditSelectAll()), actionCollection());
  action->setToolTip(i18n("Select all the entries in the collection"));
  action = KStdAction::deselect(this, SLOT(slotEditDeselect()), actionCollection());
  action->setToolTip(i18n("Deselect all the entries in the collection"));

  m_editFind = KStdAction::find(this, SLOT(slotEditFind()), actionCollection());
  m_editFind->setToolTip(i18n("Search the collection"));
  m_editFindNext = KStdAction::findNext(this, SLOT(slotEditFindNext()), actionCollection());
  m_editFindNext->setToolTip(i18n("Find next match in the collection"));
  // gets enabled once one search is done
  m_editFindNext->setEnabled(false);
  m_editFindPrev = KStdAction::findPrev(this, SLOT(slotEditFindPrev()), actionCollection());
  m_editFindPrev->setToolTip(i18n("Find previous match in the collection"));
  // gets enabled once one search is done
  m_editFindPrev->setEnabled(false);

  /*************************************************
   * Collection menu
   *************************************************/
  m_editNewEntry = new KAction(i18n("&New Entry"), QString::fromLatin1("filenew"), CTRL + Key_N,
                               this, SLOT(slotNewEntry()),
                               actionCollection(), "edit_new_entry");
  m_editNewEntry->setToolTip(i18n("Create a new entry"));
  action = new KAction(i18n("&Edit Entry"), QString::fromLatin1("edit"), CTRL + Key_E,
                            this, SLOT(slotShowEntryEditor()),
                            actionCollection(), "edit_edit_entry");
  action->setToolTip(i18n("Edit the selected entries"));
  m_editCopyEntry = new KAction(i18n("&Copy Entry"), QString::fromLatin1("editcopy"), CTRL + Key_Y,
                                m_controller, SLOT(slotCopySelectedEntries()),
                                actionCollection(), "edit_copy_entry");
  m_editCopyEntry->setToolTip(i18n("Copy the selected entries"));
  m_editDeleteEntry = new KAction(i18n("&Delete Entry"), QString::fromLatin1("editdelete"), CTRL + Key_D,
                                  m_controller, SLOT(slotDeleteSelectedEntries()),
                                  actionCollection(), "edit_delete_entry");
  m_editDeleteEntry->setToolTip(i18n("Delete the selected entries"));
  action = new KAction(i18n("&Rename Collection..."), QString::fromLatin1("editclear"), CTRL + Key_R,
                       m_doc, SLOT(slotRenameCollection()),
                       actionCollection(), "edit_rename_collection");
  action->setToolTip(i18n("Rename the collection"));
  action = new KAction(i18n("Collection &Fields..."), QString::fromLatin1("edit"), CTRL + Key_U,
                       this, SLOT(slotShowCollectionFieldsDialog()),
                       actionCollection(), "edit_fields");
  action->setToolTip(i18n("Modify the collection fields"));
  m_editConvertBibtex = new KAction(i18n("Convert to &Bibliography"), 0,
                                    m_doc, SLOT(slotConvertToBibtex()),
                                    actionCollection(), "edit_convert_bibtex");
  m_editConvertBibtex->setToolTip(i18n("Convert a book collection to a bibliography"));
  m_editStringMacros = new KAction(i18n("String &Macros..."), 0,
                                   this, SLOT(slotShowStringMacroDialog()),
                                   actionCollection(), "edit_string_macros");
  m_editStringMacros->setToolTip(i18n("Edit the Bibtex string macros"));

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
#if KDE_IS_VERSION(3,1,90)
  createStandardStatusBarAction();
#else
  m_toggleStatusBar = KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()),
                                                actionCollection());
  m_toggleStatusBar->setToolTip(i18n("Enable/disable the statusbar"));
#endif
  KStdAction::configureToolbars(this, SLOT(slotConfigToolbar()), actionCollection());
  KStdAction::keyBindings(this, SLOT(slotConfigKeys()), actionCollection());
  m_toggleGroupWidget = new KToggleAction(i18n("Show Grou&p View"), 0,
                                          this, SLOT(slotToggleGroupWidget()),
                                          actionCollection(), "toggle_group_widget");
  m_toggleGroupWidget->setToolTip(i18n("Enable/disable the group view"));
  m_toggleEntryEditor = new KToggleAction(i18n("Show Entry &Editor"), 0,
                                         this, SLOT(slotToggleEntryEditor()),
                                         actionCollection(), "toggle_edit_widget");
  m_toggleEntryEditor->setToolTip(i18n("Enable/disable the editor"));
  m_toggleEntryView = new KToggleAction(i18n("Show Entry &View"), 0,
                                        this, SLOT(slotToggleEntryView()),
                                        actionCollection(), "toggle_entry_view");
  m_toggleEntryView->setToolTip(i18n("Enable/disable the entry view"));
  action = new KAction(i18n("Advanced &Filter..."), QString::fromLatin1("filter"), CTRL + Key_J,
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
  action = new LabelAction(i18n("Group By:"), 0,
                           actionCollection(), "change_unit_grouping_label");
  action->setToolTip(i18n("Change the grouping of the collection"));
  m_entryGrouping = new KSelectAction(i18n("&Group Selection"), 0, this,
                                     SLOT(slotChangeGrouping()),
                                     actionCollection(), "change_unit_grouping");
  m_entryGrouping->setToolTip(i18n("Change the grouping of the collection"));

  action = new LabelAction(i18n("Quick Filter:"), 0,
                           actionCollection(), "quick_filter_label");
  action->setToolTip(i18n("Filter the collection"));
  m_quickFilter = new LineEditAction(i18n("Filter Entry"), 0,
                                     actionCollection(), "quick_filter");
  // want to update every time the filter text changes
  connect(m_quickFilter, SIGNAL(textChanged(const QString&)),
          this, SLOT(slotUpdateFilter(const QString&)));

#ifdef UIFILE
  kdWarning() << "MainWindow::initActions() - change createGUI() call!" << endl;
  createGUI(UIFILE);
#else
  createGUI();
#endif
}

void MainWindow::initDocument() {
  m_doc = new Data::Document(this);

  // allow status messages from the document
  connect(m_doc, SIGNAL(signalStatusMsg(const QString&)),
          SLOT(slotStatusMsg(const QString&)));

  // do stuff that changes when the doc is modified
  connect(m_doc, SIGNAL(signalModified()),
          SLOT(slotEnableModifiedActions()));

  connect(m_doc, SIGNAL(signalCollectionAdded(Bookcase::Data::Collection*)),
          m_controller, SLOT(slotCollectionAdded(Bookcase::Data::Collection*)));
  connect(m_doc, SIGNAL(signalCollectionDeleted(Bookcase::Data::Collection*)),
          m_controller, SLOT(slotCollectionDeleted(Bookcase::Data::Collection*)));
  connect(m_doc, SIGNAL(signalCollectionRenamed(const QString&)),
          m_controller, SLOT(slotCollectionRenamed(const QString&)));
}

void MainWindow::initView() {
  m_split = new QSplitter(Qt::Horizontal, this);
  setCentralWidget(m_split);

  // m_split is the parent widget for the left side column view
  m_groupView = new GroupView(m_split, "groupview");

  m_split2 = new QSplitter(Qt::Vertical, m_split);

  m_detailedView = new DetailedListView(m_split2, "detailedlistview");
  m_entryView = new EntryView(m_split2, "entryview");
}

void MainWindow::initConnections() {
  connect(m_groupView, SIGNAL(signalEntrySelected(QWidget*, const Bookcase::Data::EntryList&)),
          m_controller, SLOT(slotUpdateSelection(QWidget*, const Bookcase::Data::EntryList&)));
  connect(m_detailedView, SIGNAL(signalEntrySelected(QWidget*, const Bookcase::Data::EntryList&)),
          m_controller, SLOT(slotUpdateSelection(QWidget*, const Bookcase::Data::EntryList&)));
  connect(m_editDialog, SIGNAL(signalClearSelection(QWidget*)),
          m_controller, SLOT(slotUpdateSelection(QWidget*)));

  // when an entry is double clicked, make sure the editor is visible
  connect(m_groupView, SIGNAL(doubleClicked(QListViewItem*)),
          SLOT(slotShowEntryEditor(QListViewItem*)));
  connect(m_detailedView, SIGNAL(doubleClicked(QListViewItem*)),
          SLOT(slotShowEntryEditor(QListViewItem*)));

  // have to toggle the menu item if the dialog gets closed
  connect(m_editDialog, SIGNAL(finished()),
          this, SLOT(slotEditDialogFinished()));

  // since the detailed view can take a while to populate,
  // it gets a progress monitor, too
  connect(m_detailedView, SIGNAL(signalFractionDone(float)),
          SLOT(slotUpdateFractionDone(float)));

  connect(m_doc, SIGNAL(signalEntrySelected(Bookcase::Data::Entry*, const QString&)),
          m_controller, SLOT(slotUpdateSelection(Bookcase::Data::Entry*, const QString&)));

  connect(m_editDialog, SIGNAL(signalSaveEntry(Bookcase::Data::Entry*)),
          m_doc, SLOT(slotSaveEntry(Bookcase::Data::Entry*)));

  connect(m_groupView, SIGNAL(signalDeleteEntry(Bookcase::Data::Entry*)),
          m_doc, SLOT(slotDeleteEntry(Bookcase::Data::Entry*)));
  connect(m_detailedView, SIGNAL(signalDeleteEntry(Bookcase::Data::Entry*)),
          m_doc, SLOT(slotDeleteEntry(Bookcase::Data::Entry*)));

  connect(m_doc, SIGNAL(signalEntryAdded(Bookcase::Data::Entry*)),
          m_controller, SLOT(slotEntryAdded(Bookcase::Data::Entry*)));
  connect(m_doc, SIGNAL(signalEntryModified(Bookcase::Data::Entry*)),
          m_controller, SLOT(slotEntryModified(Bookcase::Data::Entry*)));
  connect(m_doc, SIGNAL(signalEntryDeleted(Bookcase::Data::Entry*)),
          m_controller, SLOT(slotEntryDeleted(Bookcase::Data::Entry*)));
}

void MainWindow::initFileOpen(bool nofile_) {
  // check to see if most recent file should be opened
  m_config->setGroup("General Options");
  if(!nofile_
      && m_config->readBoolEntry("Reopen Last File", true)
      && !m_config->readEntry("Last Open File").isEmpty()) {
    KURL lastFile = KURL(m_config->readEntry("Last Open File"));
    slotFileOpen(lastFile);
  } else {
    // instead add default initial book collection
    m_controller->slotCollectionAdded(m_doc->collection());

    m_fileSave->setEnabled(false);
    slotEnableOpenedActions();
    slotEnableModifiedActions(false);

    slotEntryCount(0);
  }
}

// These are general options.
// The options that can be changed in the "Configuration..." dialog
// are taken care of by the ConfigDialog object.
void MainWindow::saveOptions() {
//  kdDebug() << "MainWindow::saveOptions()" << endl;
  // for some reason, the m_config pointer is getting changed, but
  // I can't figure out where, so just to be on the safe side
  if(m_config != kapp->config()) {
    kdDebug() << "MainWindow::saveOptions() - inconsistent KConfig pointers!" << endl;
    m_config = kapp->config();
  }
  
  saveMainWindowSettings(m_config, QString::fromLatin1("Main Window Options"));
  
  m_config->setGroup("General Options");
#if !KDE_IS_VERSION(3,1,90)
  m_config->writeEntry("Show Statusbar", m_toggleStatusBar->isChecked());
#endif
  m_config->writeEntry("Show Group Widget", m_toggleGroupWidget->isChecked());
  m_config->writeEntry("Show Edit Widget", m_toggleEntryEditor->isChecked());
  m_config->writeEntry("Show Entry View", m_toggleEntryView->isChecked());

#if KDE_VERSION < 306
  toolBar("mainToolBar")->saveSettings(m_config, QString::fromLatin1("MainToolBar"));
  toolBar("collectionToolBar")->saveSettings(m_config, QString::fromLatin1("CollectionToolBar"));
#endif
  m_fileOpenRecent->saveEntries(m_config, QString::fromLatin1("Recent Files"));
  if(!isNewDocument()) {
    m_config->writeEntry("Last Open File", m_doc->URL().url());
  } else {
    // decided I didn't want the entry over-written when exited with an empty document
//    m_config->writeEntry("Last Open File", QString::null);
  }

  m_config->setGroup("Main Window Options");
  m_config->writeEntry("Main Splitter Sizes", m_split->sizes());
  m_config->writeEntry("Secondary Splitter Sizes", m_split2->sizes());

  // this is used in the EntryEditDialog constructor, too
  m_editDialog->saveDialogSize(QString::fromLatin1("Edit Dialog Options"));

  saveCollectionOptions(m_doc->collection());
}

void MainWindow::readCollectionOptions(Data::Collection* coll_) {
  m_config->setGroup(QString::fromLatin1("Options - %1").arg(coll_->entryName()));

  QString defaultGroup = coll_->defaultGroupField();
  QString entryGroup = m_config->readEntry("Group By", defaultGroup);
  if(!coll_->entryGroups().contains(entryGroup)) {
    entryGroup = defaultGroup;
  }
  m_groupView->setGroupField(coll_, entryGroup);

  QString entryXSLTFile = m_config->readEntry("Entry Template", QString::fromLatin1("Default"));
  m_entryView->setXSLTFile(entryXSLTFile + QString::fromLatin1(".xsl"));

  // make sure the right combo element is selected
  slotUpdateCollectionToolBar(coll_);
}

void MainWindow::saveCollectionOptions(Data::Collection* coll_) {
//  kdDebug() << "MainWindow::saveCollectionOptions() = " << coll_->entryName() << endl;
  // don't save initial collection options, or empty collections
  if(!coll_ || coll_->entryList().isEmpty() || isNewDocument()) {
    return;
  }

  m_config->setGroup(QString::fromLatin1("Options - %1").arg(coll_->entryName()));
  QString groupName = coll_->entryGroups()[m_entryGrouping->currentItem()];
  m_config->writeEntry("Group By", groupName);

  m_detailedView->saveConfig(coll_);
}

void MainWindow::readOptions() {
//  kdDebug() << "MainWindow::readOptions()" << endl;
  // for some reason, the m_config pointer is getting changed, but
  // I can't figure out where, so just to be on the safe side
  if(m_config != kapp->config()) {
    kdDebug() << "MainWindow::readOptions() - inconsistent KConfig pointers!" << endl;
    m_config = kapp->config();
  }

  applyMainWindowSettings(m_config, QString::fromLatin1("Main Window Options"));

  m_config->setGroup("Main Window Options");

  QValueList<int> splitList = m_config->readIntListEntry("Main Splitter Sizes");
  if(!splitList.empty()) {
    m_split->setSizes(splitList);
  }

  splitList = m_config->readIntListEntry("Secondary Splitter Sizes");
  if(!splitList.empty()) {
    m_split2->setSizes(splitList);
  }

  m_config->setGroup("General Options");
  bool showCount = m_config->readBoolEntry("Show Group Count", true);
  m_groupView->showCount(showCount);

#if KDE_VERSION < 306
  toolBar("mainToolBar")->applySettings(m_config, QString::fromLatin1("MainToolBar"));
  m_toggleToolBar->setChecked(!toolBar("mainToolBar")->isHidden());
  slotToggleToolBar();

  toolBar("collectionToolBar")->applySettings(m_config, QString::fromLatin1("CollectionToolBar"));
  m_toggleCollectionBar->setChecked(!toolBar("collectionToolBar")->isHidden());
  slotToggleCollectionBar();
#endif
  connect(toolBar("collectionToolBar"), SIGNAL(modechange()), SLOT(slotUpdateToolbarIcons()));

#if !KDE_IS_VERSION(3,1,90)
  bool bViewStatusBar = m_config->readBoolEntry("Show Statusbar", true);
  m_toggleStatusBar->setChecked(bViewStatusBar);
  slotToggleStatusBar();
#endif

  bool bViewGroupWidget = m_config->readBoolEntry("Show Group Widget", true);
  m_toggleGroupWidget->setChecked(bViewGroupWidget);
  slotToggleGroupWidget();

  bool bViewEntryView = m_config->readBoolEntry("Show Entry View", true);
  m_toggleEntryView->setChecked(bViewEntryView);
  slotToggleEntryView();

  // initialize the recent file list
  m_fileOpenRecent->loadEntries(m_config, QString::fromLatin1("Recent Files"));

  bool autoCapitals = m_config->readBoolEntry("Auto Capitalization", true);
  Data::Field::setAutoCapitalize(autoCapitals);

  bool autoFormat = m_config->readBoolEntry("Auto Format", true);
  Data::Field::setAutoFormat(autoFormat);

  QStringList articles;
  if(m_config->hasKey("Articles")) {
    articles = m_config->readListEntry("Articles", ',');
  } else {
    articles = Data::Field::defaultArticleList();
  }
  Data::Field::setArticleList(articles);

  QStringList suffixes;
  if(m_config->hasKey("Name Suffixes")) {
    suffixes = m_config->readListEntry("Name Suffixes", ',');
  } else {
    suffixes = Data::Field::defaultSuffixList();
  }
  Data::Field::setSuffixList(suffixes);

  QStringList prefixes;
  if(m_config->hasKey("Surname Prefixes")) {
    prefixes = m_config->readListEntry("Surname Prefixes", ',');
  } else {
    prefixes = Data::Field::defaultSurnamePrefixList();
  }
  Data::Field::setSurnamePrefixList(prefixes);

  // Don't read any options for the edit dialog here, since it's not yet initialized.
  // Put them in init()
}

void MainWindow::saveProperties(KConfig* cfg_) {
  if(!isNewDocument() && !m_doc->isModified()) {
    // saving to tempfile not necessary
  } else {
    KURL url = m_doc->URL();
    cfg_->writeEntry("filename", url.url());
    cfg_->writeEntry("modified", m_doc->isModified());
    QString tempname = KURL::encode_string(kapp->tempSaveName(url.url()));
    KURL tempurl;
    tempurl.setPath(tempname);
    m_doc->saveDocument(tempurl);
  }
}

void MainWindow::readProperties(KConfig* cfg_) {
  QString filename = cfg_->readEntry(QString::fromLatin1("filename"));
  bool modified = cfg_->readBoolEntry(QString::fromLatin1("modified"), false);
  if(modified) {
    bool canRecover;
    QString tempname = kapp->checkRecoverFile(filename, canRecover);

    if(canRecover) {
      KURL tempurl;
      tempurl.setPath(tempname);
      m_doc->openDocument(tempurl);
      m_doc->slotSetModified(true);
      setCaption(tempurl.fileName(), true);
      QFile::remove(tempname);
    }
  } else {
    if(!filename.isEmpty()) {
      KURL url;
      url.setPath(filename);
      m_doc->openDocument(url);
      setCaption(filename, false);
    }
  }
}

bool MainWindow::queryClose() {
//  kdDebug() << "MainWindow::queryClose()" << endl;
  return m_editDialog->queryModified() && m_doc->saveModified();
}

bool MainWindow::queryExit() {
//  kdDebug() << "MainWindow::queryExit()" << endl;
  ImageFactory::clean();
  saveOptions();
  return true;
}

Bookcase::EntryItem* MainWindow::selectedOrFirstItem() {
  QListViewItem* item = m_detailedView->currentItem();
  if(!item) {
    item = m_detailedView->firstChild();
  }
  return static_cast<EntryItem*>(item);
}

void MainWindow::slotFileNew(int type_) {
  slotStatusMsg(i18n("Creating new document..."));

  if(m_editDialog->queryModified() && m_doc->saveModified()) {
    m_doc->newDocument(type_);
    m_fileOpenRecent->setCurrentItem(-1);
    slotEnableOpenedActions();
    slotEnableModifiedActions(false);
    m_newDocument = true;
  }

  slotStatusMsg(i18n(ready));
}

void MainWindow::slotFileOpen() {
  slotStatusMsg(i18n("Opening file..."));

  if(m_editDialog->queryModified() && m_doc->saveModified()) {
    QString filter = i18n("*.bc|Bookcase files (*.bc)");
    filter += QString::fromLatin1("\n");
    filter += i18n("*.xml|XML files (*.xml)");
    filter += QString::fromLatin1("\n");
    filter += i18n("*|All files");
    // keyword 'open'
    KURL url = KFileDialog::getOpenURL(QString::fromLatin1(":open"), filter,
                                       this, i18n("Open File..."));
    if(!url.isEmpty() && url.isValid()) {
      slotFileOpen(url);
    }
  }
  slotStatusMsg(i18n(ready));
}

void MainWindow::slotFileOpen(const KURL& url_) {
  slotStatusMsg(i18n("Opening file..."));
  
  if(m_editDialog->queryModified() && m_doc->saveModified()) {
    bool success = openURL(url_);
    if(success) {
      m_fileOpenRecent->addURL(url_);
      m_fileOpenRecent->setCurrentItem(-1);
    }// else {
     // m_fileOpenRecent->removeURL(url_);
    //}
  }

  slotStatusMsg(i18n(ready));
}

void MainWindow::slotFileOpenRecent(const KURL& url_) {
  slotStatusMsg(i18n("Opening file..."));

  if(m_editDialog->queryModified() && m_doc->saveModified()) {
    bool success = openURL(url_);
    if(!success) {
      m_fileOpenRecent->removeURL(url_);
    }
  }

  slotStatusMsg(i18n(ready));
}

bool MainWindow::openURL(const KURL& url_) {
//  kdDebug() <<  "MainWindow::openURL() - " << url_.prettyURL() << endl;

  // try to open document
  kapp->setOverrideCursor(Qt::waitCursor);
  bool success = m_doc->openDocument(url_);
  kapp->restoreOverrideCursor();

  if(success) {
    m_quickFilter->clear();
    slotEnableOpenedActions();
    slotEnableModifiedActions(false);
    m_newDocument = false;
  }

  return success;
}

void MainWindow::slotFileSave() {
  if(!m_editDialog->queryModified()) {
    return;
  }
  slotStatusMsg(i18n("Saving file..."));

  if(isNewDocument()) {
    slotFileSaveAs();
  } else {
    m_doc->saveDocument(m_doc->URL());
    m_newDocument = false;
  }
  m_fileSave->setEnabled(false);
  setCaption(m_doc->URL().fileName(), false);

  slotStatusMsg(i18n(ready));
}

void MainWindow::slotFileSaveAs() {
  if(!m_editDialog->queryModified()) {
    return;
  }
  
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
    m_newDocument = false;
  }

  slotStatusMsg(i18n(ready));
}

void MainWindow::slotFilePrint() {
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

  Export::HTMLExporter exporter(m_doc->collection(), m_detailedView->visibleEntries());
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

void MainWindow::slotFileQuit() {
  slotStatusMsg(i18n("Exiting..."));

  // this gets called in queryExit() anyway
  //saveOptions();
  close();

  slotStatusMsg(i18n(ready));
}

void MainWindow::slotEditCut() {
  activateEditWidgetSlot(SLOT(cut()));
}

void MainWindow::slotEditCopy() {
  activateEditWidgetSlot(SLOT(copy()));
}

void MainWindow::slotEditPaste() {
  activateEditWidgetSlot(SLOT(paste()));
}

void MainWindow::activateEditWidgetSlot(const char* slot_) {
//  kdDebug() << "MainWindow::slotEditPaste() - " << slot_ << endl;
  // the edit widget is the only one that copies, cuts, and pastes
  QWidget* w;
  if(m_editDialog->isVisible()) {
    w = m_editDialog->focusWidget();
  } else {
    w = kapp->focusWidget();
  }

  if(w && w->isVisible()) {
//    kdDebug() << "MainWindow::slotEditPaste() - using a " << w->className() << endl;
    QStrList slotNames = w->metaObject()->slotNames(true);
    // The SLOT macro adds a '1' in front of the slot name
    // which is not present in the slotNames list, so ignore first char
    if(slotNames.contains(slot_+sizeof(char))) {
      QSignal signal;
      signal.connect(w, slot_);
      signal.activate();
    }
  }
}

void MainWindow::slotEditSelectAll() {
  m_detailedView->selectAllVisible();
}

void MainWindow::slotEditDeselect() {
  m_controller->slotUpdateSelection(0, Data::EntryList());
}

void MainWindow::slotEditFind() {
  if(!m_findDlg) {
    m_findDlg = new FindDialog(this);
    m_editFindNext->setEnabled(true);
    m_editFindPrev->setEnabled(true);
  } else {
    m_findDlg->updateFieldList();
  }
  
  m_findDlg->show();
}

void MainWindow::slotEditFindNext() {
  if(m_doc->isEmpty()) {
    return;
  }
  
  if(m_findDlg) {
    m_findDlg->slotFindNext();
  }
}

void MainWindow::slotEditFindPrev() {
  if(m_doc->isEmpty()) {
    return;
  }
  
  if(m_findDlg) {
    m_findDlg->slotFindPrev();
  }
}

void MainWindow::slotConfigToolbar() {
  saveMainWindowSettings(m_config, QString::fromLatin1("Main Window Options"));
#ifdef UIFILE
  KEditToolbar dlg(actionCollection(), UIFILE);
#else
  KEditToolbar dlg(actionCollection());
#endif
  connect(&dlg, SIGNAL(newToolbarConfig()), this, SLOT(slotNewToolbarConfig()));
  dlg.exec();
}

void MainWindow::slotNewToolbarConfig() {
  applyMainWindowSettings(m_config, QString::fromLatin1("Main Window Options"));
#ifdef UIFILE
  createGUI(UIFILE);
#else
  createGUI();
#endif
}

void MainWindow::slotConfigKeys() {
  KKeyDialog::configure(actionCollection());
}

void MainWindow::slotToggleToolBar() {
#if KDE_VERSION < 306
  if(m_toggleToolBar->isChecked()) {
    toolBar("mainToolBar")->show();
  } else {
    toolBar("mainToolBar")->hide();
  }
#endif
}

void MainWindow::slotToggleCollectionBar() {
#if KDE_VERSION < 306
  if(m_toggleCollectionBar->isChecked()) {
    toolBar("collectionToolBar")->show();
  } else {
    toolBar("collectionToolBar")->hide();
  }
#endif
}

void MainWindow::slotToggleStatusBar() {
  if(m_toggleStatusBar->isChecked()) {
    statusBar()->show();
  } else {
    statusBar()->hide();
  }
}

void MainWindow::slotToggleGroupWidget() {
  if(m_toggleGroupWidget->isChecked()) {
    m_groupView->show();
  } else {
    m_groupView->hide();
  }
}

void MainWindow::slotToggleEntryEditor() {
  if(m_toggleEntryEditor->isChecked()) {
    m_editDialog->show();
  } else {
    m_editDialog->hide();
  }
}

void MainWindow::slotToggleEntryView() {
  if(m_toggleEntryView->isChecked()) {
    m_entryView->show();
  } else {
    m_entryView->hide();
  }
}

void MainWindow::slotShowConfigDialog() {
  if(!m_configDlg) {
    m_configDlg = new ConfigDialog(this);
    m_configDlg->readConfiguration(m_config);
    connect(m_configDlg, SIGNAL(signalConfigChanged()),
            SLOT(slotHandleConfigChange()));
    connect(m_configDlg, SIGNAL(finished()),
            SLOT(slotHideConfigDialog()));
  } else {
#if KDE_IS_VERSION(3,1,90)
    KWin::activateWindow(m_configDlg->winId());
#else
    KWin::setActiveWindow(m_configDlg->winId());
#endif
  }
  m_configDlg->show();
}

void MainWindow::slotHideConfigDialog() {
  if(m_configDlg) {
    m_configDlg->delayedDestruct();
    m_configDlg = 0;
  }
}

void MainWindow::slotShowTipOfDay(bool force_/*=true*/) {
  QString tipfile = KGlobal::dirs()->findResource("appdata", QString::fromLatin1("bookcase.tips"));
  KTipDialog::showTip(this, tipfile, force_);
}

void MainWindow::slotStatusMsg(const QString& text_) {
  statusBar()->clear();
  // add a space at the beginning and end for asthetic reasons
  statusBar()->changeItem(QString::fromLatin1(" ")+text_+QString::fromLatin1(" "), ID_STATUS_MSG);
  kapp->processEvents();
}

void MainWindow::slotEntryCount(int count_) {
  Data::Collection* coll = m_doc->collection();
  if(!coll) {
    return;
  }

  int count = coll->entryCount();
  QString text = i18n("Total Entries: %1").arg(count);
  // I add a space before and after for asthetic reasons
  text.prepend(QString::fromLatin1(" "));
  text.append(QString::fromLatin1(" "));
  // if more than one book is selected, add the number of selected books
  if(count_ > 1) {
    text += i18n("(%1 selected)").arg(count_);
    text += QString::fromLatin1(" ");
  }
    
  statusBar()->changeItem(text, ID_STATUS_COUNT);
}

void MainWindow::slotEnableOpenedActions() {
  slotUpdateToolbarIcons();

  // collapse all the groups (depth=1)
  m_groupView->slotCollapseAll(1);
  // expand the collections
  m_groupView->slotExpandAll(0);

  m_fileSaveAs->setEnabled(true);
  m_filePrint->setEnabled(true);
  m_fileExportMenu->setEnabled(true);
  m_editFind->setEnabled(true);

  Data::Collection::CollectionType type = m_doc->collection()->collectionType();
  m_exportBibtex->setEnabled(type == Data::Collection::Bibtex);
  m_exportBibtexml->setEnabled(type == Data::Collection::Bibtex);
  m_editConvertBibtex->setEnabled(type == Data::Collection::Book);
  m_editStringMacros->setEnabled(type == Data::Collection::Bibtex);

  // close the filter dialog when a new collection is opened
  slotHideFilterDialog();
  slotHideStringMacroDialog();
}

void MainWindow::slotEnableModifiedActions(bool modified_ /*= true*/) {
  if(m_fileSave->isEnabled() != modified_) {
    setCaption(m_doc->URL().fileName(), modified_);
    m_fileSave->setEnabled(modified_);
  }
  // when a collection is replaced, slotEnableOpenedActions() doesn't get called automatically
  if(m_doc->collection()) {
    Data::Collection::CollectionType type = m_doc->collection()->collectionType();
    m_exportBibtex->setEnabled(type == Data::Collection::Bibtex);
    m_exportBibtexml->setEnabled(type == Data::Collection::Bibtex);
    m_editConvertBibtex->setEnabled(type == Data::Collection::Book);
    m_editStringMacros->setEnabled(type == Data::Collection::Bibtex);
  }
}

void MainWindow::slotUpdateFractionDone(float f_) {
//  kdDebug() << "MainWindow::slotUpdateFractionDone() - " << f_ << endl;
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
//  kdDebug() << "MainWindow::slotUpdateFractionDone() - " << progress << endl;
  m_progress->setValue(static_cast<int>((m_currentStep -1 + f_)/static_cast<float>(m_maxSteps) * 100.0));
  kapp->processEvents();
}

void MainWindow::slotHandleConfigChange() {
  // for some reason, the m_config pointer is getting changed, but
  // I can't figure out where, so just to be on the safe side
  if(m_config != kapp->config()) {
    m_config = kapp->config();
  }

  m_configDlg->saveConfiguration(m_config);

  // make sure to update the other that depend on options being changed
  m_config->setGroup("General Options");
  m_groupView->showCount(m_config->readBoolEntry("Show Group Count"));

  m_config->setGroup(QString::fromLatin1("Options - %1").arg(m_doc->collection()->entryName()));
  m_entryView->setXSLTFile(m_config->readEntry("Entry Template") + QString::fromLatin1(".xsl"));
}

void MainWindow::slotUpdateCollectionToolBar(Data::Collection* coll_/*=0*/) {
//  kdDebug() << "MainWindow::updateCollectionToolBar()" << endl;

  //TODO fix this later
  if(!coll_){
    coll_ = m_doc->collection();
  }
  if(!coll_) {
    return;
  }
  
  QString current = m_groupView->groupBy();
  if(current.isEmpty() || !coll_->entryGroups().contains(current)) {
    current = coll_->defaultGroupField();
    m_groupView->setGroupField(coll_, current);
  }

  QStringList groups = coll_->entryGroups();
  if(groups.isEmpty()) {
    m_entryGrouping->clear();
    return;
  }

  QStringList groupTitles;
  int index = 0;
  QStringList::ConstIterator groupIt = groups.begin();
  for(int i = 0; groupIt != groups.end(); ++groupIt, ++i) {
    // special case for people "pseudo-group"
    if(*groupIt == QString::fromLatin1("_people")) {
      groupTitles << (QString::fromLatin1("<") + i18n("People") + QString::fromLatin1(">"));
    } else {
      groupTitles << coll_->fieldTitleByName(*groupIt);
    }
    if(*groupIt == current) {
      index = i;
    }
  }
// is it more of a performance hit to compare two stringlists then to repopulate needlessly?
//  if(groupTitles != m_entryGrouping->items()) {
    m_entryGrouping->setItems(groupTitles);
//  }
  m_entryGrouping->setCurrentItem(index);
  // Bug in Qt such that combobox's width is never adjusted, nothing I can do about it.
}

void MainWindow::slotChangeGrouping() {
//  kdDebug() << "MainWindow::slotChangeGrouping()" << endl;
  unsigned idx = m_entryGrouping->currentItem();

  Data::Collection* coll = m_doc->collection();
  QString groupName;
  if(idx < coll->entryGroups().count()) {
    groupName = coll->entryGroups()[idx];
  } else {
    groupName = coll->defaultGroupField();
  }
//  kdDebug() << "\tchange to " << groupName << endl;
  m_groupView->setGroupField(coll, groupName);
}

void MainWindow::doPrint(const QString& html_) {
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
    printer->setDocName(m_doc->URL().prettyURL());

    QPainter *p = new QPainter;
    p->begin(printer);
    
    // mostly taken from KHTMLView::print()
    QString headerLeft = KGlobal::locale()->formatDate(QDate::currentDate(), false);
    QString headerRight = m_doc->URL().prettyURL();
    QString footerMid;
    
    QFont headerFont(QString::fromLatin1("helvetica"), 8);
    p->setFont(headerFont);
    const int lspace = p->fontMetrics().lineSpacing();
    const int headerHeight = (lspace * 3) / 2;
    
    QPaintDeviceMetrics metrics(printer);
    const int pageHeight = metrics.height() - 2*headerHeight;
    const int pageWidth = metrics.width();
            
//    kdDebug() << "MainWindow::doPrint() - pageHeight = " << pageHeight << ""
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

void MainWindow::XSLTError() {
  QString str = i18n("Bookcase encountered an error in XSLT processing.\n");
  str += i18n("Please check your installation.");
  KMessageBox::sorry(this, str);
}

//void MainWindow::FileError(const QString& filename) {
//  QString str = i18n("Bookcase is unable to find a required file - %1.\n").arg(filename);
//  str += i18n("Please check your installation.");
//  KMessageBox::sorry(this, str);
//}

void MainWindow::slotShowFilterDialog() {
  if(!m_filterDlg) {
    m_filterDlg = new FilterDialog(m_detailedView, this);
    m_filterDlg->setFilter(m_detailedView->filter());
//    connect(m_lookupDlg, SIGNAL(signalConfigChanged()),
//            SLOT(slotHandleConfigChange()));
    m_quickFilter->blockSignals(true);
    connect(m_filterDlg, SIGNAL(filterApplied()),
            m_quickFilter, SLOT(clear()));
    connect(m_filterDlg, SIGNAL(finished()),
            SLOT(slotHideFilterDialog()));
  } else {
#if KDE_IS_VERSION(3,1,90)
    KWin::activateWindow(m_filterDlg->winId());
#else
    KWin::setActiveWindow(m_filterDlg->winId());
#endif
  }
  m_filterDlg->show();
}

void MainWindow::slotHideFilterDialog() {
  m_quickFilter->blockSignals(false);
  if(m_filterDlg) {
    m_filterDlg->delayedDestruct();
    m_filterDlg = 0;
  }
}

void MainWindow::slotUpdateFilter(const QString& text_) {
  if(m_detailedView) {
    QString text = text_.stripWhiteSpace();
    if(text.isEmpty()) {
      // passing a null pointer just shows everything
      m_detailedView->setFilter(static_cast<Filter*>(0));
      if(m_filterDlg) {
        m_filterDlg->slotClear();
      }
    } else {
      Filter* filter = new Filter(Filter::MatchAny);
      // if the text contains any non-Word characters, assume it's a regexp
      QRegExp rx(QString::fromLatin1("\\W"));
      FilterRule* rule;
      if(text.find(rx) == -1) {
        // an empty field string means check every field
        rule = new FilterRule(QString::null, text, FilterRule::FuncContains);
      } else {
        // if it isn't valid, hold off on applying the filter
        QRegExp tx(text);
        if(!tx.isValid()) {
          return;
        }
        rule = new FilterRule(QString::null, text, FilterRule::FuncRegExp);
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

void MainWindow::slotShowCollectionFieldsDialog() {
  if(!m_collFieldsDlg) {
    m_collFieldsDlg = new CollectionFieldsDialog(m_doc->collection(), this);
    connect(m_collFieldsDlg, SIGNAL(finished()),
            SLOT(slotHideCollectionFieldsDialog()));
    connect(m_collFieldsDlg, SIGNAL(signalCollectionModified()),
            m_doc, SLOT(slotSetModified()));
  } else {
#if KDE_IS_VERSION(3,1,90)
    KWin::activateWindow(m_collFieldsDlg->winId());
#else
    KWin::setActiveWindow(m_collFieldsDlg->winId());
#endif
  }
  m_collFieldsDlg->show();
}

void MainWindow::slotHideCollectionFieldsDialog() {
  if(m_collFieldsDlg) {
    m_collFieldsDlg->delayedDestruct();
    m_collFieldsDlg = 0;
  }
}

void MainWindow::slotFileImport(int format_) {
  slotStatusMsg(i18n("Importing data..."));
  m_quickFilter->clear();

  ImportDialog::ImportFormat format = static_cast<ImportDialog::ImportFormat>(format_);
  KURL url;
  if(ImportDialog::selectFileFirst(format)) {
    url = KFileDialog::getOpenURL(QString::fromLatin1(":import"), ImportDialog::fileFilter(format),
                                   this, i18n("Import File..."));
    bool ok = url.isValid();
#if KDE_IS_VERSION(3,1,90)
    ok &= KIO::NetAccess::exists(url, false, this);
#elif KDE_IS_VERSION(3,0,90)
    ok &= KIO::NetAccess::exists(url, false);
#else
    ok &= KIO::NetAccess::exists(url);
#endif
    if(!ok) {
      slotStatusMsg(i18n(ready));
      return;
    }
  }

  ImportDialog dlg(format, url, this, "importdlg");
  if(dlg.exec() != QDialog::Accepted) {
    slotStatusMsg(i18n(ready));
    return;
  }

  if(m_editDialog->queryModified() && (!dlg.replaceCollection() || m_doc->saveModified())) {
    kapp->setOverrideCursor(Qt::waitCursor);
    Data::Collection* coll = dlg.collection();
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
      m_newDocument = true;
      slotEnableOpenedActions();
      slotEnableModifiedActions(false);
    }
    kapp->restoreOverrideCursor();
  }

  slotStatusMsg(i18n(ready));
}

void MainWindow::slotFileExport(int format_) {
  slotStatusMsg(i18n("Exporting data..."));
  
  ExportDialog dlg(static_cast<ExportDialog::ExportFormat>(format_),
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

    // if the file has no extension, add it automatically, based on the current filter
    if(url.fileName().contains('.') == 0) {
      QString cf = fileDlg.currentFilter();
      if(cf.length() > 1 && cf[1] == '.') {
        url.setFileName(url.fileName() + cf.mid(1));
      }
    }

    if(!url.isEmpty()) {
      kapp->setOverrideCursor(Qt::waitCursor);
      if(dlg.isText()) {
        FileHandler::writeTextURL(url, dlg.text(), !dlg.encodeUTF8());
//          kdDebug() << text << endl;
      } else {
        FileHandler::writeDataURL(url, dlg.data());
      }
      kapp->restoreOverrideCursor();
    }
  }

  slotStatusMsg(i18n(ready));
}

void MainWindow::slotShowStringMacroDialog() {
  if(m_doc->collection()->collectionType() != Data::Collection::Bibtex) {
    return;
  }

  if(!m_stringMacroDlg) {
    const Data::BibtexCollection* c = static_cast<Data::BibtexCollection*>(m_doc->collection());
    m_stringMacroDlg = new StringMapDialog(c->macroList(), this, "StringMacroDialog", false);
    m_stringMacroDlg->setCaption(i18n("String Macros"));
    m_stringMacroDlg->setLabels(i18n("Macro"), i18n("String"));
    connect(m_stringMacroDlg, SIGNAL(finished()), SLOT(slotHideStringMacroDialog()));
    connect(m_stringMacroDlg, SIGNAL(okClicked()), SLOT(slotStringMacroDialogOk()));
  } else {
#if KDE_IS_VERSION(3,1,90)
    KWin::activateWindow(m_stringMacroDlg->winId());
#else
    KWin::setActiveWindow(m_stringMacroDlg->winId());
#endif
  }
  m_stringMacroDlg->show();
}

void MainWindow::slotHideStringMacroDialog() {
  if(m_stringMacroDlg) {
    m_stringMacroDlg->delayedDestruct();
    m_stringMacroDlg = 0;
  }
}

void MainWindow::slotStringMacroDialogOk() {
  // no point in checking if collection is bibtex, as dialog would never have been created
  if(m_stringMacroDlg) {
    static_cast<Data::BibtexCollection*>(m_doc->collection())->setMacroList(m_stringMacroDlg->stringMap());
    m_doc->slotSetModified(true);
  }
}

QStringList MainWindow::groupBy() const {
  QStringList g = m_groupView->groupBy();
  // special case for pseudo-group
  if(g[0] == QString::fromLatin1("_people")) {
    g.clear();
    for(Data::FieldListIterator it(m_doc->collection()->peopleFields()); it.current(); ++it) {
      g << it.current()->name();
    }
  }
  return g;
}

QStringList MainWindow::sortTitles() const {
  QStringList list;
  list << m_detailedView->sortColumnTitle1();
  list << m_detailedView->sortColumnTitle2();
  list << m_detailedView->sortColumnTitle3();
  return list;
}

QStringList MainWindow::visibleColumns() const {
  return m_detailedView->visibleColumns();
}

void MainWindow::slotNewEntry() {
  m_toggleEntryEditor->setChecked(true);
  slotToggleEntryEditor();
  m_editDialog->slotHandleNew();
}

void MainWindow::slotEditDialogFinished() {
  m_toggleEntryEditor->setChecked(false);
}

void MainWindow::slotShowEntryEditor(QListViewItem* item_) {
  if(item_ && !dynamic_cast<Bookcase::EntryItem*>(item_)) {
    return;
  }

  m_toggleEntryEditor->setChecked(true);
  m_editDialog->show();

#if KDE_IS_VERSION(3,1,90)
  KWin::activateWindow(m_editDialog->winId());
#else
  KWin::setActiveWindow(m_editDialog->winId());
#endif
}

void MainWindow::slotUpdateToolbarIcons() {
/*  QPixmap newPix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("filenew"), KIcon::Toolbar);
  QImage newImg = newPix.convertToImage();
  QPixmap entryPix = KGlobal::iconLoader()->loadIcon(m_doc->collection()->entryName(), KIcon::User);
  QImage entryImg = entryPix.convertToImage();
  newImg.scale(QMAX(newImg.width(), entryImg.width()), QMAX(newImg.height(), entryImg.height()));
  entryImg.scale(QMAX(newImg.width(), entryImg.width()), QMAX(newImg.height(), entryImg.height()));
  KIconEffect::overlay(newImg, entryImg);
  entryPix.convertFromImage(newImg, 0);
  m_editNewEntry->setIconSet(QIconSet(entryPix));*/
}
