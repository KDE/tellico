/***************************************************************************
    copyright            : (C) 2001-2006 by Robby Stephenson
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
#include "tellico_kernel.h"
#include "document.h"
#include "detailedlistview.h"
#include "entryeditdialog.h"
#include "groupview.h"
#include "viewstack.h"
#include "collection.h"
#include "entry.h"
#include "configdialog.h"
#include "entryitem.h"
#include "filter.h"
#include "filterdialog.h"
#include "collectionfieldsdialog.h"
#include "controller.h"
#include "importdialog.h"
#include "exportdialog.h"
#include "filehandler.h" // needed so static mainWindow variable can be set
#include "gui/stringmapdialog.h"
#include "translators/htmlexporter.h" // for printing
#include "entryview.h"
#include "entryiconview.h"
#include "imagefactory.h" // needed so tmp files can get cleaned
#include "collections/bibtexcollection.h" // needed for bibtex string macro dialog
#include "translators/bibtexhandler.h" // needed for bibtex options
#include "fetchdialog.h"
#include "reportdialog.h"
#include "tellico_strings.h"
#include "filterview.h"
#include "loanview.h"
#include "gui/tabcontrol.h"
#include "gui/lineedit.h"
#include "tellico_utils.h"
#include "tellico_debug.h"
#include "entryiconfactory.h"
#include "statusbar.h"
#include "fetch/fetchmanager.h"
#include "cite/actionmanager.h"
#include "core/tellico_config.h"
#include "core/drophandler.h"
#include "latin1literal.h"

#include <kapplication.h>
#include <kcombobox.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstdaction.h>
#include <kwin.h>
#include <kprogress.h>
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
#include <dcopclient.h>
#include <kaction.h>

#include <qsplitter.h>
//#include <qpainter.h>
#include <qsignalmapper.h>
#include <qtimer.h>
#include <qmetaobject.h> // needed for copy, cut, paste slots
#include <qwhatsthis.h>
#include <qvbox.h>

namespace {
  static const int MAIN_WINDOW_MIN_WIDTH = 600;
  //static const int PRINTED_PAGE_OVERLAP = 0;
  static const int MAX_IMAGES_WARN_PERFORMANCE = 200;
}

using Tellico::MainWindow;

MainWindow::MainWindow(QWidget* parent_/*=0*/, const char* name_/*=0*/) : KMainWindow(parent_, name_),
    ApplicationInterface(),
    m_updateAll(0),
    m_statusBar(0),
    m_editDialog(0),
    m_groupView(0),
    m_filterView(0),
    m_loanView(0),
    m_configDlg(0),
    m_filterDlg(0),
    m_collFieldsDlg(0),
    m_stringMacroDlg(0),
    m_fetchDlg(0),
    m_reportDlg(0),
    m_queuedFilters(0),
    m_initialized(false),
    m_newDocument(true) {

  if(!kapp->dcopClient()->isRegistered()) {
    kapp->dcopClient()->registerAs("tellico");
    kapp->dcopClient()->setDefaultObject(objId());
  }

  m_fetchActions.setAutoDelete(true); // these are the fetcher actions

  Controller::init(this); // the only time this is ever called!
  // has to be after controller init
  Kernel::init(this); // the only time this is ever called!

  setIcon(DesktopIcon(QString::fromLatin1("tellico")));

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
  // So don't make any connections, don't read options for it until initFileOpen
  readOptions();

  setAcceptDrops(true);
  DropHandler* drophandler = new DropHandler(this);
  installEventFilter(drophandler);

  MARK_LINE;
  QTimer::singleShot(0, this, SLOT(slotInit()));
}

void MainWindow::slotInit() {
  MARK;
  if(m_editDialog) {
    return;
  }
  m_editDialog = new EntryEditDialog(this, "editdialog");
  Controller::self()->addObserver(m_editDialog);

  m_toggleEntryEditor->setChecked(Config::showEditWidget());
  slotToggleEntryEditor();

  initConnections();
  ImageFactory::init();

  // disable OOO menu item if library is not available
  action("cite_openoffice")->setEnabled(Cite::ActionManager::isEnabled(Cite::CiteOpenOffice));
}

void MainWindow::initStatusBar() {
  MARK;
  m_statusBar = new Tellico::StatusBar(this);
}

void MainWindow::initActions() {
  MARK;
  /*************************************************
   * File->New menu
   *************************************************/
  QSignalMapper* collectionMapper = new QSignalMapper(this);
  connect(collectionMapper, SIGNAL(mapped(int)),
          this, SLOT(slotFileNew(int)));

  KActionMenu* fileNewMenu = new KActionMenu(actionCollection(), "file_new_collection");
  fileNewMenu->setText(i18n("New"));
  fileNewMenu->setIconSet(BarIconSet(QString::fromLatin1("filenew"))); // doesn't work
  fileNewMenu->setIconSet(BarIcon(QString::fromLatin1("filenew")));
  fileNewMenu->setToolTip(i18n("Create a new collection"));
  fileNewMenu->setDelayed(false);

  KAction* action = new KAction(actionCollection(), "new_book_collection");
  action->setText(i18n("New &Book Collection"));
  action->setIconSet(UserIconSet(QString::fromLatin1("book")));
  action->setToolTip(i18n("Create a new book collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::Book);

  action = new KAction(actionCollection(), "new_bibtex_collection");
  action->setText(i18n("New B&ibliography"));
  action->setIconSet(UserIconSet(QString::fromLatin1("bibtex")));
  action->setToolTip(i18n("Create a new bibtex bibliography"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::Bibtex);

  action = new KAction(actionCollection(), "new_comic_book_collection");
  action->setText(i18n("New &Comic Book Collection"));
  action->setIconSet(UserIconSet(QString::fromLatin1("comic")));
  action->setToolTip(i18n("Create a new comic book collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::ComicBook);

  action = new KAction(actionCollection(), "new_video_collection");
  action->setText(i18n("New &Video Collection"));
  action->setIconSet(UserIconSet(QString::fromLatin1("video")));
  action->setToolTip(i18n("Create a new video collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::Video);

  action = new KAction(actionCollection(), "new_music_collection");
  action->setText(i18n("New &Music Collection"));
  action->setIconSet(UserIconSet(QString::fromLatin1("album")));
  action->setToolTip(i18n("Create a new music collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::Album);

  action = new KAction(actionCollection(), "new_coin_collection");
  action->setText(i18n("New C&oin Collection"));
  action->setIconSet(UserIconSet(QString::fromLatin1("coin")));
  action->setToolTip(i18n("Create a new coin collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::Coin);

  action = new KAction(actionCollection(), "new_stamp_collection");
  action->setText(i18n("New &Stamp Collection"));
  action->setIconSet(UserIconSet(QString::fromLatin1("stamp")));
  action->setToolTip(i18n("Create a new stamp collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::Stamp);

  action = new KAction(actionCollection(), "new_card_collection");
  action->setText(i18n("New C&ard Collection"));
  action->setIconSet(UserIconSet(QString::fromLatin1("card")));
  action->setToolTip(i18n("Create a new trading card collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::Card);

  action = new KAction(actionCollection(), "new_wine_collection");
  action->setText(i18n("New &Wine Collection"));
  action->setIconSet(UserIconSet(QString::fromLatin1("wine")));
  action->setToolTip(i18n("Create a new wine collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::Wine);

  action = new KAction(actionCollection(), "new_game_collection");
  action->setText(i18n("New &Game Collection"));
  action->setIconSet(UserIconSet(QString::fromLatin1("game")));
  action->setToolTip(i18n("Create a new game collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));

  collectionMapper->setMapping(action, Data::Collection::Game);
  action = new KAction(actionCollection(), "new_boardgame_collection");
  action->setText(i18n("New Boa&rd Game Collection"));
  action->setIconSet(UserIconSet(QString::fromLatin1("boardgame")));
  action->setToolTip(i18n("Create a new board game collection"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::BoardGame);

  action = new KAction(actionCollection(), "new_file_catalog");
  action->setText(i18n("New &File Catalog"));
  action->setIconSet(UserIconSet(QString::fromLatin1("file")));
  action->setToolTip(i18n("Create a new file catalog"));
  fileNewMenu->insert(action);
  connect(action, SIGNAL(activated()), collectionMapper, SLOT(map()));
  collectionMapper->setMapping(action, Data::Collection::File);

  action = new KAction(actionCollection(), "new_custom_collection");
  action->setText(i18n("New C&ustom Collection"));
  action->setIconSet(UserIconSet(QString::fromLatin1("filenew")));
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
  action = KStdAction::saveAs(this, SLOT(slotFileSaveAs()), actionCollection());
  action->setToolTip(i18n("Save the document as a different file..."));
  action = KStdAction::print(this, SLOT(slotFilePrint()), actionCollection());
  action->setToolTip(i18n("Print the contents of the document..."));
  action = KStdAction::quit(this, SLOT(slotFileQuit()), actionCollection());
  action->setToolTip(i18n("Quit the application"));

/**************** Import Menu ***************************/

  QSignalMapper* importMapper = new QSignalMapper(this);
  connect(importMapper, SIGNAL(mapped(int)),
          this, SLOT(slotFileImport(int)));

  KActionMenu* importMenu = new KActionMenu(actionCollection(), "file_import");
  importMenu->setText(i18n("&Import"));
  importMenu->setIconSet(BarIconSet(QString::fromLatin1("fileimport")));
  importMenu->setToolTip(i18n("Import collection data from other formats"));
  importMenu->setDelayed(false);

  action = new KAction(actionCollection(), "file_import_tellico");
  action->setText(i18n("Import Tellico Data..."));
  action->setToolTip(i18n("Import another Tellico data file"));
  importMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, Import::TellicoXML);

  action = new KAction(actionCollection(), "file_import_csv");
  action->setText(i18n("Import CSV Data..."));
  action->setToolTip(i18n("Import a CSV file"));
  importMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, Import::CSV);

  action = new KAction(actionCollection(), "file_import_mods");
  action->setText(i18n("Import MODS Data..."));
  action->setToolTip(i18n("Import a MODS data file"));
  importMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, Import::MODS);

  action = new KAction(actionCollection(), "file_import_alexandria");
  action->setText(i18n("Import Alexandria Data..."));
  action->setToolTip(i18n("Import data from the Alexandria book collection manager"));
  importMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, Import::Alexandria);

  action = new KAction(actionCollection(), "file_import_delicious");
  action->setText(i18n("Import Delicious Library Data..."));
  action->setToolTip(i18n("Import data from Delicious Library"));
  importMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, Import::Delicious);

  action = new KAction(actionCollection(), "file_import_referencer");
  action->setText(i18n("Import Referencer Data..."));
  action->setToolTip(i18n("Import data from Referencer"));
  importMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, Import::Referencer);

  action = new KAction(actionCollection(), "file_import_bibtex");
  action->setText(i18n("Import Bibtex Data..."));
  action->setToolTip(i18n("Import a bibtex bibliography file"));
  importMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, Import::Bibtex);

  action = new KAction(actionCollection(), "file_import_bibtexml");
  action->setText(i18n("Import Bibtexml Data..."));
  action->setToolTip(i18n("Import a Bibtexml bibliography file"));
  importMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, Import::Bibtexml);

  action = new KAction(actionCollection(), "file_import_ris");
  action->setText(i18n("Import RIS Data..."));
  action->setToolTip(i18n("Import an RIS reference file"));
  importMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, Import::RIS);

  action = new KAction(actionCollection(), "file_import_pdf");
  action->setText(i18n("Import PDF File..."));
  action->setToolTip(i18n("Import a PDF file"));
  importMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, Import::PDF);

  action = new KAction(actionCollection(), "file_import_audiofile");
  action->setText(i18n("Import Audio File Metadata..."));
  action->setToolTip(i18n("Import meta-data from audio files"));
  importMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, Import::AudioFile);
#if !HAVE_TAGLIB
  action->setEnabled(false);
#endif

  action = new KAction(actionCollection(), "file_import_freedb");
  action->setText(i18n("Import Audio CD Data..."));
  action->setToolTip(i18n("Import audio CD information"));
  importMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, Import::FreeDB);
#if !HAVE_KCDDB
  action->setEnabled(false);
#endif

  action = new KAction(actionCollection(), "file_import_gcfilms");
  action->setText(i18n("Import GCstar Data..."));
  action->setToolTip(i18n("Import a GCstar data file"));
  importMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, Import::GCfilms);

  action = new KAction(actionCollection(), "file_import_griffith");
  action->setText(i18n("Import Griffith Data..."));
  action->setToolTip(i18n("Import a Griffith database"));
  importMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, Import::Griffith);

  action = new KAction(actionCollection(), "file_import_amc");
  action->setText(i18n("Import Ant Movie Catalog Data..."));
  action->setToolTip(i18n("Import an Ant Movie Catalog data file"));
  importMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, Import::AMC);

  action = new KAction(actionCollection(), "file_import_filelisting");
  action->setText(i18n("Import File Listing..."));
  action->setToolTip(i18n("Import information about files in a folder"));
  importMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, Import::FileListing);

  action = new KAction(actionCollection(), "file_import_xslt");
  action->setText(i18n("Import XSL Transform..."));
  action->setToolTip(i18n("Import using an XSL Transform"));
  importMenu->insert(action);
  connect(action, SIGNAL(activated()), importMapper, SLOT(map()));
  importMapper->setMapping(action, Import::XSLT);

/**************** Export Menu ***************************/

  QSignalMapper* exportMapper = new QSignalMapper(this);
  connect(exportMapper, SIGNAL(mapped(int)),
          this, SLOT(slotFileExport(int)));

  KActionMenu* exportMenu = new KActionMenu(actionCollection(), "file_export");
  exportMenu->setText(i18n("&Export"));
  exportMenu->setIconSet(BarIconSet(QString::fromLatin1("fileexport")));
  exportMenu->setToolTip(i18n("Export the collection data to other formats"));
  exportMenu->setDelayed(false);

  action = new KAction(actionCollection(), "file_export_xml");
  action->setText(i18n("Export to XML..."));
  action->setToolTip(i18n("Export to a Tellico XML file"));
  exportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, Export::TellicoXML);

  action = new KAction(actionCollection(), "file_export_zip");
  action->setText(i18n("Export to Zip..."));
  action->setToolTip(i18n("Export to a Tellico Zip file"));
  exportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, Export::TellicoZip);

  action = new KAction(actionCollection(), "file_export_html");
  action->setText(i18n("Export to HTML..."));
  action->setToolTip(i18n("Export to an HTML file"));
  exportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, Export::HTML);

  action = new KAction(actionCollection(), "file_export_csv");
  action->setText(i18n("Export to CSV..."));
  action->setToolTip(i18n("Export to a comma-separated values file"));
  exportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, Export::CSV);

  action = new KAction(actionCollection(), "file_export_pilotdb");
  action->setText(i18n("Export to PilotDB..."));
  action->setToolTip(i18n("Export to a PilotDB database"));
  exportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, Export::PilotDB);

  action = new KAction(actionCollection(), "file_export_alexandria");
  action->setText(i18n("Export to Alexandria..."));
  action->setToolTip(i18n("Export to an Alexandria library"));
  exportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, Export::Alexandria);

  action = new KAction(actionCollection(), "file_export_bibtex");
  action->setText(i18n("Export to Bibtex..."));
  action->setToolTip(i18n("Export to a bibtex file"));
  exportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, Export::Bibtex);

  action = new KAction(actionCollection(), "file_export_bibtexml");
  action->setText(i18n("Export to Bibtexml..."));
  action->setToolTip(i18n("Export to a Bibtexml file"));
  exportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, Export::Bibtexml);

  action = new KAction(actionCollection(), "file_export_onix");
  action->setText(i18n("Export to ONIX..."));
  action->setToolTip(i18n("Export to an ONIX file"));
  exportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, Export::ONIX);

  action = new KAction(actionCollection(), "file_export_gcfilms");
  action->setText(i18n("Export to GCfilms..."));
  action->setToolTip(i18n("Export to a GCfilms data file"));
  exportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, Export::GCfilms);

