/***************************************************************************
    copyright            : (C) 2001-2008 by Robby Stephenson
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
#include "gui/lineedit.h"
#include "tellico_utils.h"
#include "tellico_debug.h"
#include "statusbar.h"
#include "fetch/fetchmanager.h"
#include "cite/actionmanager.h"
#include "core/tellico_config.h"
#include "core/drophandler.h"
#include "core/dcopinterface.h"
#include "models/models.h"
#include "newstuff/manager.h"

#include <kapplication.h>
#include <kcombobox.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstandardaction.h>
#include <kwindowsystem.h>
#include <kprogressdialog.h>
#include <khtmlview.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <ktip.h>
#include <krecentdocument.h>
#include <kedittoolbar.h>
#include <kshortcutsdialog.h>
#include <kio/netaccess.h>
#include <kaction.h>
#include <krecentfilesaction.h>
#include <ktoggleaction.h>
#include <kactioncollection.h>
#include <kactionmenu.h>
#include <KShortcutsDialog>
#include <kundostack.h>
#include <KTabWidget>

#include <QSplitter>
//#include <QPainter>
#include <QSignalMapper>
#include <QTimer>
#include <QMetaObject> // needed for copy, cut, paste slots
#include <QDBusConnection>

namespace {
  static const int MAIN_WINDOW_MIN_WIDTH = 600;
  static const int MAX_IMAGES_WARN_PERFORMANCE = 200;
}

using Tellico::MainWindow;

MainWindow::MainWindow(QWidget* parent_/*=0*/) : KXmlGuiWindow(parent_),
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
    m_newDocument(true),
    m_dontQueueFilter(false) {

  Controller::init(this); // the only time this is ever called!
  // has to be after controller init
  Kernel::init(this); // the only time this is ever called!

  setWindowIcon(DesktopIcon(QString::fromLatin1("tellico")));

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

  new ApplicationInterface(this);
  new CollectionInterface(this);

  // Init DBUS
  Tellico::NewStuff::Manager::self();

  MARK_LINE;
  QTimer::singleShot(0, this, SLOT(slotInit()));
}

MainWindow::~MainWindow() {
  qDeleteAll(m_fetchActions);
  m_fetchActions.clear();
}

void MainWindow::slotInit() {
  if(m_editDialog) {
    return;
  }

  m_editDialog = new EntryEditDialog(this);
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
  setStatusBar(m_statusBar);
}

namespace {

inline
KIcon mimeIcon(const char* s) {
  KMimeType::Ptr ptr = KMimeType::mimeType(QString::fromLatin1(s), KMimeType::ResolveAliases);
  return ptr ? KIcon(ptr->iconName()) : KIcon();
}

inline
KIcon mimeIcon(const char* s1, const char* s2) {
  KMimeType::Ptr ptr = KMimeType::mimeType(QString::fromLatin1(s1), KMimeType::ResolveAliases);
  if(!ptr) {
    ptr = KMimeType::mimeType(QString::fromLatin1(s2), KMimeType::ResolveAliases);
  }
  return ptr ? KIcon(ptr->iconName()) : KIcon();
}

}