#if 0
    QString dummy1 = i18n("Export to GCstar...");
    QString dummy2 = i18n("Export to a GCstar data file");
#endif

  action = new KAction(actionCollection(), "file_export_xslt");
  action->setText(i18n("Export XSL Transform..."));
  action->setToolTip(i18n("Export using an XSL Transform"));
  exportMenu->insert(action);
  connect(action, SIGNAL(activated()), exportMapper, SLOT(map()));
  exportMapper->setMapping(action, Export::XSLT);

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

  action = new KAction(i18n("Internet Search..."), QString::fromLatin1("wizard"), CTRL + Key_M,
                       this, SLOT(slotShowFetchDialog()),
                       actionCollection(), "edit_search_internet");
  action->setToolTip(i18n("Search the internet..."));

  action = new KAction(i18n("Advanced &Filter..."), QString::fromLatin1("filter"), CTRL + Key_J,
                       this, SLOT(slotShowFilterDialog()),
                       actionCollection(), "filter_dialog");
  action->setToolTip(i18n("Filter the collection"));

  /*************************************************
   * Collection menu
   *************************************************/
  m_newEntry = new KAction(i18n("&New Entry..."), QString::fromLatin1("filenew"), CTRL + Key_N,
                               this, SLOT(slotNewEntry()),
                               actionCollection(), "coll_new_entry");
  m_newEntry->setToolTip(i18n("Create a new entry"));
  m_editEntry = new KAction(i18n("&Edit Entry..."), QString::fromLatin1("edit"), CTRL + Key_E,
                            this, SLOT(slotShowEntryEditor()),
                            actionCollection(), "coll_edit_entry");
  m_editEntry->setToolTip(i18n("Edit the selected entries"));
  m_copyEntry = new KAction(i18n("D&uplicate Entry"), QString::fromLatin1("editcopy"), CTRL + Key_Y,
                            Controller::self(), SLOT(slotCopySelectedEntries()),
                            actionCollection(), "coll_copy_entry");
  m_copyEntry->setToolTip(i18n("Copy the selected entries"));
  m_deleteEntry = new KAction(i18n("&Delete Entry"), QString::fromLatin1("editdelete"), CTRL + Key_D,
                              Controller::self(), SLOT(slotDeleteSelectedEntries()),
                              actionCollection(), "coll_delete_entry");
  m_deleteEntry->setToolTip(i18n("Delete the selected entries"));
  m_mergeEntry = new KAction(i18n("&Merge Entries"), QString::fromLatin1("editmerge"), CTRL + Key_G,
                              Controller::self(), SLOT(slotMergeSelectedEntries()),
                              actionCollection(), "coll_merge_entry");
  m_mergeEntry->setToolTip(i18n("Merge the selected entries"));
  m_mergeEntry->setEnabled(false); // gets enabled when more than 1 entry is selected

  action = new KAction(i18n("&Generate Reports..."), QString::fromLatin1("document"), 0, this,
                       SLOT(slotShowReportDialog()),
                       actionCollection(), "coll_reports");
  action->setToolTip(i18n("Generate collection reports"));
  m_checkOutEntry = new KAction(i18n("Check-&out..."), QString::fromLatin1("2uparrow"), 0,
                                Controller::self(),  SLOT(slotCheckOut()),
                                actionCollection(), "coll_checkout");
  m_checkOutEntry->setToolTip(i18n("Check-out the selected items"));
  m_checkInEntry = new KAction(i18n("Check-&in"), QString::fromLatin1("2downarrow"), 0,
                               Controller::self(),  SLOT(slotCheckIn()),
                               actionCollection(), "coll_checkin");
  m_checkInEntry->setToolTip(i18n("Check-in the selected items"));

  action = new KAction(i18n("&Rename Collection..."), QString::fromLatin1("editclear"), CTRL + Key_R,
                       this, SLOT(slotRenameCollection()),
                       actionCollection(), "coll_rename_collection");
  action->setToolTip(i18n("Rename the collection"));
  action = new KAction(i18n("Collection &Fields..."), QString::fromLatin1("edit"), CTRL + Key_U,
                       this, SLOT(slotShowCollectionFieldsDialog()),
                       actionCollection(), "coll_fields");
  action->setToolTip(i18n("Modify the collection fields"));
  action = new KAction(i18n("Convert to &Bibliography"), 0,
                       this, SLOT(slotConvertToBibliography()),
                       actionCollection(), "coll_convert_bibliography");
  action->setToolTip(i18n("Convert a book collection to a bibliography"));
  action = new KAction(i18n("String &Macros..."), 0,
                       this, SLOT(slotShowStringMacroDialog()),
                       actionCollection(), "coll_string_macros");
  action->setToolTip(i18n("Edit the bibtex string macros"));

  QSignalMapper* citeMapper = new QSignalMapper(this);
  connect(citeMapper, SIGNAL(mapped(int)),
          this, SLOT(slotCiteEntry(int)));

  action = new KAction(actionCollection(), "cite_clipboard");
  action->setText(i18n("Copy Bibtex to Cli&pboard"));
  action->setToolTip(i18n("Copy bibtex citations to the clipboard"));
  connect(action, SIGNAL(activated()), citeMapper, SLOT(map()));
  citeMapper->setMapping(action, Cite::CiteClipboard);

  action = new KAction(actionCollection(), "cite_lyxpipe");
  action->setText(i18n("Cite Entry in &LyX"));
  action->setToolTip(i18n("Cite the selected entries in LyX"));
  connect(action, SIGNAL(activated()), citeMapper, SLOT(map()));
  citeMapper->setMapping(action, Cite::CiteLyxpipe);

  action = new KAction(actionCollection(), "cite_openoffice");
  action->setText(i18n("Ci&te Entry in OpenOffice.org"));
  action->setToolTip(i18n("Cite the selected entries in OpenOffice.org"));
  connect(action, SIGNAL(activated()), citeMapper, SLOT(map()));
  citeMapper->setMapping(action, Cite::CiteOpenOffice);

  QSignalMapper* updateMapper = new QSignalMapper(this, "update_mapper");
  connect(updateMapper, SIGNAL(mapped(const QString&)),
          Controller::self(), SLOT(slotUpdateSelectedEntries(const QString&)));

  m_updateEntryMenu = new KActionMenu(i18n("&Update Entry"), actionCollection(), "coll_update_entry");
//  m_updateEntryMenu->setIconSet(BarIconSet(QString::fromLatin1("fileexport")));
  m_updateEntryMenu->setDelayed(false);

  m_updateAll = new KAction(actionCollection(), "update_entry_all");
  m_updateAll->setText(i18n("All Sources"));
  m_updateAll->setToolTip(i18n("Update entry data from all available sources"));
//  m_updateEntryMenu->insert(action);
  connect(m_updateAll, SIGNAL(activated()), updateMapper, SLOT(map()));
  updateMapper->setMapping(m_updateAll, QString::fromLatin1("_all"));

  /*************************************************
   * Settings menu
   *************************************************/
  setStandardToolBarMenuEnabled(true);
  createStandardStatusBarAction();
  KStdAction::configureToolbars(this, SLOT(slotConfigToolbar()), actionCollection());
  KStdAction::keyBindings(this, SLOT(slotConfigKeys()), actionCollection());
  m_toggleGroupWidget = new KToggleAction(i18n("Show Grou&p View"), 0,
                                          this, SLOT(slotToggleGroupWidget()),
                                          actionCollection(), "toggle_group_widget");
  m_toggleGroupWidget->setToolTip(i18n("Enable/disable the group view"));
  m_toggleGroupWidget->setCheckedState(i18n("Hide Grou&p View"));

  m_toggleEntryEditor = new KToggleAction(i18n("Show Entry &Editor"), 0,
                                         this, SLOT(slotToggleEntryEditor()),
                                         actionCollection(), "toggle_edit_widget");
  m_toggleEntryEditor->setToolTip(i18n("Enable/disable the editor"));
  m_toggleEntryEditor->setCheckedState(i18n("Hide Entry &Editor"));

  m_toggleEntryView = new KToggleAction(i18n("Show Entry &View"), 0,
                                        this, SLOT(slotToggleEntryView()),
                                        actionCollection(), "toggle_entry_view");
  m_toggleEntryView->setToolTip(i18n("Enable/disable the entry view"));
  m_toggleEntryView->setCheckedState(i18n("Hide Entry &View"));

  KStdAction::preferences(this, SLOT(slotShowConfigDialog()), actionCollection());

  /*************************************************
   * Help menu
   *************************************************/
  KStdAction::tipOfDay(this, SLOT(slotShowTipOfDay()), actionCollection(), "tipOfDay");

  /*************************************************
   * Collection Toolbar
   *************************************************/
  (void) new KAction(i18n("Change Grouping"), CTRL + Key_G,
                     this, SLOT(slotGroupLabelActivated()),
                     actionCollection(), "change_entry_grouping_accel");

  m_entryGrouping = new KSelectAction(i18n("&Group Selection"), 0, this,
                                      SLOT(slotChangeGrouping()),
                                      actionCollection(), "change_entry_grouping");
  m_entryGrouping->setToolTip(i18n("Change the grouping of the collection"));

  (void) new KAction(i18n("Filter"), CTRL + Key_F,
                     this, SLOT(slotFilterLabelActivated()),
                     actionCollection(), "quick_filter_accel");
  (void) new KAction(i18n("Clear Filter"), QString::fromLatin1("locationbar_erase"), 0,
                     this, SLOT(slotClearFilter()),
                     actionCollection(), "quick_filter_clear");

  m_quickFilter = new GUI::LineEdit();
  m_quickFilter->setHint(i18n("Filter here...")); // same text as kdepim and amarok
  // about 10 characters wide
  m_quickFilter->setFixedWidth(m_quickFilter->fontMetrics().maxWidth()*10);
  // want to update every time the filter text changes
  connect(m_quickFilter, SIGNAL(textChanged(const QString&)),
          this, SLOT(slotQueueFilter()));

  KWidgetAction* wAction = new KWidgetAction(m_quickFilter, i18n("Filter"), 0, 0, 0,
                                             actionCollection(), "quick_filter");
  wAction->setToolTip(i18n("Filter the collection"));
  wAction->setShortcutConfigurable(false);
  wAction->setAutoSized(true);

  // show tool tips in status bar
  actionCollection()->setHighlightingEnabled(true);
  connect(actionCollection(), SIGNAL(actionStatusText(const QString &)),
          SLOT(slotStatusMsg(const QString &)));
  connect(actionCollection(), SIGNAL(clearStatusText()),
          SLOT(slotClearStatus()));

#ifdef UIFILE
  kdWarning() << "MainWindow::initActions() - change createGUI() call!" << endl;
  createGUI(UIFILE, false);
#else
  createGUI(QString::null, false);
#endif
}

void MainWindow::initDocument() {
  MARK;
  Data::Document* doc = Data::Document::self();

  KConfigGroup config(KGlobal::config(), "General Options");
  doc->setLoadAllImages(config.readBoolEntry("Load All Images", false));

  // allow status messages from the document
  connect(doc, SIGNAL(signalStatusMsg(const QString&)),
          SLOT(slotStatusMsg(const QString&)));

  // do stuff that changes when the doc is modified
  connect(doc, SIGNAL(signalModified(bool)),
          SLOT(slotEnableModifiedActions(bool)));

  connect(Kernel::self()->commandHistory(), SIGNAL(commandExecuted()),
          doc, SLOT(slotSetModified()));
  connect(Kernel::self()->commandHistory(), SIGNAL(documentRestored()),
          doc, SLOT(slotDocumentRestored()));
}

void MainWindow::initView() {
  MARK;
  m_split = new QSplitter(Qt::Horizontal, this);
  setCentralWidget(m_split);

  m_viewTabs = new GUI::TabControl(m_split);
  m_viewTabs->setTabBarHidden(true);
  m_groupView = new GroupView(m_viewTabs, "groupview");
  Controller::self()->addObserver(m_groupView);
  m_viewTabs->addTab(m_groupView, SmallIcon(QString::fromLatin1("folder")), i18n("Groups"));
  QWhatsThis::add(m_groupView, i18n("<qt>The <i>Group View</i> sorts the entries into groupings "
                                    "based on a selected field.</qt>"));

  m_rightSplit = new QSplitter(Qt::Vertical, m_split);

  m_detailedView = new DetailedListView(m_rightSplit, "detailedlistview");
  Controller::self()->addObserver(m_detailedView);
  QWhatsThis::add(m_detailedView, i18n("<qt>The <i>Column View</i> shows the value of multiple fields "
                                       "for each entry.</qt>"));
  connect(Data::Document::self(), SIGNAL(signalCollectionImagesLoaded(Tellico::Data::CollPtr)),
          m_detailedView, SLOT(slotRefreshImages()));

  m_viewStack = new ViewStack(m_rightSplit, "viewstack");
  Controller::self()->addObserver(m_viewStack->iconView());
  connect(m_viewStack->entryView(), SIGNAL(signalAction(const KURL&)),
          SLOT(slotURLAction(const KURL&)));

  setMinimumWidth(MAIN_WINDOW_MIN_WIDTH);
}

void MainWindow::initConnections() {
  // have to toggle the menu item if the dialog gets closed
  connect(m_editDialog, SIGNAL(finished()),
          this, SLOT(slotEditDialogFinished()));

  // let the group view call filters, too
  connect(m_groupView, SIGNAL(signalUpdateFilter(Tellico::FilterPtr)),
          Controller::self(), SLOT(slotUpdateFilter(Tellico::FilterPtr)));
}

void MainWindow::initFileOpen(bool nofile_) {
  MARK;
  slotInit();
  // check to see if most recent file should be opened
  bool happyStart = false;
  if(!nofile_ && Config::reopenLastFile()) {
    // Config::lastOpenFile() is the full URL, protocol included
    KURL lastFile(Config::lastOpenFile()); // empty string is actually ok, it gets handled
    if(!lastFile.isEmpty() && lastFile.isValid()) {
      slotFileOpen(lastFile);
      happyStart = true;
    }
  }
  if(!happyStart) {
    // the document is created with an initial book collection, continue with that
    Controller::self()->slotCollectionAdded(Data::Document::self()->collection());

    m_fileSave->setEnabled(false);
    slotEnableOpenedActions();
    slotEnableModifiedActions(false);

    slotEntryCount();

    const int type = Kernel::self()->collectionType();
    QString welcomeFile = locate("appdata", QString::fromLatin1("welcome.html"));
    QString text = FileHandler::readTextFile(welcomeFile);
    text.replace(QString::fromLatin1("$FGCOLOR$"), Config::templateTextColor(type).name());
    text.replace(QString::fromLatin1("$BGCOLOR$"), Config::templateBaseColor(type).name());
    text.replace(QString::fromLatin1("$COLOR1$"),  Config::templateHighlightedTextColor(type).name());
    text.replace(QString::fromLatin1("$COLOR2$"),  Config::templateHighlightedBaseColor(type).name());
    text.replace(QString::fromLatin1("$IMGDIR$"),  QFile::encodeName(ImageFactory::tempDir()));
    text.replace(QString::fromLatin1("$BANNER$"),
                 i18n("Welcome to the Tellico Collection Manager"));
    text.replace(QString::fromLatin1("$WELCOMETEXT$"),
                 i18n("<h3>Tellico is a tool for managing collections of books, "
                      "videos, music, and whatever else you want to catalog.</h3>"
                      "<h3>New entries can be added to your collection by "
                      "<a href=\"tc:///coll_new_entry\">entering data manually</a> or by "
                      "<a href=\"tc:///edit_search_internet\">downloading data</a> from "
                      "various Internet sources.</h3>"));
    m_viewStack->entryView()->showText(text);
  }
  m_initialized = true;
}