void MainWindow::initActions() {
  MARK;
  /*************************************************
   * File->New menu
   *************************************************/
  QSignalMapper* collectionMapper = new QSignalMapper(this);
  connect(collectionMapper, SIGNAL(mapped(int)),
          this, SLOT(slotFileNew(int)));

  KActionMenu* fileNewMenu = new KActionMenu(i18n("New"), this);
  fileNewMenu->setIcon(KIcon(QString::fromLatin1("document-new")));
  fileNewMenu->setToolTip(i18n("Create a new collection"));
  fileNewMenu->setDelayed(false);
  actionCollection()->addAction(QString::fromLatin1("file_new_collection"), fileNewMenu);

  KAction* action;

#define COLL_ACTION(TYPE, NAME, TEXT, TIP, ICON) \
  action = actionCollection()->addAction(QString::fromLatin1(NAME), collectionMapper, SLOT(map())); \
  action->setText(TEXT); \
  action->setToolTip(TIP); \
  action->setIcon(KIcon(QString::fromLatin1(ICON))); \
  fileNewMenu->addAction(action); \
  collectionMapper->setMapping(action, Data::Collection::TYPE);

  COLL_ACTION(Book, "new_book_collection", i18n("New &Book Collection"),
              i18n("Create a new book collection"), "book");

  COLL_ACTION(Bibtex, "new_bibtex_collection", i18n("New B&ibliography"),
              i18n("Create a new bibtex bibliography"), "bibtex");

  COLL_ACTION(ComicBook, "new_comic_book_collection", i18n("New &Comic Book Collection"),
              i18n("Create a new comic book collection"), "comic");

  COLL_ACTION(Video, "new_video_collection", i18n("New &Video Collection"),
              i18n("Create a new video collection"), "video");

  COLL_ACTION(Album, "new_music_collection", i18n("New &Music Collection"),
              i18n("Create a new music collection"), "album");

  COLL_ACTION(Coin, "new_coin_collection", i18n("New C&oin Collection"),
              i18n("Create a new coin collection"), "coin");

  COLL_ACTION(Stamp, "new_stamp_collection", i18n("New &Stamp Collection"),
              i18n("Create a new stamp collection"), "stamp");

  COLL_ACTION(Card, "new_card_collection", i18n("New C&ard Collection"),
              i18n("Create a new trading card collection"), "card");

  COLL_ACTION(Wine, "new_wine_collection", i18n("New &Wine Collection"),
              i18n("Create a new wine collection"), "wine");

  COLL_ACTION(Game, "new_game_collection", i18n("New &Game Collection"),
              i18n("Create a new game collection"), "game");

  COLL_ACTION(BoardGame, "new_boardgame_collection", i18n("New Boa&rd Game Collection"),
              i18n("Create a new board game collection"), "boardgame");

  COLL_ACTION(File, "BoardGame", i18n("New &File Catalog"),
              i18n("Create a new file catalog"), "file");

  action = actionCollection()->addAction(QString::fromLatin1("new_custom_collection"), collectionMapper, SLOT(map()));
  action->setText(i18n("New C&ustom Collection"));
  action->setToolTip(i18n("Create a new custom collection"));
  action->setIcon(KIcon(QString::fromLatin1("document-new")));
  fileNewMenu->addAction(action);
  collectionMapper->setMapping(action, Data::Collection::Base);

#undef COLL_ACTION

  /*************************************************
   * File menu
   *************************************************/
  action = KStandardAction::open(this, SLOT(slotFileOpen()), actionCollection());
  action->setToolTip(i18n("Open an existing document"));
  m_fileOpenRecent = KStandardAction::openRecent(this, SLOT(slotFileOpenRecent(const KUrl&)), actionCollection());
  m_fileOpenRecent->setToolTip(i18n("Open a recently used file"));
  m_fileSave = KStandardAction::save(this, SLOT(slotFileSave()), actionCollection());
  m_fileSave->setToolTip(i18n("Save the document"));
  action = KStandardAction::saveAs(this, SLOT(slotFileSaveAs()), actionCollection());
  action->setToolTip(i18n("Save the document as a different file..."));
  action = KStandardAction::print(this, SLOT(slotFilePrint()), actionCollection());
  action->setToolTip(i18n("Print the contents of the document..."));
  action = KStandardAction::quit(this, SLOT(slotFileQuit()), actionCollection());
  action->setToolTip(i18n("Quit the application"));

/**************** Import Menu ***************************/

  QSignalMapper* importMapper = new QSignalMapper(this);
  connect(importMapper, SIGNAL(mapped(int)),
          this, SLOT(slotFileImport(int)));

  KActionMenu* importMenu = new KActionMenu(i18n("&Import"), this);
  importMenu->setIcon(KIcon(QString::fromLatin1("document-import")));
  importMenu->setToolTip(i18n("Import the collection data from other formats"));
  importMenu->setDelayed(false);
  actionCollection()->addAction(QString::fromLatin1("file_import"), importMenu);

#define IMPORT_ACTION(TYPE, NAME, TEXT, TIP, ICON) \
  action = actionCollection()->addAction(QString::fromLatin1(NAME), importMapper, SLOT(map())); \
  action->setText(TEXT); \
  action->setToolTip(TIP); \
  action->setIcon(ICON); \
  importMenu->addAction(action); \
  importMapper->setMapping(action, TYPE);

  IMPORT_ACTION(Import::TellicoXML, "file_import_tellico", i18n("Import Tellico Data..."),
                i18n("Import another Tellico data file"), KIcon(QString::fromLatin1("tellico")));

  IMPORT_ACTION(Import::CSV, "file_import_csv", i18n("Import CSV Data..."),
                i18n("Import a CSV file"), mimeIcon("text/csv", "text/x-csv"));

  IMPORT_ACTION(Import::MODS, "file_import_mods", i18n("Import MODS Data..."),
                i18n("Import a MODS data file"), mimeIcon("text/xml"));

  IMPORT_ACTION(Import::Alexandria, "file_import_alexandria", i18n("Import Alexandria Data..."),
                i18n("Import data from the Alexandria book collection manager"), KIcon(QString::fromLatin1("alexandria")));

  IMPORT_ACTION(Import::Delicious, "file_import_delicious", i18n("Import Delicious Library Data..."),
                i18n("Import data from Delicious Library"), mimeIcon("text/xml"));

  IMPORT_ACTION(Import::Referencer, "file_import_referencer", i18n("Import Referencer Data..."),
                i18n("Import data from Referencer"), KIcon(QString::fromLatin1("referencer")));

  IMPORT_ACTION(Import::Bibtex, "file_import_bibtex", i18n("Import Bibtex Data..."),
                i18n("Import a bibtex bibliography file"), mimeIcon("text/x-bibtex"));

  IMPORT_ACTION(Import::Bibtexml, "file_import_bibtexml", i18n("Import Bibtexml Data..."),
                i18n("Import a Bibtexml bibliography file"), mimeIcon("text/xml"));

  IMPORT_ACTION(Import::RIS, "file_import_ris", i18n("Import RIS Data..."),
                i18n("Import an RIS reference file"), mimeIcon("application/x-research-info-systems"));

  IMPORT_ACTION(Import::PDF, "file_import_pdf", i18n("Import PDF File..."),
                i18n("Import a PDF file"), mimeIcon("application/pdf"));

  IMPORT_ACTION(Import::AudioFile, "file_import_audiofile", i18n("Import Audio File Metadata..."),
                i18n("Import meta-data from audio files"), mimeIcon("audio/mp3", "audioaudio/x-mp3"));
#ifndef HAVE_TAGLIB
  action->setEnabled(false);
#endif

  IMPORT_ACTION(Import::FreeDB, "file_import_freedb", i18n("Import Audio CD Data..."),
                i18n("Import audio CD information"), mimeIcon("media/audiocd"));
#ifndef HAVE_KCDDB
  action->setEnabled(false);
#endif

  IMPORT_ACTION(Import::GCfilms, "file_import_gcfilms", i18n("Import GCstar Data..."),
                i18n("Import a GCstar data file"), KIcon(QString::fromLatin1("gcstar")));

  IMPORT_ACTION(Import::Griffith, "file_import_griffith", i18n("Import Griffith Data..."),
                i18n("Import a Griffith database"), KIcon(QString::fromLatin1("griffith")));

  IMPORT_ACTION(Import::AMC, "file_import_amc", i18n("Import Ant Movie Catalog Data..."),
                i18n("Import an Ant Movie Catalog data file"), mimeIcon("application/x-crossover-amc"));

  IMPORT_ACTION(Import::FileListing, "file_import_filelisting", i18n("Import File Listing..."),
                i18n("Import information about files in a folder"), mimeIcon("inode/directory"));

  IMPORT_ACTION(Import::XSLT, "file_import_xslt", i18n("Import XSL Transform..."),
                i18n("Import using an XSL Transform"), mimeIcon("application/xslt+xml", "text/x-xslt"));

#undef IMPORT_ACTION

/**************** Export Menu ***************************/

  QSignalMapper* exportMapper = new QSignalMapper(this);
  connect(exportMapper, SIGNAL(mapped(int)),
          this, SLOT(slotFileExport(int)));

  KActionMenu* exportMenu = new KActionMenu(i18n("&Export"), this);
  exportMenu->setIcon(KIcon(QString::fromLatin1("document-export")));
  exportMenu->setToolTip(i18n("Export the collection data to other formats"));
  exportMenu->setDelayed(false);
  actionCollection()->addAction(QString::fromLatin1("file_export"), exportMenu);

#define EXPORT_ACTION(TYPE, NAME, TEXT, TIP, ICON) \
  action = actionCollection()->addAction(QString::fromLatin1(NAME), exportMapper, SLOT(map())); \
  action->setText(TEXT); \
  action->setToolTip(TIP); \
  action->setIcon(ICON); \
  exportMenu->addAction(action); \
  exportMapper->setMapping(action, TYPE);

  EXPORT_ACTION(Export::TellicoXML, "file_export_xml", i18n("Export to XML..."),
                i18n("Export to a Tellico XML file"), KIcon(QString::fromLatin1("tellico")));

  EXPORT_ACTION(Export::TellicoZip, "file_export_zip", i18n("Export to Zip..."),
                i18n("Export to a Tellico Zip file"), KIcon(QString::fromLatin1("tellico")));

  EXPORT_ACTION(Export::HTML, "file_export_html", i18n("Export to HTML..."),
                i18n("Export to an HTML file"), mimeIcon("text/html"));

  EXPORT_ACTION(Export::CSV, "file_export_csv", i18n("Export to CSV..."),
                i18n("Export to a comma-separated values file"), mimeIcon("text/csv", "text/x-csv"));

  EXPORT_ACTION(Export::PilotDB, "file_export_pilotdb", i18n("Export to PilotDB..."),
                i18n("Export to a PilotDB database"), mimeIcon("application/vnd.palm"));

  EXPORT_ACTION(Export::Alexandria, "file_export_alexandria", i18n("Export to Alexandria..."),
                i18n("Export to an Alexandria library"), KIcon(QString::fromLatin1("alexandria")));

  EXPORT_ACTION(Export::Bibtex, "file_export_bibtex", i18n("Export to Bibtex..."),
                i18n("Export to a bibtex file"), mimeIcon("text/x-bibtex"));

  EXPORT_ACTION(Export::Bibtexml, "file_export_bibtexml", i18n("Export to Bibtexml..."),
                i18n("Export to a Bibtexml file"), mimeIcon("text/xml"));

  EXPORT_ACTION(Export::ONIX, "file_export_onix", i18n("Export to ONIX..."),
                i18n("Export to an ONIX file"), mimeIcon("text/xml"));

  EXPORT_ACTION(Export::GCfilms, "file_export_gcfilms", i18n("Export to GCfilms..."),
                i18n("Export to a GCfilms data file"), KIcon(QString::fromLatin1("gcstar")));

#if 0
    QString dummy1 = i18n("Export to GCstar...");
    QString dummy2 = i18n("Export to a GCstar data file");
#endif

  EXPORT_ACTION(Export::XSLT, "file_export_xslt", i18n("Export XSL Transform..."),
                i18n("Export using an XSL Transform"), mimeIcon("application/xslt+xml", "text/x-xslt"));

#undef EXPORT_ACTION

  /*************************************************
   * Edit menu
   *************************************************/
  Kernel::self()->commandHistory()->createUndoAction(actionCollection());
  Kernel::self()->commandHistory()->createRedoAction(actionCollection());

  action = KStandardAction::cut(this, SLOT(slotEditCut()), actionCollection());
  action->setToolTip(i18n("Cut the selected text and puts it in the clipboard"));
  action = KStandardAction::copy(this, SLOT(slotEditCopy()), actionCollection());
  action->setToolTip(i18n("Copy the selected text to the clipboard"));
  action = KStandardAction::paste(this, SLOT(slotEditPaste()), actionCollection());
  action->setToolTip(i18n("Paste the clipboard contents"));
  action = KStandardAction::selectAll(this, SLOT(slotEditSelectAll()), actionCollection());
  action->setToolTip(i18n("Select all the entries in the collection"));
  action = KStandardAction::deselect(this, SLOT(slotEditDeselect()), actionCollection());
  action->setToolTip(i18n("Deselect all the entries in the collection"));

  action = actionCollection()->addAction(QString::fromLatin1("edit_search_internet"), this, SLOT(slotShowFetchDialog()));
  action->setText(i18n("Internet Search..."));
  action->setIconText(i18n("Search"));  // find a better word for this?
  action->setIcon(KIcon(QString::fromLatin1("tools-wizard")));
  action->setShortcut(Qt::CTRL + Qt::Key_M);
  action->setToolTip(i18n("Search the internet..."));

  action = actionCollection()->addAction(QString::fromLatin1("filter_dialog"), this, SLOT(slotShowFilterDialog()));
  action->setText(i18n("Advanced &Filter..."));
  action->setIconText(i18n("Filter"));
  action->setIcon(KIcon(QString::fromLatin1("view-filter")));
  action->setShortcut(Qt::CTRL + Qt::Key_J);
  action->setToolTip(i18n("Filter the collection"));

  /*************************************************
   * Collection menu
   *************************************************/
  m_newEntry = actionCollection()->addAction(QString::fromLatin1("coll_new_entry"),
                                             this, SLOT(slotNewEntry()));
  m_newEntry->setText(i18n("&New Entry..."));
  m_newEntry->setIcon(KIcon(QString::fromLatin1("document-new")));
  m_newEntry->setShortcut(Qt::CTRL + Qt::Key_N);
  m_newEntry->setToolTip(i18n("Create a new entry"));

  m_editEntry = actionCollection()->addAction(QString::fromLatin1("coll_edit_entry"),
                                              this, SLOT(slotShowEntryEditor()));
  m_editEntry->setText(i18n("&Edit Entry..."));
  m_editEntry->setIcon(KIcon(QString::fromLatin1("document-properties")));
  m_editEntry->setShortcut(Qt::CTRL + Qt::Key_E);
  m_editEntry->setToolTip(i18n("Edit the selected entries"));

  m_copyEntry = actionCollection()->addAction(QString::fromLatin1("coll_copy_entry"),
                                              Controller::self(), SLOT(slotCopySelectedEntries()));
  m_copyEntry->setText(i18n("D&uplicate Entry"));
  m_copyEntry->setIcon(KIcon(QString::fromLatin1("edit-copy")));
  m_copyEntry->setShortcut(Qt::CTRL + Qt::Key_Y);
  m_copyEntry->setToolTip(i18n("Copy the selected entries"));

  m_deleteEntry = actionCollection()->addAction(QString::fromLatin1("coll_delete_entry"),
                                                Controller::self(), SLOT(slotDeleteSelectedEntries()));
  m_deleteEntry->setText(i18n("&Delete Entry"));
  m_deleteEntry->setIcon(KIcon(QString::fromLatin1("edit-delete")));
  m_deleteEntry->setShortcut(Qt::CTRL + Qt::Key_D);
  m_deleteEntry->setToolTip(i18n("Delete the selected entries"));

  m_mergeEntry = actionCollection()->addAction(QString::fromLatin1("coll_merge_entry"),
                                               Controller::self(), SLOT(slotMergeSelectedEntries()));
  m_mergeEntry->setText(i18n("&Merge Entries"));
  m_mergeEntry->setIcon(KIcon(QString::fromLatin1("document-import")));
  m_mergeEntry->setShortcut(Qt::CTRL + Qt::Key_G);
  m_mergeEntry->setToolTip(i18n("Merge the selected entries"));
  m_mergeEntry->setEnabled(false); // gets enabled when more than 1 entry is selected

  action = actionCollection()->addAction(QString::fromLatin1("coll_reports"), this, SLOT(slotShowReportDialog()));
  action->setText(i18n("&Generate Reports..."));
  action->setIconText(i18n("Reports"));
  action->setIcon(KIcon(QString::fromLatin1("text-rdf")));
  action->setToolTip(i18n("Generate collection reports"));

  m_checkOutEntry = actionCollection()->addAction(QString::fromLatin1("coll_checkout"), Controller::self(), SLOT(slotCheckOut()));
  m_checkOutEntry->setText(i18n("Check-&out..."));
  m_checkOutEntry->setIcon(KIcon(QString::fromLatin1("arrow-up-double")));
  m_checkOutEntry->setToolTip(i18n("Check-out the selected items"));

  m_checkInEntry = actionCollection()->addAction(QString::fromLatin1("coll_checkin"), Controller::self(), SLOT(slotCheckIn()));
  m_checkInEntry->setText(i18n("Check-&in"));
  m_checkInEntry->setIcon(KIcon(QString::fromLatin1("arrow-down-double")));
  m_checkInEntry->setToolTip(i18n("Check-in the selected items"));

  action = actionCollection()->addAction(QString::fromLatin1("coll_rename_collection"), this, SLOT(slotRenameCollection()));
  action->setText(i18n("&Rename Collection..."));
  action->setIcon(KIcon(QString::fromLatin1("edit-rename")));
  action->setShortcut(Qt::CTRL + Qt::Key_R);
  action->setToolTip(i18n("Rename the collection"));

  action = actionCollection()->addAction(QString::fromLatin1("coll_fields"), this, SLOT(slotShowCollectionFieldsDialog()));
  action->setText(i18n("Collection &Fields..."));
  action->setIconText(i18n("Fields"));
  action->setIcon(KIcon(QString::fromLatin1("preferences-other")));
  action->setShortcut(Qt::CTRL + Qt::Key_U);
  action->setToolTip(i18n("Modify the collection fields"));

  action = actionCollection()->addAction(QString::fromLatin1("coll_convert_bibliography"), this, SLOT(slotConvertToBibliography()));
  action->setText(i18n("Convert to &Bibliography"));
  action->setIcon(KIcon(QString::fromLatin1("bibtex")));
  action->setToolTip(i18n("Convert a book collection to a bibliography"));

  action = actionCollection()->addAction(QString::fromLatin1("coll_string_macros"), this, SLOT(slotShowStringMacroDialog()));
  action->setText(i18n("String &Macros..."));
  action->setIcon(KIcon(QString::fromLatin1("fileview-text")));
  action->setToolTip(i18n("Edit the bibtex string macros"));

  QSignalMapper* citeMapper = new QSignalMapper(this);
  connect(citeMapper, SIGNAL(mapped(int)),
          this, SLOT(slotCiteEntry(int)));

  action = actionCollection()->addAction(QString::fromLatin1("cite_clipboard"), citeMapper, SLOT(map()));
  action->setText(i18n("Copy Bibtex to Cli&pboard"));
  action->setToolTip(i18n("Copy bibtex citations to the clipboard"));
  action->setIcon(KIcon(QString::fromLatin1("edit-paste")));
  citeMapper->setMapping(action, Cite::CiteClipboard);

  action = actionCollection()->addAction(QString::fromLatin1("cite_lyxpipe"), citeMapper, SLOT(map()));
  action->setText(i18n("Cite Entry in &LyX"));
  action->setToolTip(i18n("Cite the selected entries in LyX"));
  action->setIcon(KIcon(QString::fromLatin1("lyx")));
  citeMapper->setMapping(action, Cite::CiteLyxpipe);

  action = actionCollection()->addAction(QString::fromLatin1("cite_openoffice"), citeMapper, SLOT(map()));
  action->setText(i18n("Ci&te Entry in OpenOffice.org"));
  action->setToolTip(i18n("Cite the selected entries in OpenOffice.org"));
  action->setIcon(KIcon(QString::fromLatin1("ooo-writer")));
  citeMapper->setMapping(action, Cite::CiteOpenOffice);

  m_updateMapper = new QSignalMapper(this);
  connect(m_updateMapper, SIGNAL(mapped(const QString&)),
          Controller::self(), SLOT(slotUpdateSelectedEntries(const QString&)));

  m_updateEntryMenu = new KActionMenu(i18n("&Update Entry"), this);
  m_updateEntryMenu->setIcon(KIcon(QString::fromLatin1("document-export")));
  m_updateEntryMenu->setDelayed(false);
  actionCollection()->addAction(QString::fromLatin1("coll_update_entry"), m_updateEntryMenu);

  m_updateAll = actionCollection()->addAction(QString::fromLatin1("update_entry_all"), m_updateMapper, SLOT(map()));
  m_updateAll->setText(i18n("All Sources"));
  m_updateAll->setToolTip(i18n("Update entry data from all available sources"));
  m_updateMapper->setMapping(m_updateAll, QString::fromLatin1("_all"));

  /*************************************************
   * Settings menu
   *************************************************/
  setStandardToolBarMenuEnabled(true);
  createStandardStatusBarAction();

  KStandardAction::configureToolbars(this, SLOT(slotConfigToolbar()), actionCollection());
  KStandardAction::keyBindings(this, SLOT(slotConfigKeys()), actionCollection());

  m_toggleGroupWidget = new KToggleAction(i18n("Show Grou&p View"), this);
  m_toggleGroupWidget->setToolTip(i18n("Enable/disable the group view"));
  connect(m_toggleGroupWidget, SIGNAL(triggered()), SLOT(slotToggleGroupWidget()));
  actionCollection()->addAction(QString::fromLatin1("toggle_group_widget"), m_toggleGroupWidget);

  m_toggleEntryEditor = new KToggleAction(i18n("Show Entry &Editor"), this);
  connect(m_toggleEntryEditor, SIGNAL(triggered()), SLOT(slotToggleEntryEditor()));
  m_toggleEntryEditor->setToolTip(i18n("Enable/disable the editor"));
  actionCollection()->addAction(QString::fromLatin1("toggle_edit_widget"), m_toggleEntryEditor);

  m_toggleEntryView = new KToggleAction(i18n("Show Entry &View"), this);
  m_toggleEntryView->setToolTip(i18n("Enable/disable the entry view"));
  connect(m_toggleEntryView, SIGNAL(triggered()), SLOT(slotToggleEntryView()));
  actionCollection()->addAction(QString::fromLatin1("toggle_entry_view"), m_toggleEntryView);

  KStandardAction::preferences(this, SLOT(slotShowConfigDialog()), actionCollection());

  /*************************************************
   * Help menu
   *************************************************/
  KStandardAction::tipOfDay(this, SLOT(slotShowTipOfDay()), actionCollection());

  /*************************************************
   * Collection Toolbar
   *************************************************/
  action = actionCollection()->addAction(QString::fromLatin1("change_entry_grouping_accel"), this, SLOT(slotGroupLabelActivated()));
  action->setText(i18n("Change Grouping"));
  action->setShortcut(Qt::CTRL + Qt::Key_G);

  m_entryGrouping = new KSelectAction(i18n("&Group Selection"), this);
  m_entryGrouping->setToolTip(i18n("Change the grouping of the collection"));
  connect(m_entryGrouping, SIGNAL(triggered(int)), SLOT(slotChangeGrouping()));
  actionCollection()->addAction(QString::fromLatin1("change_entry_grouping"), m_entryGrouping);

  action = actionCollection()->addAction(QString::fromLatin1("quick_filter_accel"), this, SLOT(slotFilterLabelActivated()));
  action->setText(i18n("Filter"));
  action->setShortcut(Qt::CTRL + Qt::Key_F);

  m_quickFilter = new GUI::LineEdit(this);
  m_quickFilter->setClickMessage(i18n("Filter here...")); // same text as kdepim and amarok
  m_quickFilter->setClearButtonShown(true);
  // about 10 characters wide
  m_quickFilter->setFixedWidth(m_quickFilter->fontMetrics().maxWidth()*10);
  // want to update every time the filter text changes
  connect(m_quickFilter, SIGNAL(textChanged(const QString&)),
          this, SLOT(slotQueueFilter()));
  connect(m_quickFilter, SIGNAL(clearButtonClicked()),
          this, SLOT(slotClearFilter()));
  m_quickFilter->installEventFilter(this); // intercept keyEvents

  action = new KAction(i18n("Filter"), this);
  action->setDefaultWidget(m_quickFilter);
  action->setToolTip(i18n("Filter the collection"));
  action->setShortcutConfigurable(false);
  actionCollection()->addAction(QString::fromLatin1("quick_filter"), action);

  //  wAction->setAutoSized(true);

#ifdef UIFILE
  myWarning() << "change createGUI() call!";
  createGUI(UIFILE);
#else
  createGUI();
#endif
}