// These are general options.
// The options that can be changed in the "Configuration..." dialog
// are taken care of by the ConfigDialog object.
void MainWindow::saveOptions() {
//  myDebug() << "MainWindow::saveOptions()" << endl;

  saveMainWindowSettings(KGlobal::config(), QString::fromLatin1("Main Window Options"));

  Config::setShowGroupWidget(m_toggleGroupWidget->isChecked());
  Config::setShowEditWidget(m_toggleEntryEditor->isChecked());
  Config::setShowEntryView(m_toggleEntryView->isChecked());

  m_fileOpenRecent->saveEntries(KGlobal::config(), QString::fromLatin1("Recent Files"));
  if(!isNewDocument()) {
    Config::setLastOpenFile(Data::Document::self()->URL().url());
  }

  if(m_groupView->isShown()) {
    Config::setMainSplitterSizes(m_split->sizes());
  }
  if(m_viewStack->isShown()) {
    // badly named option, but no need to change
    Config::setSecondarySplitterSizes(m_rightSplit->sizes());
  }

  Config::setGroupViewSortColumn(m_groupView->sortStyle()); // ok to use SortColumn key, save semantics
  Config::setGroupViewSortAscending(m_groupView->ascendingSort());

  if(m_loanView) {
    Config::setLoanViewSortAscending(m_loanView->sortStyle()); // ok to use SortColumn key, save semantics
    Config::setLoanViewSortAscending(m_loanView->ascendingSort());
  }

  if(m_filterView) {
    Config::setFilterViewSortAscending(m_filterView->sortStyle()); // ok to use SortColumn key, save semantics
    Config::setFilterViewSortAscending(m_filterView->ascendingSort());
  }

  // this is used in the EntryEditDialog constructor, too
  m_editDialog->saveDialogSize(QString::fromLatin1("Edit Dialog Options"));

  saveCollectionOptions(Data::Document::self()->collection());
  Config::writeConfig();
}

void MainWindow::readCollectionOptions(Data::CollPtr coll_) {
  KConfigGroup group(KGlobal::config(), QString::fromLatin1("Options - %1").arg(coll_->typeName()));

  QString defaultGroup = coll_->defaultGroupField();
  QString entryGroup;
  if(coll_->type() != Data::Collection::Base) {
    entryGroup = group.readEntry("Group By", defaultGroup);
  } else {
    KURL url = Kernel::self()->URL();
    for(uint i = 0; i < Config::maxCustomURLSettings(); ++i) {
      KURL u = group.readEntry(QString::fromLatin1("URL_%1").arg(i));
      if(url == u) {
        entryGroup = group.readEntry(QString::fromLatin1("Group By_%1").arg(i), defaultGroup);
        break;
      }
    }
    // fall back to old setting
    if(entryGroup.isEmpty()) {
      entryGroup = group.readEntry("Group By", defaultGroup);
    }
  }
  if(entryGroup.isEmpty() || !coll_->entryGroups().contains(entryGroup)) {
    entryGroup = defaultGroup;
  }
  m_groupView->setGroupField(entryGroup);

  QString entryXSLTFile = Config::templateName(coll_->type());
  if(entryXSLTFile.isEmpty()) {
    entryXSLTFile = QString::fromLatin1("Fancy"); // should never happen, but just in case
  }
  m_viewStack->entryView()->setXSLTFile(entryXSLTFile + QString::fromLatin1(".xsl"));

  // make sure the right combo element is selected
  slotUpdateCollectionToolBar(coll_);
}

void MainWindow::saveCollectionOptions(Data::CollPtr coll_) {
  // don't save initial collection options, or empty collections
  if(!coll_ || coll_->entryCount() == 0 || isNewDocument()) {
    return;
  }

  int configIndex = -1;
  KConfigGroup config(KGlobal::config(), QString::fromLatin1("Options - %1").arg(coll_->typeName()));
  QString groupName;
  if(m_entryGrouping->currentItem() > -1 &&
     static_cast<int>(coll_->entryGroups().count()) > m_entryGrouping->currentItem()) {
    groupName = Kernel::self()->fieldNameByTitle(m_entryGrouping->currentText());
    if(coll_->type() != Data::Collection::Base) {
      config.writeEntry("Group By", groupName);
    }
  }

  if(coll_->type() == Data::Collection::Base) {
    // all of this is to have custom settings on a per file basis
    KURL url = Kernel::self()->URL();
    QValueList<KURL> urls = QValueList<KURL>() << url;
    QStringList groupBys = QStringList() << groupName;
    for(uint i = 0; i < Config::maxCustomURLSettings(); ++i) {
      KURL u = config.readEntry(QString::fromLatin1("URL_%1").arg(i));
      QString g = config.readEntry(QString::fromLatin1("Group By_%1").arg(i));
      if(!u.isEmpty() && url != u) {
        urls.append(u);
        groupBys.append(g);
      } else if(!u.isEmpty()) {
        configIndex = i;
      }
    }
    uint limit = QMIN(urls.count(), Config::maxCustomURLSettings());
    for(uint i = 0; i < limit; ++i) {
      config.writeEntry(QString::fromLatin1("URL_%1").arg(i), urls[i].url());
      config.writeEntry(QString::fromLatin1("Group By_%1").arg(i), groupBys[i]);
    }
  }
  m_detailedView->saveConfig(coll_, configIndex);
}

void MainWindow::readOptions() {
//  myDebug() << "MainWindow::readOptions()" << endl;

  applyMainWindowSettings(KGlobal::config(), QString::fromLatin1("Main Window Options"));

  QValueList<int> splitList = Config::mainSplitterSizes();
  if(!splitList.empty()) {
    m_split->setSizes(splitList);
  }

  splitList = Config::secondarySplitterSizes();
  if(!splitList.empty()) {
    m_rightSplit->setSizes(splitList);
  }

  m_viewStack->iconView()->setMaxAllowedIconWidth(Config::maxIconSize());

  connect(toolBar("collectionToolBar"), SIGNAL(modechange()), SLOT(slotUpdateToolbarIcons()));

  m_toggleGroupWidget->setChecked(Config::showGroupWidget());
  slotToggleGroupWidget();

  m_toggleEntryView->setChecked(Config::showEntryView());
  slotToggleEntryView();

  // initialize the recent file list
  m_fileOpenRecent->loadEntries(KGlobal::config(), QString::fromLatin1("Recent Files"));

  // sort by count if column = 1
  int sortStyle = Config::groupViewSortColumn();
  m_groupView->setSortStyle(static_cast<GUI::ListView::SortStyle>(sortStyle));
  bool sortAscending = Config::groupViewSortAscending();
  m_groupView->setSortOrder(sortAscending ? Qt::Ascending : Qt::Descending);

  m_detailedView->setPixmapSize(Config::maxPixmapWidth(), Config::maxPixmapHeight());

  bool useBraces = Config::useBraces();
  if(useBraces) {
    BibtexHandler::s_quoteStyle = BibtexHandler::BRACES;
  } else {
    BibtexHandler::s_quoteStyle = BibtexHandler::QUOTES;
  }

  // Don't read any options for the edit dialog here, since it's not yet initialized.
  // Put them in init()
}

void MainWindow::saveProperties(KConfig* cfg_) {
  if(!isNewDocument() && !Data::Document::self()->isModified()) {
    // saving to tempfile not necessary
  } else {
    KURL url = Data::Document::self()->URL();
    cfg_->writeEntry("filename", url.url());
    cfg_->writeEntry("modified", Data::Document::self()->isModified());
    QString tempname = KURL::encode_string(kapp->tempSaveName(url.url()));
    KURL tempurl;
    tempurl.setPath(tempname);
    Data::Document::self()->saveDocument(tempurl);
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
      Data::Document::self()->openDocument(tempurl);
      Data::Document::self()->slotSetModified(true);
      updateCaption(true);
      QFile::remove(tempname);
    }
  } else {
    if(!filename.isEmpty()) {
      KURL url;
      url.setPath(filename);
      Data::Document::self()->openDocument(url);
      updateCaption(false);
    }
  }
}

bool MainWindow::queryClose() {
  // in case we're still loading the images, cancel that
  Data::Document::self()->cancelImageWriting();
  return m_editDialog->queryModified() && Data::Document::self()->saveModified();
}

bool MainWindow::queryExit() {
  FileHandler::clean();
  ImageFactory::clean(true);
  saveOptions();
  return true;
}

void MainWindow::slotFileNew(int type_) {
  slotStatusMsg(i18n("Creating new document..."));

  // close the fields dialog
  slotHideCollectionFieldsDialog();

  if(m_editDialog->queryModified() && Data::Document::self()->saveModified()) {
    // remove filter and loan tabs, they'll get re-added if needed
    if(m_filterView) {
      m_viewTabs->removePage(m_filterView);
      Controller::self()->removeObserver(m_filterView);
      delete m_filterView;
      m_filterView = 0;
    }
    if(m_loanView) {
      m_viewTabs->removePage(m_loanView);
      Controller::self()->removeObserver(m_loanView);
      delete m_loanView;
      m_loanView = 0;
    }
    m_viewTabs->setTabBarHidden(true);
    Data::Document::self()->newDocument(type_);
    m_fileOpenRecent->setCurrentItem(-1);
    slotEnableOpenedActions();
    slotEnableModifiedActions(false);
    m_newDocument = true;
    ImageFactory::clean(false);
  }

  StatusBar::self()->clearStatus();
}

void MainWindow::slotFileOpen() {
  slotStatusMsg(i18n("Opening file..."));

  if(m_editDialog->queryModified() && Data::Document::self()->saveModified()) {
    QString filter = i18n("*.tc *.bc|Tellico Files (*.tc)");
    filter += QString::fromLatin1("\n");
    filter += i18n("*.xml|XML Files (*.xml)");
    filter += QString::fromLatin1("\n");
    filter += i18n("*|All Files");
    // keyword 'open'
    KURL url = KFileDialog::getOpenURL(QString::fromLatin1(":open"), filter,
                                       this, i18n("Open File"));
    if(!url.isEmpty() && url.isValid()) {
      slotFileOpen(url);
    }
  }
  StatusBar::self()->clearStatus();
}

void MainWindow::slotFileOpen(const KURL& url_) {
  slotStatusMsg(i18n("Opening file..."));

  // close the fields dialog
  slotHideCollectionFieldsDialog();

  // there seems to be a race condition at start between slotInit() and initFileOpen()
  // which means the edit dialog might not have been created yet
  if((!m_editDialog || m_editDialog->queryModified()) && Data::Document::self()->saveModified()) {
    if(openURL(url_)) {
      m_fileOpenRecent->addURL(url_);
      m_fileOpenRecent->setCurrentItem(-1);
    }
  }

  StatusBar::self()->clearStatus();
}

void MainWindow::slotFileOpenRecent(const KURL& url_) {
  slotStatusMsg(i18n("Opening file..."));

  // close the fields dialog
  slotHideCollectionFieldsDialog();

  if(m_editDialog->queryModified() && Data::Document::self()->saveModified()) {
    if(!openURL(url_)) {
      m_fileOpenRecent->removeURL(url_);
      m_fileOpenRecent->setCurrentItem(-1);
    }
  } else {
    // the KAction shouldn't be checked now
    m_fileOpenRecent->setCurrentItem(-1);
  }

  StatusBar::self()->clearStatus();
}

void MainWindow::openFile(const QString& file_) {
  KURL url = KURL::fromPathOrURL(file_);
  if(!url.isEmpty() && url.isValid()) {
    slotFileOpen(url);
  }
}

bool MainWindow::openURL(const KURL& url_) {
//  myDebug() <<  "MainWindow::openURL() - " << url_.prettyURL() << endl;

  // try to open document
  GUI::CursorSaver cs(Qt::waitCursor);

  bool success = Data::Document::self()->openDocument(url_);

  if(success) {
    m_quickFilter->clear();
    slotEnableOpenedActions();
    m_newDocument = false;
    slotEnableModifiedActions(Data::Document::self()->isModified()); // doc might add some stuff
  } else if(!m_initialized) {
    // special case on startup when openURL() is called with a command line argument
    // and that URL can't be opened. The window still needs to be initialized
    // the doc object is created with an initial book collection, continue with that
    Controller::self()->slotCollectionAdded(Data::Document::self()->collection());

    m_fileSave->setEnabled(false);
    slotEnableOpenedActions();
    slotEnableModifiedActions(false);

    slotEntryCount();
  }
  // slotFileOpen(URL) gets called when opening files on the command line
  // so go ahead and make sure m_initialized is set.
  m_initialized = true;

  // remove filter and loan tabs, they'll get re-added if needed
  if(m_filterView && m_filterView->childCount() == 0) {
    m_viewTabs->removePage(m_filterView);
    Controller::self()->removeObserver(m_filterView);
    delete m_filterView;
    m_filterView = 0;
  }
  if(m_loanView && m_loanView->childCount() == 0) {
    m_viewTabs->removePage(m_loanView);
    Controller::self()->removeObserver(m_loanView);
    delete m_loanView;
    m_loanView = 0;
  }
  Controller::self()->hideTabs(); // does conditional check

  return success;
}

void MainWindow::slotFileSave() {
  fileSave();
}

bool MainWindow::fileSave() {
  if(!m_editDialog->queryModified()) {
    return false;
  }
  slotStatusMsg(i18n("Saving file..."));

  bool ret = true;
  if(isNewDocument()) {
    ret = fileSaveAs();
  } else {
    // special check: if there are more than 200 images AND the "Write Images In File" config key
    // is not set, then warn user that performance may suffer, and write result
    if(Config::imageLocation() == Config::ImagesInFile &&
       Config::askWriteImagesInFile() &&
       Data::Document::self()->imageCount() > MAX_IMAGES_WARN_PERFORMANCE) {
      QString msg = i18n("<qt><p>You are saving a file with many images, which causes Tellico to "
                         "slow down significantly. Do you want to save the images separately in "
                         "Tellico's data directory to improve performance?</p><p>Your choice can "
                         "always be changed in the configuration dialog.</p></qt>");

      KGuiItem yes(i18n("Save Images Separately"));
      KGuiItem no(i18n("Save Images in File"));

      int res = KMessageBox::warningYesNo(this, msg, QString::null /* caption */, yes, no);
      if(res == KMessageBox::No) {
        Config::setImageLocation(Config::ImagesInAppDir);
      }
      Config::setAskWriteImagesInFile(false);
    }

    GUI::CursorSaver cs(Qt::waitCursor);
    if(Data::Document::self()->saveDocument(Data::Document::self()->URL())) {
      m_newDocument = false;
      updateCaption(false);
      m_fileSave->setEnabled(false);
      m_detailedView->resetEntryStatus();
    } else {
      ret = false;
    }
  }

  StatusBar::self()->clearStatus();
  return ret;
}

void MainWindow::slotFileSaveAs() {
  fileSaveAs();
}

bool MainWindow::fileSaveAs() {
  if(!m_editDialog->queryModified()) {
    return false;
  }

  slotStatusMsg(i18n("Saving file with a new filename..."));

  QString filter = i18n("*.tc *.bc|Tellico Files (*.tc)");
  filter += QChar('\n');
  filter += i18n("*|All Files");

  // keyword 'open'
  KFileDialog dlg(QString::fromLatin1(":open"), filter, this, "filedialog", true);
  dlg.setCaption(i18n("Save As"));
  dlg.setOperationMode(KFileDialog::Saving);

  int result = dlg.exec();
  if(result == QDialog::Rejected) {
    StatusBar::self()->clearStatus();
    return false;
  }

  bool ret = true;
  KURL url = dlg.selectedURL();
  if(!url.isEmpty() && url.isValid()) {
    GUI::CursorSaver cs(Qt::waitCursor);
    if(Data::Document::self()->saveDocument(url)) {
      KRecentDocument::add(url);
      m_fileOpenRecent->addURL(url);
      updateCaption(false);
      m_newDocument = false;
      m_fileSave->setEnabled(false);
      m_detailedView->resetEntryStatus();
    } else {
      ret = false;
    }
  }

  StatusBar::self()->clearStatus();
  return ret;
}

void MainWindow::slotFilePrint() {
  slotStatusMsg(i18n("Printing..."));

  bool printGrouped = Config::printGrouped();
  bool printHeaders = Config::printFieldHeaders();
  int imageWidth = Config::maxImageWidth();
  int imageHeight = Config::maxImageHeight();

  // If the collection is being filtered, warn the user
  if(m_detailedView->filter() != 0) {
    QString str = i18n("The collection is currently being filtered to show a limited subset of "
                       "the entries. Only the visible entries will be printed. Continue?");
    int ret = KMessageBox::warningContinueCancel(this, str, QString::null, KStdGuiItem::print(),
                                                 QString::fromLatin1("WarnPrintVisible"));
    if(ret == KMessageBox::Cancel) {
      StatusBar::self()->clearStatus();
      return;
    }
  }

  GUI::CursorSaver cs(Qt::waitCursor);

  Export::HTMLExporter exporter(Data::Document::self()->collection());
  // only print visible entries
  exporter.setEntries(m_detailedView->visibleEntries());
  exporter.setXSLTFile(QString::fromLatin1("tellico-printing.xsl"));
  exporter.setPrintHeaders(printHeaders);
  exporter.setPrintGrouped(printGrouped);
  exporter.setGroupBy(Controller::self()->expandedGroupBy());
  if(!printGrouped) { // the sort titles are only used if the entries are not grouped
    exporter.setSortTitles(Controller::self()->sortTitles());
  }
  exporter.setColumns(m_detailedView->visibleColumns());
  exporter.setMaxImageSize(imageWidth, imageHeight);

  slotStatusMsg(i18n("Processing document..."));
  if(Config::printFormatted()) {
    exporter.setOptions(Export::ExportUTF8 | Export::ExportFormatted);
  } else {
    exporter.setOptions(Export::ExportUTF8);
  }
  QString html = exporter.text();
  if(html.isEmpty()) {
    XSLTError();
    StatusBar::self()->clearStatus();
    return;
  }

  // don't have busy cursor when showing the print dialog
  cs.restore();
//  myDebug() << html << endl;
  slotStatusMsg(i18n("Printing..."));
  doPrint(html);

  StatusBar::self()->clearStatus();
}

void MainWindow::slotFileQuit() {
  slotStatusMsg(i18n("Exiting..."));

  // this gets called in queryExit() anyway
  //saveOptions();
  close();

  StatusBar::self()->clearStatus();
}

void MainWindow::slotEditCut() {
  activateEditSlot(SLOT(cut()));
}

void MainWindow::slotEditCopy() {
  activateEditSlot(SLOT(copy()));
}

void MainWindow::slotEditPaste() {
  activateEditSlot(SLOT(paste()));
}

void MainWindow::activateEditSlot(const char* slot_) {
  // the edit widget is the only one that copies, cuts, and pastes
  QWidget* w;
  if(m_editDialog->isVisible()) {
    w = m_editDialog->focusWidget();
  } else {
    w = kapp->focusWidget();
  }

  if(w && w->isVisible()) {
    QMetaObject* meta = w->metaObject();

    int idx = meta->findSlot(slot_ + 1, true);
    if(idx > -1) {
      w->qt_invoke(idx, 0);
    }
  }
}

void MainWindow::slotEditSelectAll() {
  m_detailedView->selectAllVisible();
}

void MainWindow::slotEditDeselect() {
  Controller::self()->slotUpdateSelection(0, Data::EntryVec());
}

void MainWindow::slotConfigToolbar() {
  saveMainWindowSettings(KGlobal::config(), QString::fromLatin1("Main Window Options"));
#ifdef UIFILE
  KEditToolbar dlg(actionCollection(), UIFILE);
#else
  KEditToolbar dlg(actionCollection());
#endif
  connect(&dlg, SIGNAL(newToolbarConfig()), this, SLOT(slotNewToolbarConfig()));
  dlg.exec();
}