#undef mimeIcon

void MainWindow::initDocument() {
  MARK;
  Data::Document* doc = Data::Document::self();

  KConfigGroup config(KGlobal::config(), "General Options");
  doc->setLoadAllImages(config.readEntry("Load All Images", false));

  // allow status messages from the document
  connect(doc, SIGNAL(signalStatusMsg(const QString&)),
          SLOT(slotStatusMsg(const QString&)));

  // do stuff that changes when the doc is modified
  connect(doc, SIGNAL(signalModified(bool)),
          SLOT(slotEnableModifiedActions(bool)));

  connect(Kernel::self()->commandHistory(), SIGNAL(cleanChanged(bool)),
          doc, SLOT(slotSetClean(bool)));
}

void MainWindow::initView() {
  MARK;
  m_split = new QSplitter(Qt::Horizontal, this);
  setCentralWidget(m_split);

  m_viewTabs = new KTabWidget(m_split);
  m_viewTabs->setTabBarHidden(true);
  m_groupView = new GroupView(m_viewTabs);
  Controller::self()->addObserver(m_groupView);
  m_viewTabs->addTab(m_groupView, KIcon(QString::fromLatin1("folder")), i18n("Groups"));
  m_groupView->setWhatsThis(i18n("<qt>The <i>Group View</i> sorts the entries into groupings "
                                    "based on a selected field.</qt>"));

  m_rightSplit = new QSplitter(Qt::Vertical, m_split);

  m_detailedView = new DetailedListView(m_rightSplit);
  Controller::self()->addObserver(m_detailedView);
  m_detailedView->setWhatsThis(i18n("<qt>The <i>Column View</i> shows the value of multiple fields "
                                       "for each entry.</qt>"));
  connect(Data::Document::self(), SIGNAL(signalCollectionImagesLoaded(Tellico::Data::CollPtr)),
          m_detailedView, SLOT(slotRefreshImages()));

  m_viewStack = new ViewStack(m_rightSplit);
  Controller::self()->addObserver(m_viewStack->iconView());
  connect(m_viewStack->entryView(), SIGNAL(signalAction(const KUrl&)),
          SLOT(slotURLAction(const KUrl&)));

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
    KUrl lastFile(Config::lastOpenFile()); // empty string is actually ok, it gets handled
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
    QString welcomeFile = KStandardDirs::locate("appdata", QString::fromLatin1("welcome.html"));
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
  KConfigGroup config(KGlobal::config(), "Main Window Options");
  saveMainWindowSettings(config);

  Config::setShowGroupWidget(m_toggleGroupWidget->isChecked());
  Config::setShowEditWidget(m_toggleEntryEditor->isChecked());
  Config::setShowEntryView(m_toggleEntryView->isChecked());

  KConfigGroup filesConfig(KGlobal::config(), "Recent Files");
  m_fileOpenRecent->saveEntries(filesConfig);
  if(!isNewDocument()) {
    Config::setLastOpenFile(Data::Document::self()->URL().url());
  }

  if(!m_groupView->isHidden()) {
    Config::setMainSplitterSizes(m_split->sizes());
  }
  if(!m_viewStack->isHidden()) {
    // badly named option, but no need to change
    Config::setSecondarySplitterSizes(m_rightSplit->sizes());
  }

  // historical reasons
  const int sortColumn = m_groupView->sortRole() == Qt::DisplayRole ? 0 : 1;
  Config::setGroupViewSortColumn(sortColumn); // ok to use SortColumn key, save semantics
  Config::setGroupViewSortAscending(m_groupView->sortOrder() == Qt::AscendingOrder);

  if(m_loanView) {
    const int sortColumn = m_loanView->sortRole() == Qt::DisplayRole ? 0 : 1;
    Config::setLoanViewSortAscending(sortColumn); // ok to use SortColumn key, save semantics
    Config::setLoanViewSortAscending(m_loanView->sortOrder() == Qt::AscendingOrder);
  }

  if(m_filterView) {
    const int sortColumn = m_filterView->sortRole() == Qt::DisplayRole ? 0 : 1;
    Config::setFilterViewSortAscending(sortColumn); // ok to use SortColumn key, save semantics
    Config::setFilterViewSortAscending(m_filterView->sortOrder() == Qt::AscendingOrder);
  }

  // this is used in the EntryEditDialog constructor, too
  KConfigGroup editDialogConfig(KGlobal::config(), "Edit Dialog Options");
  m_editDialog->saveDialogSize(editDialogConfig);

  saveCollectionOptions(Data::Document::self()->collection());
  Config::self()->writeConfig();
}