void MainWindow::slotNewToolbarConfig() {
  applyMainWindowSettings(KGlobal::config(), QString::fromLatin1("Main Window Options"));
#ifdef UIFILE
  createGUI(UIFILE, false);
#else
  createGUI(QString::null, false);
#endif
}

void MainWindow::slotConfigKeys() {
  KKeyDialog::configure(actionCollection());
}

void MainWindow::slotToggleGroupWidget() {
  if(m_toggleGroupWidget->isChecked()) {
    m_viewTabs->show();
  } else {
    m_viewTabs->hide();
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
    m_viewStack->show();
  } else {
    m_viewStack->hide();
  }
}

void MainWindow::slotShowConfigDialog() {
  if(!m_configDlg) {
    m_configDlg = new ConfigDialog(this);
    m_configDlg->show();
    m_configDlg->readConfiguration();
    connect(m_configDlg, SIGNAL(signalConfigChanged()),
            SLOT(slotHandleConfigChange()));
    connect(m_configDlg, SIGNAL(finished()),
            SLOT(slotHideConfigDialog()));
  } else {
    KWin::activateWindow(m_configDlg->winId());
    m_configDlg->show();
  }
}

void MainWindow::slotHideConfigDialog() {
  if(m_configDlg) {
    m_configDlg->delayedDestruct();
    m_configDlg = 0;
  }
}

void MainWindow::slotShowTipOfDay(bool force_/*=true*/) {
  QString tipfile = locate("appdata", QString::fromLatin1("tellico.tips"));
  KTipDialog::showTip(this, tipfile, force_);
}

void MainWindow::slotStatusMsg(const QString& text_) {
  m_statusBar->setStatus(text_);
}

void MainWindow::slotClearStatus() {
  StatusBar::self()->clearStatus();
}

void MainWindow::slotEntryCount() {
  Data::CollPtr coll = Data::Document::self()->collection();
  if(!coll) {
    return;
  }

  int count = coll->entryCount();
  QString text = i18n("Total entries: %1").arg(count);

  int selectCount = Controller::self()->selectedEntries().count();
  int filterCount = m_detailedView->visibleItems();
  // if more than one book is selected, add the number of selected books
  if(filterCount < count && selectCount > 1) {
    text += QChar(' ');
    text += i18n("(%1 filtered; %2 selected)").arg(filterCount).arg(selectCount);
  } else if(filterCount < count) {
    text += QChar(' ');
    text += i18n("(%1 filtered)").arg(filterCount);
  } else if(selectCount > 1) {
    text += QChar(' ');
    text += i18n("(%1 selected)").arg(selectCount);
  }

  m_statusBar->setCount(text);
}

void MainWindow::slotEnableOpenedActions() {
  slotUpdateToolbarIcons();

  // collapse all the groups (depth=1)
  m_groupView->slotCollapseAll(1);

  updateCollectionActions();

  // close the filter dialog when a new collection is opened
  slotHideFilterDialog();
  slotHideStringMacroDialog();
}

void MainWindow::slotEnableModifiedActions(bool modified_ /*= true*/) {
  updateCaption(modified_);
  updateCollectionActions();
  m_fileSave->setEnabled(modified_);
}

void MainWindow::slotHandleConfigChange() {
  const int imageLocation = Config::imageLocation();
  const bool autoCapitalize = Config::autoCapitalization();
  const bool autoFormat = Config::autoFormat();
  QStringList articles = Config::articleList();
  QStringList nocaps = Config::noCapitalizationList();
  QStringList suffixes = Config::nameSuffixList();
  QStringList prefixes = Config::surnamePrefixList();

  m_configDlg->saveConfiguration();

  // only modified if there are entries and image location is changed
  if(imageLocation != Config::imageLocation() && !Data::Document::self()->isEmpty()) {
    Data::Document::self()->slotSetModified();
  }

  if(autoCapitalize != Config::autoCapitalization() ||
    autoFormat != Config::autoFormat() ||
    articles != Config::articleList() ||
    nocaps != Config::noCapitalizationList() ||
    suffixes != Config::nameSuffixList() ||
    prefixes != Config::surnamePrefixList()) {
    // invalidate all groups
    Data::Document::self()->collection()->invalidateGroups();
    // refreshing the title causes the group view to refresh
    Controller::self()->slotRefreshField(Data::Document::self()->collection()->fieldByName(QString::fromLatin1("title")));
  }

  QString entryXSLTFile = Config::templateName(Kernel::self()->collectionType());
  m_viewStack->entryView()->setXSLTFile(entryXSLTFile + QString::fromLatin1(".xsl"));
}

void MainWindow::slotUpdateCollectionToolBar(Data::CollPtr coll_) {
//  myDebug() << "MainWindow::updateCollectionToolBar()" << endl;

  if(!coll_) {
    kdWarning() << "MainWindow::slotUpdateCollectionToolBar() - no collection pointer!" << endl;
    return;
  }

  QString current = m_groupView->groupBy();
  if(current.isEmpty() || !coll_->entryGroups().contains(current)) {
    current = coll_->defaultGroupField();
  }

  const QStringList groups = coll_->entryGroups();
  if(groups.isEmpty()) {
    m_entryGrouping->clear();
    return;
  }

  QMap<QString, QString> groupMap; // use a map so they get sorted
  for(QStringList::ConstIterator groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {
    // special case for people "pseudo-group"
    if(*groupIt == Data::Collection::s_peopleGroupName) {
      groupMap.insert(*groupIt, QString::fromLatin1("<") + i18n("People") + QString::fromLatin1(">"));
    } else {
      groupMap.insert(*groupIt, coll_->fieldTitleByName(*groupIt));
    }
  }

  QStringList names = groupMap.keys();
  int index = names.findIndex(current);
  if(index == -1) {
    current = names[0];
    index = 0;
  }
  QStringList titles = groupMap.values();
  m_entryGrouping->setItems(titles);
  m_entryGrouping->setCurrentItem(index);
  // in case the current grouping field get modified to be non-grouping...
  m_groupView->setGroupField(current); // don't call slotChangeGrouping() since it adds an undo item

  // this isn't really proper, but works so the combo box width gets adjusted
  const int len = m_entryGrouping->containerCount();
  for(int i = 0; i < len; ++i) {
    KToolBar* tb = dynamic_cast<KToolBar*>(m_entryGrouping->container(i));
    if(tb) {
      KComboBox* cb = tb->getCombo(m_entryGrouping->itemId(i));
      if(cb) {
        // qt caches the combobox size and never recalculates the sizeHint()
        // the source code recommends calling setFont to invalidate the sizeHint
        cb->setFont(cb->font());
        cb->updateGeometry();
      }
    }
  }
}

void MainWindow::slotChangeGrouping() {
//  myDebug() << "MainWindow::slotChangeGrouping()" << endl;
  QString title = m_entryGrouping->currentText();

  QString groupName = Kernel::self()->fieldNameByTitle(title);
  if(groupName.isEmpty()) {
    if(title == QString::fromLatin1("<") + i18n("People") + QString::fromLatin1(">")) {
      groupName = Data::Collection::s_peopleGroupName;
    } else {
      groupName = Data::Document::self()->collection()->defaultGroupField();
    }
  }
  m_groupView->setGroupField(groupName);
  m_viewTabs->showPage(m_groupView);
}

void MainWindow::slotShowReportDialog() {
//  myDebug() << "MainWindow::slotShowReport()" << endl;
  if(!m_reportDlg) {
    m_reportDlg = new ReportDialog(this);
    connect(m_reportDlg, SIGNAL(finished()),
            SLOT(slotHideReportDialog()));
  } else {
    KWin::activateWindow(m_reportDlg->winId());
  }
  m_reportDlg->show();
}

void MainWindow::slotHideReportDialog() {
  if(m_reportDlg) {
    m_reportDlg->delayedDestruct();
    m_reportDlg = 0;
  }
}

void MainWindow::doPrint(const QString& html_) {
  KHTMLPart w ;
  w.setJScriptEnabled(false);
  w.setJavaEnabled(false);
  w.setMetaRefreshEnabled(false);
  w.setPluginsEnabled(false);
  w.begin(Data::Document::self()->URL());
  w.write(html_);
  w.end();

// the problem with doing my own layout is that the text gets truncated, both at the
// top and at the bottom. Even adding the overlap parameter, there were problems.
// KHTMLView takes care of that with a truncatedAt() parameter, but that's hidden in
// the khtml::render_root class. So for now, just use the KHTMLView::print() method.
#if 1
  w.view()->print();
#else
  KPrinter* printer = new KPrinter(QPrinter::PrinterResolution);

  if(printer->setup(this, i18n("Print %1").arg(Data::Document::self()->URL().prettyURL()))) {
    printer->setFullPage(false);
    printer->setCreator(QString::fromLatin1("Tellico"));
    printer->setDocName(Data::Document::self()->URL().prettyURL());

    QPainter *p = new QPainter;
    p->begin(printer);

    // mostly taken from KHTMLView::print()
    QString headerLeft = KGlobal::locale()->formatDate(QDate::currentDate(), false);
    QString headerRight = Data::Document::self()->URL().prettyURL();
    QString footerMid;

    QFont headerFont(QString::fromLatin1("helvetica"), 8);
    p->setFont(headerFont);
    const int lspace = p->fontMetrics().lineSpacing();
    const int headerHeight = (lspace * 3) / 2;

    QPaintDeviceMetrics metrics(printer);
    const int pageHeight = metrics.height() - 2*headerHeight;
    const int pageWidth = metrics.width();

//    myDebug() << "MainWindow::doPrint() - pageHeight = " << pageHeight << ""
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
}

void MainWindow::XSLTError() {
  QString str = i18n("Tellico encountered an error in XSLT processing.") + QChar('\n');
  str += i18n("Please check your installation.");
  Kernel::self()->sorry(str);
}

void MainWindow::slotShowFilterDialog() {
  if(!m_filterDlg) {
    m_filterDlg = new FilterDialog(FilterDialog::CreateFilter, this); // allow saving
    m_filterDlg->setFilter(m_detailedView->filter());
    m_quickFilter->setEnabled(false);
    connect(m_filterDlg, SIGNAL(signalCollectionModified()),
            Data::Document::self(), SLOT(slotSetModified()));
    connect(m_filterDlg, SIGNAL(signalUpdateFilter(Tellico::FilterPtr)),
            m_quickFilter, SLOT(clear()));
    connect(m_filterDlg, SIGNAL(signalUpdateFilter(Tellico::FilterPtr)),
            Controller::self(), SLOT(slotUpdateFilter(Tellico::FilterPtr)));
    connect(m_filterDlg, SIGNAL(finished()),
            SLOT(slotHideFilterDialog()));
  } else {
    KWin::activateWindow(m_filterDlg->winId());
  }
  m_filterDlg->show();
}

void MainWindow::slotHideFilterDialog() {
//  m_quickFilter->blockSignals(false);
  m_quickFilter->setEnabled(true);
  if(m_filterDlg) {
    m_filterDlg->delayedDestruct();
    m_filterDlg = 0;
  }
}

void MainWindow::slotQueueFilter() {
  m_queuedFilters++;
  QTimer::singleShot(200, this, SLOT(slotUpdateFilter()));
}

void MainWindow::slotUpdateFilter() {
  m_queuedFilters--;
  if(m_queuedFilters > 0) {
    return;
  }

  setFilter(m_quickFilter->text());
}

void MainWindow::setFilter(const QString& text_) {
  QString text = text_.stripWhiteSpace();
  Filter::Ptr filter = 0;
  if(!text.isEmpty()) {
    filter = new Filter(Filter::MatchAll);
    QString fieldName = QString::null;
    // if the text contains '=' assume it's a field name or title
    if(text.find('=') > -1) {
      fieldName = text.section('=', 0, 0).stripWhiteSpace();
      text = text.section('=', 1).stripWhiteSpace();
      // check that the field name might be a title
      if(!Data::Document::self()->collection()->hasField(fieldName)) {
        fieldName = Data::Document::self()->collection()->fieldNameByTitle(fieldName);
      }
    }
    // if the text contains any non-word characters, assume it's a regexp
    // but \W in qt is letter, number, or '_', I want to be a bit less strict
    QRegExp rx(QString::fromLatin1("[^\\w\\s-']"));
    if(text.find(rx) == -1) {
      // split by whitespace, and add rules for each word
      QStringList tokens = QStringList::split(QRegExp(QString::fromLatin1("\\s")), text);
      for(QStringList::Iterator it = tokens.begin(); it != tokens.end(); ++it) {
        // an empty field string means check every field
        filter->append(new FilterRule(fieldName, *it, FilterRule::FuncContains));
      }
    } else {
      // if it isn't valid, hold off on applying the filter
      QRegExp tx(text);
      if(!tx.isValid()) {
        myDebug() << "MainWindow::slotUpdateFilter() - invalid regexp: " << text << endl;
        return;
      }
      filter->append(new FilterRule(fieldName, text, FilterRule::FuncRegExp));
    }
    // also want to update the line edit in case the filter was set by DCOP
    if(m_quickFilter->text().isEmpty() && m_quickFilter->text() != text_) {
      m_quickFilter->setText(text_);
    }
  }
  // only update filter if one exists or did exist
  if(filter || m_detailedView->filter()) {
    Controller::self()->slotUpdateFilter(filter);
  }
}

void MainWindow::slotShowCollectionFieldsDialog() {
  if(!m_collFieldsDlg) {
    m_collFieldsDlg = new CollectionFieldsDialog(Data::Document::self()->collection(), this);
    connect(m_collFieldsDlg, SIGNAL(finished()),
            SLOT(slotHideCollectionFieldsDialog()));
  } else {
    KWin::activateWindow(m_collFieldsDlg->winId());
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

  Import::Format format = static_cast<Import::Format>(format_);
  bool checkURL = true;
  KURL url;
  switch(ImportDialog::importTarget(format)) {
    case Import::File:
      url = KFileDialog::getOpenURL(ImportDialog::startDir(format), ImportDialog::fileFilter(format),
                                    this, i18n("Import File"));
      break;

    case Import::Dir:
      // TODO: allow remote audiofile importing
      url.setPath(KFileDialog::getExistingDirectory(ImportDialog::startDir(format),
                                                    this, i18n("Import Directory")));
      break;

    case Import::None:
    default:
      checkURL = false;
      break;
  }

  if(checkURL) {
    bool ok = !url.isEmpty() && url.isValid() && KIO::NetAccess::exists(url, true, this);
    if(!ok) {
      StatusBar::self()->clearStatus();
      return;
    }
  }
  importFile(format, url);
  StatusBar::self()->clearStatus();
}

void MainWindow::slotFileExport(int format_) {
  slotStatusMsg(i18n("Exporting data..."));

  Export::Format format = static_cast<Export::Format>(format_);
  ExportDialog dlg(format, Data::Document::self()->collection(), this, "exportdialog");

  if(dlg.exec() == QDialog::Rejected) {
    StatusBar::self()->clearStatus();
    return;
  }

  switch(ExportDialog::exportTarget(format)) {
    case Export::None:
      dlg.exportURL();
      break;

    case Export::Dir:
      myDebug() << "MainWindow::slotFileExport() - ExportDir not implemented!" << endl;
      break;

    case Export::File:
    {
      KFileDialog fileDlg(QString::fromLatin1(":export"), dlg.fileFilter(), this, "filedialog", true);
      fileDlg.setCaption(i18n("Export As"));
      fileDlg.setOperationMode(KFileDialog::Saving);

      if(fileDlg.exec() == QDialog::Rejected) {
        StatusBar::self()->clearStatus();
        return;
      }

      KURL url = fileDlg.selectedURL();
      if(!url.isEmpty() && url.isValid()) {
        GUI::CursorSaver cs(Qt::waitCursor);
        dlg.exportURL(url);
      }
    }
    break;
  }

  StatusBar::self()->clearStatus();
}

void MainWindow::slotShowStringMacroDialog() {
  if(Data::Document::self()->collection()->type() != Data::Collection::Bibtex) {
    return;
  }

  if(!m_stringMacroDlg) {
    const Data::BibtexCollection* c = static_cast<Data::BibtexCollection*>(Data::Document::self()->collection().data());
    m_stringMacroDlg = new StringMapDialog(c->macroList(), this, "StringMacroDialog", false);
    m_stringMacroDlg->setCaption(i18n("String Macros"));
    m_stringMacroDlg->setLabels(i18n("Macro"), i18n("String"));
    connect(m_stringMacroDlg, SIGNAL(finished()), SLOT(slotHideStringMacroDialog()));
    connect(m_stringMacroDlg, SIGNAL(okClicked()), SLOT(slotStringMacroDialogOk()));
  } else {
    KWin::activateWindow(m_stringMacroDlg->winId());
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
    static_cast<Data::BibtexCollection*>(Data::Document::self()->collection().data())->setMacroList(m_stringMacroDlg->stringMap());
    Data::Document::self()->slotSetModified(true);
  }
}

void MainWindow::slotNewEntry() {
  m_toggleEntryEditor->setChecked(true);
  slotToggleEntryEditor();
  m_editDialog->slotHandleNew();
}

void MainWindow::slotEditDialogFinished() {
  m_toggleEntryEditor->setChecked(false);
}

void MainWindow::slotShowEntryEditor() {
  m_toggleEntryEditor->setChecked(true);
  m_editDialog->show();

  KWin::activateWindow(m_editDialog->winId());
}

void MainWindow::slotConvertToBibliography() {
  // only book collections can be converted to bibtex
  Data::CollPtr coll = Data::Document::self()->collection();
  if(!coll || coll->type() != Data::Collection::Book) {
    return;
  }

  GUI::CursorSaver cs;

  Data::CollPtr newColl = Data::BibtexCollection::convertBookCollection(coll);
  if(newColl) {
    m_newDocument = true;
    Kernel::self()->replaceCollection(newColl);
    m_fileOpenRecent->setCurrentItem(-1);
    slotUpdateToolbarIcons();
    updateCollectionActions();
  } else {
    kdWarning() << "MainWindow::slotConvertToBibliography() - ERROR: no bibliography created!" << endl;
  }
}

void MainWindow::slotCiteEntry(int action_) {
  StatusBar::self()->setStatus(i18n("Creating citations..."));
  Cite::ActionManager::self()->cite(static_cast<Cite::CiteAction>(action_), Controller::self()->selectedEntries());
  StatusBar::self()->clearStatus();
}

void MainWindow::slotShowFetchDialog() {
  if(!m_fetchDlg) {
    m_fetchDlg = new FetchDialog(this);
    connect(m_fetchDlg, SIGNAL(finished()), SLOT(slotHideFetchDialog()));
    connect(Controller::self(), SIGNAL(collectionAdded(int)), m_fetchDlg, SLOT(slotResetCollection()));
  } else {
    KWin::activateWindow(m_fetchDlg->winId());
  }
  m_fetchDlg->show();
}

void MainWindow::slotHideFetchDialog() {
  if(m_fetchDlg) {
    m_fetchDlg->delayedDestruct();
    m_fetchDlg = 0;
  }
}

bool MainWindow::importFile(Import::Format format_, const KURL& url_, Import::Action action_) {
  // try to open document
  GUI::CursorSaver cs(Qt::waitCursor);

  bool failed = false;
  Data::CollPtr coll;
  if(!url_.isEmpty() && url_.isValid() && KIO::NetAccess::exists(url_, true, this)) {
    coll = ImportDialog::importURL(format_, url_);
  } else {
    Kernel::self()->sorry(i18n(errorLoad).arg(url_.fileName()));
    failed = true;
  }

  if(!coll && !m_initialized) {
    // special case on startup when openURL() is called with a command line argument
    // and that URL can't be opened. The window still needs to be initialized
    // the doc object is created with an initial book collection, continue with that
    Controller::self()->slotCollectionAdded(Data::Document::self()->collection());
    m_fileSave->setEnabled(false);
    slotEnableOpenedActions();
    slotEnableModifiedActions(false);
    slotEntryCount();
    m_fileOpenRecent->setCurrentItem(-1);
    m_initialized = true;
    failed = true;
  } else if(coll) {
    // this is rather dumb, but I'm too lazy to find the bug
    // if the document isn't initialized, then Tellico crashes
    // since Document::replaceCollection() ends up calling lots of stuff that isn't initialized
    if(!m_initialized) {
      Controller::self()->slotCollectionAdded(Data::Document::self()->collection());
      m_initialized = true;
    }
    failed = !importCollection(coll, action_);
  }

  StatusBar::self()->clearStatus();
  return !failed; // return true means success
}

bool MainWindow::exportCollection(Export::Format format_, const KURL& url_) {
  if(!url_.isValid()) {
    myDebug() << "MainWindow::exportCollection() - invalid URL: " << url_.url() << endl;
    return false;
  }

  GUI::CursorSaver cs;
  const Data::CollPtr c = Data::Document::self()->collection();
  if(!c) {
    return false;
  }

  // only bibliographies can export to bibtex or bibtexml
  bool isBibtex = (c->type() == Data::Collection::Bibtex);
  if(!isBibtex && (format_ == Export::Bibtex || format_ == Export::Bibtexml)) {
    return false;
  }
  // only books and bibliographies can export to alexandria
  bool isBook = (c->type() == Data::Collection::Book);
  if(!isBibtex && !isBook && format_ == Export::Alexandria) {
    return false;
  }

  bool success = ExportDialog::exportCollection(format_, url_);
  return success;
}

bool MainWindow::showEntry(long id) {
  Data::EntryPtr entry = Data::Document::self()->collection()->entryById(id);
  if(entry) {
    m_viewStack->showEntry(entry);
  }
  return entry != 0;
}

void MainWindow::addFilterView() {
  if(m_filterView) {
    return;
  }

  m_filterView = new FilterView(m_viewTabs, "filterview");
  Controller::self()->addObserver(m_filterView);
  m_viewTabs->insertTab(m_filterView, SmallIcon(QString::fromLatin1("filter")), i18n("Filters"), 1);
  QWhatsThis::add(m_filterView, i18n("<qt>The <i>Filter View</i> shows the entries which meet certain "
                                     "filter rules.</qt>"));

  int sortStyle = Config::filterViewSortColumn();
  m_filterView->setSortStyle(static_cast<GUI::ListView::SortStyle>(sortStyle));
  bool sortAscending = Config::filterViewSortAscending();
  m_filterView->setSortOrder(sortAscending ? Qt::Ascending : Qt::Descending);
}

void MainWindow::addLoanView() {
  if(m_loanView) {
    return;
  }

  m_loanView = new LoanView(m_viewTabs, "loanview");
  Controller::self()->addObserver(m_loanView);
  m_viewTabs->insertTab(m_loanView, SmallIcon(QString::fromLatin1("kaddressbook")), i18n("Loans"), 2);
  QWhatsThis::add(m_loanView, i18n("<qt>The <i>Loan View</i> shows a list of all the people who "
                                   "have borrowed items from your collection.</qt>"));

  int sortStyle = Config::loanViewSortColumn();
  m_loanView->setSortStyle(static_cast<GUI::ListView::SortStyle>(sortStyle));
  bool sortAscending = Config::loanViewSortAscending();
  m_loanView->setSortOrder(sortAscending ? Qt::Ascending : Qt::Descending);
}

void MainWindow::updateCaption(bool modified_) {
  QString caption;
  if(Data::Document::self()->collection()) {
    caption = Data::Document::self()->collection()->title();
  }
  if(!m_newDocument) {
    if(!caption.isEmpty()) {
       caption += QString::fromLatin1(" - ");
    }
    KURL u = Data::Document::self()->URL();
    if(u.isLocalFile()) {
      // for new files, the path is set to /Untitled in Data::Document
      if(u.path() == '/' + i18n("Untitled")) {
        caption += u.fileName();
      } else {
        caption += u.path();
      }
    } else {
      caption += u.prettyURL();
    }
  }
  setCaption(caption, modified_);
}

void MainWindow::slotUpdateToolbarIcons() {
  //  myDebug() << "MainWindow::slotUpdateToolbarIcons() " << endl;
  // first change the icon for the menu item
  m_newEntry->setIconSet(UserIconSet(Kernel::self()->collectionTypeName()));

  // since the toolbar icon is probably a different size than the menu item icon
  // superimpose it on the "mime_empty" icon
  KToolBar* tb = toolBar("collectionToolBar");
  if(!tb) {
    return;
  }

  for(int i = 0; i < tb->count(); ++i) {
    if(m_newEntry->isPlugged(tb, tb->idAt(i))) {
      QIconSet icons;
      icons.installIconFactory(new EntryIconFactory(tb->iconSize()));
      tb->setButtonIconSet(tb->idAt(i), icons);
      break;
    }
  }
}

void MainWindow::slotGroupLabelActivated() {
  // need entry grouping combo id
  KToolBar* tb = toolBar("collectionToolBar");
  if(!tb) {
    return;
  }

  for(int i = 0; i < tb->count(); ++i) {
    if(m_entryGrouping->isPlugged(tb, tb->idAt(i))) {
      KComboBox* combo = tb->getCombo(tb->idAt(i));
      if(combo) {
        combo->popup();
        break;
      }
    }
  }
}

void MainWindow::slotFilterLabelActivated() {
  m_quickFilter->setFocus();
  m_quickFilter->selectAll();
}

void MainWindow::slotClearFilter() {
  m_quickFilter->clear();
  slotQueueFilter();
}

void MainWindow::slotRenameCollection() {
  Kernel::self()->renameCollection();
}

void MainWindow::updateCollectionActions() {
  if(!Data::Document::self()->collection()) {
    return;
  }

  stateChanged(QString::fromLatin1("collection_reset"));
  Data::Collection::Type type = Data::Document::self()->collection()->type();
  switch(type) {
    case Data::Collection::Book:
      stateChanged(QString::fromLatin1("is_book"));
      break;
    case Data::Collection::Bibtex:
      stateChanged(QString::fromLatin1("is_bibliography"));
      break;
    case Data::Collection::Video:
      stateChanged(QString::fromLatin1("is_video"));
      break;
    default:
      break;
  }
  Controller::self()->updateActions();
  // special case when there are no available data sources
  if(m_fetchActions.isEmpty() && m_updateAll) {
    m_updateAll->setEnabled(false);
  }
}

void MainWindow::updateEntrySources() {
  QSignalMapper* mapper = ::qt_cast<QSignalMapper*>(child("update_mapper"));
  if(!mapper) {
    kdWarning() << "MainWindow::updateEntrySources() - no update mapper!" << endl;
    return;
  }

  unplugActionList(QString::fromLatin1("update_entry_actions"));
  for(QPtrListIterator<KAction> it(m_fetchActions); it.current(); ++it) {
    it.current()->unplugAll();
    mapper->removeMappings(it.current());
  }
  // autoDelete() all actions, which removes them from the actionCollection()
  m_fetchActions.clear();

  Fetch::FetcherVec vec = Fetch::Manager::self()->fetchers(Kernel::self()->collectionType());
  for(Fetch::FetcherVec::Iterator it = vec.begin(); it != vec.end(); ++it) {
    KAction* action = new KAction(actionCollection());
    action->setText(it->source());
    action->setToolTip(i18n("Update entry data from %1").arg(it->source()));
    action->setIconSet(Fetch::Manager::fetcherIcon(it.data()));
    connect(action, SIGNAL(activated()), mapper, SLOT(map()));
    mapper->setMapping(action, it->source());
    m_fetchActions.append(action);
  }

  plugActionList(QString::fromLatin1("update_entry_actions"), m_fetchActions);
}

void MainWindow::importFile(Import::Format format_, const KURL::List& urls_) {
  KURL::List urls = urls_;
  // update as DropHandler and Importer classes are updated
  if(urls_.count() > 1 &&
     format_ != Import::Bibtex &&
     format_ != Import::RIS &&
     format_ != Import::PDF) {
    KURL u = urls_.front();
    QString url = u.isLocalFile() ? u.path() : u.prettyURL();
    Kernel::self()->sorry(i18n("Tellico can only import one file of this type at a time. "
                               "Only %1 will be imported.").arg(url));
    urls.clear();
    urls = u;
  }

  ImportDialog dlg(format_, urls, this, "importdlg");
  if(dlg.exec() != QDialog::Accepted) {
    return;
  }

//  if edit dialog is saved ok and if replacing, then the doc is saved ok
  if(m_editDialog->queryModified() &&
     (dlg.action() != Import::Replace || Data::Document::self()->saveModified())) {
    GUI::CursorSaver cs(Qt::waitCursor);
    Data::CollPtr coll = dlg.collection();
    if(!coll) {
      if(!dlg.statusMessage().isEmpty()) {
        Kernel::self()->sorry(dlg.statusMessage());
      }
      return;
    }
    importCollection(coll, dlg.action());
  }
}

bool MainWindow::importCollection(Data::CollPtr coll_, Import::Action action_) {
  bool failed = false;
  switch(action_) {
    case Import::Append:
      {
        // only append if match, but special case importing books into bibliographies
        Data::CollPtr c = Data::Document::self()->collection();
        if(c->type() == coll_->type()
          || (c->type() == Data::Collection::Bibtex && coll_->type() == Data::Collection::Book)) {
          Kernel::self()->appendCollection(coll_);
          slotEnableModifiedActions(true);
        } else {
          Kernel::self()->sorry(i18n(errorAppendType));
          failed = true;
        }
      }
      break;

    case Import::Merge:
      {
        // only merge if match, but special case importing books into bibliographies
        Data::CollPtr c = Data::Document::self()->collection();
        if(c->type() == coll_->type()
          || (c->type() == Data::Collection::Bibtex && coll_->type() == Data::Collection::Book)) {
          Kernel::self()->mergeCollection(coll_);
          slotEnableModifiedActions(true);
        } else {
          Kernel::self()->sorry(i18n(errorMergeType));
          failed = true;
        }
      }
      break;

    default: // replace
      Kernel::self()->replaceCollection(coll_);
      m_fileOpenRecent->setCurrentItem(-1);
      m_newDocument = true;
      slotEnableOpenedActions();
      slotEnableModifiedActions(false);
      break;
  }
  return !failed;
}

void MainWindow::slotURLAction(const KURL& url_) {
  Q_ASSERT(url_.protocol() == Latin1Literal("tc"));
  QString actionName = url_.fileName();
  KAction* action = this->action(actionName);
  if(action) {
    action->activate();
  } else {
    myWarning() << "MainWindow::slotURLAction() - unknown action: " << actionName << endl;
  }
}

#include "mainwindow.moc"