void MainWindow::readCollectionOptions(Tellico::Data::CollPtr coll_) {
  KConfigGroup group(KGlobal::config(), QString::fromLatin1("Options - %1").arg(coll_->typeName()));

  QString defaultGroup = coll_->defaultGroupField();
  QString entryGroup;
  if(coll_->type() != Data::Collection::Base) {
    entryGroup = group.readEntry("Group By", defaultGroup);
  } else {
    KUrl url = Kernel::self()->URL();
    for(int i = 0; i < Config::maxCustomURLSettings(); ++i) {
      KUrl u = group.readEntry(QString::fromLatin1("URL_%1").arg(i));
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

void MainWindow::saveCollectionOptions(Tellico::Data::CollPtr coll_) {
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
    KUrl url = Kernel::self()->URL();
    QList<KUrl> urls = QList<KUrl>() << url;
    QStringList groupBys = QStringList() << groupName;
    for(int i = 0; i < Config::maxCustomURLSettings(); ++i) {
      KUrl u = config.readEntry(QString::fromLatin1("URL_%1").arg(i), KUrl());
      QString g = config.readEntry(QString::fromLatin1("Group By_%1").arg(i), QString());
      if(!u.isEmpty() && url != u) {
        urls.append(u);
        groupBys.append(g);
      } else if(!u.isEmpty()) {
        configIndex = i;
      }
    }
    int limit = qMin(urls.count(), Config::maxCustomURLSettings());
    for(int i = 0; i < limit; ++i) {
      config.writeEntry(QString::fromLatin1("URL_%1").arg(i), urls[i].url());
      config.writeEntry(QString::fromLatin1("Group By_%1").arg(i), groupBys[i]);
    }
  }
  m_detailedView->saveConfig(coll_, configIndex);
}

void MainWindow::readOptions() {
  KConfigGroup mainWindowConfig(KGlobal::config(), "Main Window Options");
  applyMainWindowSettings(mainWindowConfig);

  QList<int> splitList = Config::mainSplitterSizes();
  if(!splitList.empty()) {
    m_split->setSizes(splitList);
  }

  splitList = Config::secondarySplitterSizes();
  if(!splitList.empty()) {
    m_rightSplit->setSizes(splitList);
  }

  m_viewStack->iconView()->setMaxAllowedIconWidth(Config::maxIconSize());

  connect(toolBar("collectionToolBar"), SIGNAL(iconSizeChanged(const QSize&)), SLOT(slotUpdateToolbarIcons()));

  m_toggleGroupWidget->setChecked(Config::showGroupWidget());
  slotToggleGroupWidget();

  m_toggleEntryView->setChecked(Config::showEntryView());
  slotToggleEntryView();

  // initialize the recent file list
  KConfigGroup filesConfig(KGlobal::config(), "Recent Files");
  m_fileOpenRecent->loadEntries(filesConfig);

  // sort by count if column = 1
  int sortRole = Config::groupViewSortColumn() == 0 ? static_cast<int>(Qt::DisplayRole) : static_cast<int>(RowCountRole);
  Qt::SortOrder sortOrder = Config::groupViewSortAscending() ? Qt::AscendingOrder : Qt::DescendingOrder;
  m_groupView->setSorting(sortOrder, sortRole);

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

void MainWindow::saveProperties(KConfigGroup& cfg_) {
  if(!isNewDocument() && !Data::Document::self()->isModified()) {
    // saving to tempfile not necessary
  } else {
    KUrl url = Data::Document::self()->URL();
    cfg_.writeEntry("filename", url.url());
    cfg_.writeEntry("modified", Data::Document::self()->isModified());
//    QString tempname = KUrl::encode_string(kapp->temporarySaveName(url.url()));
    QString tempname = kapp->tempSaveName(url.url());
    KUrl tempurl;
    tempurl.setPath(tempname);
    Data::Document::self()->saveDocument(tempurl);
  }
}

void MainWindow::readProperties(const KConfigGroup& cfg_) {
  QString filename = cfg_.readEntry(QString::fromLatin1("filename"), QString());
  if(filename.isEmpty()) {
    return;
  }
  bool modified = cfg_.readEntry(QString::fromLatin1("modified"), false);
  if(modified) {
    bool canRecover;
    QString tempname = kapp->checkRecoverFile(filename, canRecover);

    if(canRecover) {
      KUrl tempurl;
      tempurl.setPath(tempname);
      Data::Document::self()->openDocument(tempurl);
      Data::Document::self()->slotSetModified(true);
      updateCaption(true);
      QFile::remove(tempname);
    }
  } else {
    KUrl url;
    url.setPath(filename);
    Data::Document::self()->openDocument(url);
    updateCaption(false);
  }
}

bool MainWindow::queryClose() {
  // in case we're still loading the images, cancel that
  Data::Document::self()->cancelImageWriting();
  return m_editDialog->queryModified() && Data::Document::self()->saveModified();
}

bool MainWindow::queryExit() {
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
      m_viewTabs->removeTab(m_viewTabs->indexOf(m_filterView));
      Controller::self()->removeObserver(m_filterView);
      delete m_filterView;
      m_filterView = 0;
    }
    if(m_loanView) {
      m_viewTabs->removeTab(m_viewTabs->indexOf(m_loanView));
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
    KUrl url = KFileDialog::getOpenUrl(KUrl(QString::fromLatin1("kfiledialog:///open")), filter,
                                       this, i18n("Open File"));
    if(!url.isEmpty() && url.isValid()) {
      slotFileOpen(url);
    }
  }
  StatusBar::self()->clearStatus();
}

void MainWindow::slotFileOpen(const KUrl& url_) {
  slotStatusMsg(i18n("Opening file..."));

  // close the fields dialog
  slotHideCollectionFieldsDialog();

  // there seems to be a race condition at start between slotInit() and initFileOpen()
  // which means the edit dialog might not have been created yet
  if((!m_editDialog || m_editDialog->queryModified()) && Data::Document::self()->saveModified()) {
    if(openURL(url_)) {
      m_fileOpenRecent->addUrl(url_);
      m_fileOpenRecent->setCurrentItem(-1);
    }
  }

  StatusBar::self()->clearStatus();
}

void MainWindow::slotFileOpenRecent(const KUrl& url_) {
  slotStatusMsg(i18n("Opening file..."));

  // close the fields dialog
  slotHideCollectionFieldsDialog();

  if(m_editDialog->queryModified() && Data::Document::self()->saveModified()) {
    if(!openURL(url_)) {
      m_fileOpenRecent->removeUrl(url_);
      m_fileOpenRecent->setCurrentItem(-1);
    }
  } else {
    // the KAction shouldn't be checked now
    m_fileOpenRecent->setCurrentItem(-1);
  }

  StatusBar::self()->clearStatus();
}

void MainWindow::openFile(const QString& file_) {
  KUrl url(file_);
  if(!url.isEmpty() && url.isValid()) {
    slotFileOpen(url);
  }
}

bool MainWindow::openURL(const KUrl& url_) {
  // try to open document
  GUI::CursorSaver cs(Qt::WaitCursor);

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
  if(m_filterView && m_filterView->isEmpty()) {
    m_viewTabs->removeTab(m_viewTabs->indexOf(m_filterView));
    Controller::self()->removeObserver(m_filterView);
    delete m_filterView;
    m_filterView = 0;
  }
  if(m_loanView && m_loanView->isEmpty()) {
    m_viewTabs->removeTab(m_viewTabs->indexOf(m_loanView));
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

    GUI::CursorSaver cs(Qt::WaitCursor);
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
  KUrl url = KFileDialog::getSaveUrl(KUrl(QString::fromLatin1("kfiledialog:///open")), filter, this, i18n("Save As"));

  if(url.isEmpty()) {
    StatusBar::self()->clearStatus();
    return false;
  }

  bool ret = true;
  if(url.isValid()) {
    GUI::CursorSaver cs(Qt::WaitCursor);
    if(Data::Document::self()->saveDocument(url)) {
      KRecentDocument::add(url);
      m_fileOpenRecent->addUrl(url);
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
  if(m_detailedView->filter()) {
    QString str = i18n("The collection is currently being filtered to show a limited subset of "
                       "the entries. Only the visible entries will be printed. Continue?");
    int ret = KMessageBox::warningContinueCancel(this, str, QString::null, KStandardGuiItem::print(),
                                                 KStandardGuiItem::cancel(), QString::fromLatin1("WarnPrintVisible"));
    if(ret == KMessageBox::Cancel) {
      StatusBar::self()->clearStatus();
      return;
    }
  }

  GUI::CursorSaver cs(Qt::WaitCursor);

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
//  myDebug() << html;
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
    const QMetaObject* meta = w->metaObject();

    int idx = meta->indexOfSlot(slot_);
    if(idx > -1) {
      myDebug() << "MainWindow invoking" << meta->method(idx).signature();
      QMetaObject::invokeMethod(w, slot_);
    }
  }
}

void MainWindow::slotEditSelectAll() {
  m_detailedView->selectAllVisible();
}

void MainWindow::slotEditDeselect() {
  Controller::self()->slotUpdateSelection(0, Data::EntryList());
}

void MainWindow::slotConfigToolbar() {
  KConfigGroup config(KGlobal::config(), QString::fromLatin1("Main Window Options"));
  saveMainWindowSettings(config);
#ifdef UIFILE
  KEditToolBar dlg(actionCollection(), UIFILE);
#else
  KEditToolBar dlg(actionCollection());
#endif
  connect(&dlg, SIGNAL(newToolbarConfig()), this, SLOT(slotNewToolbarConfig()));
  dlg.exec();
}

void MainWindow::slotNewToolbarConfig() {
  KConfigGroup config(KGlobal::config(), QString::fromLatin1("Main Window Options"));
  applyMainWindowSettings(config);
#ifdef UIFILE
  createGUI(UIFILE);
#else
  createGUI();
#endif
}

void MainWindow::slotConfigKeys() {
  KShortcutsDialog::configure(actionCollection());
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
    KWindowSystem::activateWindow(m_configDlg->winId());
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
  QString tipfile = KStandardDirs::locate("appdata", QString::fromLatin1("tellico.tips"));
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
  QString text = i18n("Total entries: %1", count);

  int selectCount = Controller::self()->selectedEntries().count();
  int filterCount = m_detailedView->visibleItems();
  // if more than one book is selected, add the number of selected books
  if(filterCount < count && selectCount > 1) {
    text += QChar(' ');
    text += i18n("(%1 filtered; %2 selected)", filterCount, selectCount);
  } else if(filterCount < count) {
    text += QChar(' ');
    text += i18n("(%1 filtered)", filterCount);
  } else if(selectCount > 1) {
    text += QChar(' ');
    text += i18n("(%1 selected)", selectCount);
  }

  m_statusBar->setCount(text);
}

void MainWindow::slotEnableOpenedActions() {
  slotUpdateToolbarIcons();

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

void MainWindow::slotUpdateCollectionToolBar(Tellico::Data::CollPtr coll_) {
  if(!coll_) {
    myWarning() << "no collection pointer!";
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
  foreach(const QString& groupName, groups) {
    // special case for people "pseudo-group"
    if(groupName == Data::Collection::s_peopleGroupName) {
      groupMap.insert(groupName, '<' + i18n("People") + '>');
    } else {
      groupMap.insert(groupName, coll_->fieldTitleByName(groupName));
    }
  }

  const QStringList names = groupMap.keys();
  int index = names.indexOf(current);
  if(index == -1) {
    current = names[0];
    index = 0;
  }
  const QStringList titles = groupMap.values();
  m_entryGrouping->setItems(titles);
  m_entryGrouping->setCurrentItem(index);
  // in case the current grouping field get modified to be non-grouping...
  m_groupView->setGroupField(current); // don't call slotChangeGrouping() since it adds an undo item

  // this isn't really proper, but works so the combo box width gets adjusted
#ifndef WIN32
#warning MainWindow containerCount() is not ported
#endif
#if 0
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
#endif
}

void MainWindow::slotChangeGrouping() {
//  DEBUG_LINE;
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
  m_viewTabs->setCurrentWidget(m_groupView);
}

void MainWindow::slotShowReportDialog() {
//  DEBUG_LINE;
  if(!m_reportDlg) {
    m_reportDlg = new ReportDialog(this);
    connect(m_reportDlg, SIGNAL(finished()),
            SLOT(slotHideReportDialog()));
  } else {
    KWindowSystem::activateWindow(m_reportDlg->winId());
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
  KHTMLPart w;
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
  w.view()->print();
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
            this, SLOT(slotClearFilterNow()));
    connect(m_filterDlg, SIGNAL(signalUpdateFilter(Tellico::FilterPtr)),
            Controller::self(), SLOT(slotUpdateFilter(Tellico::FilterPtr)));
    connect(m_filterDlg, SIGNAL(finished()),
            SLOT(slotHideFilterDialog()));
  } else {
    KWindowSystem::activateWindow(m_filterDlg->winId());
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
  if (m_dontQueueFilter) {
    return;
  }
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

void MainWindow::slotClearFilterNow() {
  // Can't just block signals because clear button won't show then
  m_dontQueueFilter = true;
  m_quickFilter->setText(" "); // To be able to clear custom filter
  setFilter(QString());
  m_dontQueueFilter = false;
}

void MainWindow::setFilter(const QString& text_) {
  QString text = text_.trimmed();
  FilterPtr filter(0);
  if(!text.isEmpty()) {
    filter.attach(new Filter(Filter::MatchAll));
    QString fieldName; // empty field name means match on any field
    // if the text contains '=' assume it's a field name or title
    if(text.indexOf('=') > -1) {
        fieldName = text.section('=', 0, 0).trimmed();
        text = text.section('=', 1).trimmed();
        // check that the field name might be a title
        if(!Data::Document::self()->collection()->hasField(fieldName)) {
        fieldName = Data::Document::self()->collection()->fieldNameByTitle(fieldName);
        }
    }
    // if the text contains any non-word characters, assume it's a regexp
    // but \W in qt is letter, number, or '_', I want to be a bit less strict
    QRegExp rx(QString::fromLatin1("[^\\w\\s-']"));
    if(text.indexOf(rx) == -1) {
    // split by whitespace, and add rules for each word
        const QStringList tokens = text.split(QRegExp(QString::fromLatin1("\\s")));
        foreach(const QString& token, tokens) {
        // an empty field string means check every field
        filter->append(new FilterRule(fieldName, token, FilterRule::FuncContains));
        }
    } else {
        // if it isn't valid, hold off on applying the filter
        QRegExp tx(text);
        if(!tx.isValid()) {
        text = QRegExp::escape(text);
        tx.setPattern(text);
        }
        if(!tx.isValid()) {
        myDebug() << "invalid regexp:" << text;
        return;
        }
        filter->append(new FilterRule(fieldName, text, FilterRule::FuncRegExp));
    }
    // also want to update the line edit in case the filter was set by DCOP
    if(m_quickFilter->text() != text_) {
        m_quickFilter->setText(text_);
    }
  }
  // only update filter if one exists or did exist
  if(!filter.isNull() || m_detailedView->filter()) {
    Controller::self()->slotUpdateFilter(filter);
  }
}

void MainWindow::slotShowCollectionFieldsDialog() {
  if(!m_collFieldsDlg) {
    m_collFieldsDlg = new CollectionFieldsDialog(Data::Document::self()->collection(), this);
    connect(m_collFieldsDlg, SIGNAL(finished()),
            SLOT(slotHideCollectionFieldsDialog()));
  } else {
    KWindowSystem::activateWindow(m_collFieldsDlg->winId());
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
  KUrl url;
  switch(ImportDialog::importTarget(format)) {
    case Import::File:
      url = KFileDialog::getOpenUrl(KUrl(ImportDialog::startDir(format)), ImportDialog::fileFilter(format),
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
    bool ok = !url.isEmpty() && url.isValid() && KIO::NetAccess::exists(url, KIO::NetAccess::SourceSide, this);
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
  ExportDialog dlg(format, Data::Document::self()->collection(), this);

  if(dlg.exec() == QDialog::Rejected) {
    StatusBar::self()->clearStatus();
    return;
  }

  switch(ExportDialog::exportTarget(format)) {
    case Export::None:
      dlg.exportURL();
      break;

    case Export::Dir:
      myDebug() << "ExportDir not implemented!";
      break;

    case Export::File:
    {
      KUrl url = KFileDialog::getSaveUrl(KUrl(QString::fromLatin1("kfiledialog:///export")),
                                         dlg.fileFilter(), this, i18n("Export As"));
      if(url.isEmpty()) {
        StatusBar::self()->clearStatus();
        return;
      }

      if(url.isValid()) {
        GUI::CursorSaver cs(Qt::WaitCursor);
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
    m_stringMacroDlg = new StringMapDialog(c->macroList(), this, false);
    m_stringMacroDlg->setCaption(i18n("String Macros"));
    m_stringMacroDlg->setLabels(i18n("Macro"), i18n("String"));
    connect(m_stringMacroDlg, SIGNAL(finished()), SLOT(slotHideStringMacroDialog()));
    connect(m_stringMacroDlg, SIGNAL(okClicked()), SLOT(slotStringMacroDialogOk()));
  } else {
    KWindowSystem::activateWindow(m_stringMacroDlg->winId());
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

  KWindowSystem::activateWindow(m_editDialog->winId());
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
    myWarning() << "ERROR: no bibliography created!";
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
    KWindowSystem::activateWindow(m_fetchDlg->winId());
  }
  m_fetchDlg->show();
}

void MainWindow::slotHideFetchDialog() {
  if(m_fetchDlg) {
    m_fetchDlg->delayedDestruct();
    m_fetchDlg = 0;
  }
}

bool MainWindow::importFile(Tellico::Import::Format format_, const KUrl& url_, Tellico::Import::Action action_) {
  // try to open document
  GUI::CursorSaver cs(Qt::WaitCursor);

  bool failed = false;
  Data::CollPtr coll;
  if(!url_.isEmpty() && url_.isValid() && KIO::NetAccess::exists(url_, KIO::NetAccess::SourceSide, this)) {
    coll = ImportDialog::importURL(format_, url_);
  } else {
    Kernel::self()->sorry(i18n(errorLoad, url_.fileName()));
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

bool MainWindow::exportCollection(Tellico::Export::Format format_, const KUrl& url_) {
  if(!url_.isValid()) {
    myDebug() << "invalid URL:" << url_;
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
  return entry;
}

void MainWindow::addFilterView() {
  if(m_filterView) {
    return;
  }

  m_filterView = new FilterView(m_viewTabs);
  Controller::self()->addObserver(m_filterView);
  m_viewTabs->insertTab(1, m_filterView, KIcon(QString::fromLatin1("view-filter")), i18n("Filters"));
  m_filterView->setWhatsThis(i18n("<qt>The <i>Filter View</i> shows the entries which meet certain "
                                  "filter rules.</qt>"));

  // sort by count if column = 1
  int sortRole = Config::filterViewSortColumn() == 0 ? static_cast<int>(Qt::DisplayRole) : static_cast<int>(RowCountRole);
  Qt::SortOrder sortOrder = Config::filterViewSortAscending() ? Qt::AscendingOrder : Qt::DescendingOrder;
  m_filterView->setSorting(sortOrder, sortRole);
}

void MainWindow::addLoanView() {
  if(m_loanView) {
    return;
  }

  m_loanView = new LoanView(m_viewTabs);
  Controller::self()->addObserver(m_loanView);
  m_viewTabs->insertTab(2, m_loanView, KIcon(QString::fromLatin1("kaddressbook")), i18n("Loans"));
  m_loanView->setWhatsThis(i18n("<qt>The <i>Loan View</i> shows a list of all the people who "
                                "have borrowed items from your collection.</qt>"));

  // sort by count if column = 1
  int sortRole = Config::loanViewSortColumn() == 0 ? static_cast<int>(Qt::DisplayRole) : static_cast<int>(RowCountRole);
  Qt::SortOrder sortOrder = Config::loanViewSortAscending() ? Qt::AscendingOrder : Qt::DescendingOrder;
  m_loanView->setSorting(sortOrder, sortRole);
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
    KUrl u = Data::Document::self()->URL();
    if(u.isLocalFile()) {
      // for new files, the path is set to /Untitled in Data::Document
      if(u.path() == '/' + i18n("Untitled")) {
        caption += u.fileName();
      } else {
        caption += u.path();
      }
    } else {
      caption += u.prettyUrl();
    }
  }
  setCaption(caption, modified_);
}

void MainWindow::slotUpdateToolbarIcons() {
  // first change the icon for the menu item
  m_newEntry->setIcon(KIcon(Kernel::self()->collectionTypeName()));

#ifndef WIN32
#warning MainWindow::slotUpdateToolbarIcons
#endif
#if 0
  // since the toolbar icon is probably a different size than the menu item icon
  // superimpose it on the "mime_empty" icon
  KToolBar* tb = toolBar("collectionToolBar");
  if(!tb) {
    return;
  }

  for(int i = 0; i < tb->count(); ++i) {
    if(m_newEntry->isPlugged(tb, tb->idAt(i))) {
      QIcon icons;
      icons.installIconFactory(new EntryIconFactory(tb->iconSize()));
      tb->setButtonIcon(tb->idAt(i), icons);
      break;
    }
  }
#endif
}

void MainWindow::slotGroupLabelActivated() {
  // need entry grouping combo id
  KToolBar* tb = toolBar("collectionToolBar");
  if(!tb) {
    return;
  }

  QComboBox* combo = ::qobject_cast<QComboBox*>(tb->widgetForAction(m_entryGrouping));
  if(combo) {
    combo->showPopup();
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
  unplugActionList(QString::fromLatin1("update_entry_actions"));
  foreach(QAction* action, m_fetchActions) {
    foreach(QWidget* widget, action->associatedWidgets()) {
      widget->removeAction(action);
    }
    m_updateMapper->removeMappings(action);
  }
  qDeleteAll(m_fetchActions);
  m_fetchActions.clear();

  Fetch::FetcherVec vec = Fetch::Manager::self()->fetchers(Kernel::self()->collectionType());
  foreach(Fetch::Fetcher::Ptr fetcher, vec) {
    KAction* action = new KAction(KIcon(Fetch::Manager::fetcherIcon(fetcher)), fetcher->source(), actionCollection());
    action->setToolTip(i18n("Update entry data from %1", fetcher->source()));
    connect(action, SIGNAL(activated()), m_updateMapper, SLOT(map()));
    m_updateMapper->setMapping(action, fetcher->source());
    m_fetchActions.append(action);
  }

  plugActionList(QString::fromLatin1("update_entry_actions"), m_fetchActions);
}

void MainWindow::importFile(Tellico::Import::Format format_, const KUrl::List& urls_) {
  KUrl::List urls = urls_;
  // update as DropHandler and Importer classes are updated
  if(urls_.count() > 1 &&
     format_ != Import::Bibtex &&
     format_ != Import::RIS &&
     format_ != Import::PDF) {
    KUrl u = urls_.front();
    QString url = u.isLocalFile() ? u.path() : u.prettyUrl();
    Kernel::self()->sorry(i18n("Tellico can only import one file of this type at a time. "
                               "Only %1 will be imported.").arg(url));
    urls.clear();
    urls = u;
  }

  ImportDialog dlg(format_, urls, this);
  if(dlg.exec() != QDialog::Accepted) {
    return;
  }

//  if edit dialog is saved ok and if replacing, then the doc is saved ok
  if(m_editDialog->queryModified() &&
     (dlg.action() != Import::Replace || Data::Document::self()->saveModified())) {
    GUI::CursorSaver cs(Qt::WaitCursor);
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

bool MainWindow::importCollection(Tellico::Data::CollPtr coll_, Tellico::Import::Action action_) {
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

void MainWindow::slotURLAction(const KUrl& url_) {
  Q_ASSERT(url_.protocol() == QLatin1String("tc"));
  QString actionName = url_.fileName();
  QAction* action = this->action(actionName.toLatin1());
  if(action) {
    action->activate(QAction::Trigger);
  } else {
    myWarning() << "MainWindow::slotURLAction() - unknown action: " << actionName;
  }
}

bool MainWindow::eventFilter(QObject* obj_, QEvent* ev_) {
  if(ev_->type() == QEvent::KeyPress && obj_ == m_quickFilter) {
    switch(static_cast<QKeyEvent*>(ev_)->key()) {
      case Qt::Key_Escape:
        m_quickFilter->clear();
        return true;
    }
  }
  return false;
}

#include "mainwindow.moc"
