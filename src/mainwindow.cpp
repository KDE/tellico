/***************************************************************************
    Copyright (C) 2001-2020 Robby Stephenson <robby@periapsis.org>
    Copyright (C) 2011 Pedro Miguel Carvalho <kde@pmc.com.pt>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                     *
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

#include "mainwindow.h"
#include "tellico_kernel.h"
#include "document.h"
#include "detailedlistview.h"
#include "entryeditdialog.h"
#include "groupview.h"
#include "viewstack.h"
#include "collection.h"
#include "collectionfactory.h"
#include "entry.h"
#include "configdialog.h"
#include "filter.h"
#include "filterparser.h"
#include "filterdialog.h"
#include "collectionfieldsdialog.h"
#include "controller.h"
#include "importdialog.h"
#include "exportdialog.h"
#include "core/filehandler.h" // needed so static mainWindow variable can be set
#include "core/logger.h"
#include "printhandler.h"
#include "entryview.h"
#include "entryiconview.h"
#include "images/imagefactory.h" // needed so tmp files can get cleaned
#include "collections/collectioninitializer.h"
#include "collections/bibtexcollection.h" // needed for bibtex string macro dialog
#include "utils/bibtexhandler.h" // needed for bibtex options
#include "utils/datafileregistry.h"
#include "fetchdialog.h"
#include "reportdialog.h"
#include "bibtexkeydialog.h"
#include "core/tellico_strings.h"
#include "filterview.h"
#include "loanview.h"
#include "fetch/fetchmanager.h"
#include "fetch/fetcherinitializer.h"
#include "cite/actionmanager.h"
#include "config/tellico_config.h"
#include "core/netaccess.h"
#include "dbusinterface.h"
#include "models/models.h"
#include "models/entryiconmodel.h"
#include "models/entryselectionmodel.h"
#include "newstuff/manager.h"
#include "gui/drophandler.h"
#include "gui/stringmapdialog.h"
#include "gui/lineedit.h"
#include "gui/statusbar.h"
#include "gui/tabwidget.h"
#include "gui/dockwidget.h"
#include "gui/collectiontemplatedialog.h"
#include "utils/cursorsaver.h"
#include "utils/guiproxy.h"
#include "utils/tellico_utils.h"
#include "tellico_debug.h"

#include <KComboBox>
#include <KToolBar>
#include <KLocalizedString>
#include <KConfig>
#include <KStandardAction>
#include <KWindowConfig>
#include <KMessageBox>
#include <KRecentDocument>
#include <KRecentDirs>
#include <KEditToolBar>
#include <KShortcutsDialog>
#include <KRecentFilesAction>
#include <KToggleAction>
#include <KActionCollection>
#include <KActionMenu>
#include <KFileWidget>
#include <KDualAction>
#include <KXMLGUIFactory>
#include <KAboutData>
#include <KIconLoader>
#include <kwidgetsaddons_version.h>

#define HAVE_STYLE_MANAGER __has_include(<KStyleManager>)
#if HAVE_STYLE_MANAGER
#include <KStyleManager>
#endif

#include <QApplication>
#include <QUndoStack>
#include <QAction>
#include <QSignalMapper>
#include <QTimer>
#include <QMetaObject> // needed for copy, cut, paste slots
#include <QMimeDatabase>
#include <QMimeType>
#include <QMenuBar>
#include <QFileDialog>
#include <QMetaMethod>
#include <QVBoxLayout>
#include <QTextEdit>

namespace {
  static const int MAX_IMAGES_WARN_PERFORMANCE = 200;

QIcon mimeIcon(const char* s) {
  QMimeDatabase db;
  QMimeType ptr = db.mimeTypeForName(QLatin1String(s));
  if(!ptr.isValid()) {
    myDebug() << "*** no icon for" << s;
  }
  return ptr.isValid() ? QIcon::fromTheme(ptr.iconName()) : QIcon();
}

QIcon mimeIcon(const char* s1, const char* s2) {
  QMimeDatabase db;
  QMimeType ptr = db.mimeTypeForName(QLatin1String(s1));
  if(!ptr.isValid()) {
    ptr = db.mimeTypeForName(QLatin1String(s2));
    if(!ptr.isValid()) {
      myDebug() << "*** no icon for" << s1 << "or" << s2;
    }
  }
  return ptr.isValid() ? QIcon::fromTheme(ptr.iconName()) : QIcon();
}

}

using namespace Tellico;
using Tellico::MainWindow;

MainWindow::MainWindow(QWidget* parent_/*=0*/) : KXmlGuiWindow(parent_),
    m_updateAll(nullptr),
    m_statusBar(nullptr),
    m_editDialog(nullptr),
    m_groupView(nullptr),
    m_filterView(nullptr),
    m_loanView(nullptr),
    m_configDlg(nullptr),
    m_filterDlg(nullptr),
    m_collFieldsDlg(nullptr),
    m_stringMacroDlg(nullptr),
    m_bibtexKeyDlg(nullptr),
    m_fetchDlg(nullptr),
    m_reportDlg(nullptr),
    m_printHandler(nullptr),
    m_queuedFilters(0),
    m_initialized(false),
    m_newDocument(true),
    m_dontQueueFilter(false),
    m_savingImageLocationChange(false) {

  Controller::init(this); // the only time this is ever called!
  // has to be after controller init
  Kernel::init(this); // the only time this is ever called!
  GUI::Proxy::setMainWidget(this);

  setWindowIcon(QIcon::fromTheme(QStringLiteral("tellico"),
                                 QIcon(QLatin1String(":/icons/tellico"))));

  // initialize the status bar and progress bar
  initStatusBar();

  // initialize all the collection types
  // which must be done before the document is created
  CollectionInitializer initCollections;
  // register all the fetcher types
  Fetch::FetcherInitializer initFetchers;

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

  MARK_LINE;
  QTimer::singleShot(0, this, &MainWindow::slotInit);
}

MainWindow::~MainWindow() {
  qDeleteAll(m_fetchActions);
  m_fetchActions.clear();
  // when closing the mainwindow, immediately after running Tellico, often there was a long pause
  // before the application eventually quit, something related to polling on eventfd, I don't
  // know what. So when closing the window, make sure to immediately quit the application
  QTimer::singleShot(0, qApp, &QCoreApplication::quit);
}

void MainWindow::slotInit() {
  // if the edit dialog exists, we know we've already called this function
  if(m_editDialog) {
    return;
  }
  MARK;

  m_editDialog = new EntryEditDialog(this);
  Controller::self()->addObserver(m_editDialog);

  m_toggleEntryEditor->setChecked(Config::showEditWidget());
  slotToggleEntryEditor();
  m_lockLayout->setActive(Config::lockLayout());

  initConnections();
  connect(ImageFactory::self(), &ImageFactory::imageLocationMismatch,
          this, &MainWindow::slotImageLocationMismatch);
  // Init DBUS for new stuff manager
  NewStuff::Manager::self();
}

void MainWindow::initStatusBar() {
  MARK;
  m_statusBar = new Tellico::StatusBar(this);
  setStatusBar(m_statusBar);
}

void MainWindow::initActions() {
  MARK;
  /*************************************************
   * File->New menu
   *************************************************/
  QSignalMapper* collectionMapper = new QSignalMapper(this);
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
  void (QSignalMapper::* mappedInt)(int) = &QSignalMapper::mapped;
  connect(collectionMapper, mappedInt, this, &MainWindow::slotFileNew);
#else
  connect(collectionMapper, &QSignalMapper::mappedInt, this, &MainWindow::slotFileNew);
#endif

  m_newCollectionMenu = new KActionMenu(i18n("New Collection"), this);
  m_newCollectionMenu->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
  m_newCollectionMenu->setToolTip(i18n("Create a new collection"));
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5,77,0)
  m_newCollectionMenu->setPopupMode(QToolButton::InstantPopup);
#else
  m_newCollectionMenu->setDelayed(false);
#endif
  actionCollection()->addAction(QStringLiteral("file_new_collection"), m_newCollectionMenu);

  QAction* action;

  void (QSignalMapper::* mapVoid)() = &QSignalMapper::map;
#define COLL_ACTION(TYPE, NAME, TEXT, TIP, ICON) \
  action = actionCollection()->addAction(QStringLiteral(NAME), collectionMapper, mapVoid); \
  action->setText(TEXT); \
  action->setToolTip(TIP); \
  action->setIcon(QIcon(QStringLiteral(":/icons/" ICON))); \
  m_newCollectionMenu->addAction(action); \
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

  COLL_ACTION(Game, "new_game_collection", i18n("New Video &Game Collection"),
              i18n("Create a new video game collection"), "game");

  COLL_ACTION(BoardGame, "new_boardgame_collection", i18n("New Boa&rd Game Collection"),
              i18n("Create a new board game collection"), "boardgame");

  COLL_ACTION(File, "new_file_catalog", i18n("New &File Catalog"),
              i18n("Create a new file catalog"), "file");

  action = actionCollection()->addAction(QStringLiteral("new_custom_collection"), collectionMapper, mapVoid);
  action->setText(i18n("New C&ustom Collection"));
  action->setToolTip(i18n("Create a new custom collection"));
  action->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
  m_newCollectionMenu->addAction(action);
  collectionMapper->setMapping(action, Data::Collection::Base);

#undef COLL_ACTION

  /*************************************************
   * File menu
   *************************************************/
  action = KStandardAction::open(this, SLOT(slotFileOpen()), actionCollection());
  action->setToolTip(i18n("Open an existing document"));
  m_fileOpenRecent = KStandardAction::openRecent(this, SLOT(slotFileOpenRecent(const QUrl&)), actionCollection());
  m_fileOpenRecent->setToolTip(i18n("Open a recently used file"));
  m_fileSave = KStandardAction::save(this, SLOT(slotFileSave()), actionCollection());
  m_fileSave->setToolTip(i18n("Save the document"));
  action = KStandardAction::saveAs(this, SLOT(slotFileSaveAs()), actionCollection());
  action->setToolTip(i18n("Save the document as a different file..."));

  action = actionCollection()->addAction(QStringLiteral("file_save_template"),
                                         this, SLOT(slotFileSaveAsTemplate()));
  action->setText(i18n("Save As Template..."));
  action->setIcon(QIcon::fromTheme(QStringLiteral("document-save-as-template")));
  action->setToolTip(i18n("Save as a collection template"));

  action = KStandardAction::print(this, SLOT(slotFilePrint()), actionCollection());
  action->setToolTip(i18n("Print the contents of the collection..."));
#ifdef USE_KHTML
  {
    KHTMLPart w;
    // KHTMLPart printing was broken in KDE until KHTML 5.16
    const QString version =  w.componentData().version();
    const uint major = version.section(QLatin1Char('.'), 0, 0).toUInt();
    const uint minor = version.section(QLatin1Char('.'), 1, 1).toUInt();
    if(major == 5 && minor < 16) {
      myWarning() << "Printing is broken for KDE Frameworks < 5.16. Please upgrade";
      action->setEnabled(false);
    }
  }
#else
  // print preview is only available with QWebEngine
  action = KStandardAction::printPreview(this, SLOT(slotFilePrintPreview()), actionCollection());
  action->setToolTip(i18n("Preview the contents of the collection..."));
#endif

  action = KStandardAction::quit(this, SLOT(slotFileQuit()), actionCollection());
  action->setToolTip(i18n("Quit the application"));

/**************** Import Menu ***************************/

  QSignalMapper* importMapper = new QSignalMapper(this);
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
  connect(importMapper, mappedInt, this, &MainWindow::slotFileImport);
#else
  connect(importMapper, &QSignalMapper::mappedInt, this, &MainWindow::slotFileImport);
#endif

  KActionMenu* importMenu = new KActionMenu(i18n("&Import"), this);
  importMenu->setIcon(QIcon::fromTheme(QStringLiteral("document-import")));
  importMenu->setToolTip(i18n("Import the collection data from other formats"));
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5,77,0)
  importMenu->setPopupMode(QToolButton::InstantPopup);
#else
  importMenu->setDelayed(false);
#endif
  actionCollection()->addAction(QStringLiteral("file_import"), importMenu);

#define IMPORT_ACTION(TYPE, NAME, TEXT, TIP, ICON) \
  action = actionCollection()->addAction(QStringLiteral(NAME), importMapper, mapVoid); \
  action->setText(TEXT); \
  action->setToolTip(TIP); \
  action->setIcon(ICON); \
  importMenu->addAction(action); \
  importMapper->setMapping(action, TYPE);

  IMPORT_ACTION(Import::TellicoXML, "file_import_tellico", i18n("Import Tellico Data..."),
                i18n("Import another Tellico data file"),
                QIcon::fromTheme(QStringLiteral("tellico"), QIcon(QLatin1String(":/icons/tellico"))));

  IMPORT_ACTION(Import::CSV, "file_import_csv", i18n("Import CSV Data..."),
                i18n("Import a CSV file"), mimeIcon("text/csv", "text/x-csv"));

  IMPORT_ACTION(Import::MODS, "file_import_mods", i18n("Import MODS Data..."),
                i18n("Import a MODS data file"), mimeIcon("text/xml"));

  IMPORT_ACTION(Import::Alexandria, "file_import_alexandria", i18n("Import Alexandria Data..."),
                i18n("Import data from the Alexandria book collection manager"),
                QIcon::fromTheme(QStringLiteral("alexandria"), QIcon(QLatin1String(":/icons/alexandria"))));

  IMPORT_ACTION(Import::Delicious, "file_import_delicious", i18n("Import Delicious Library Data..."),
                i18n("Import data from Delicious Library"),
                QIcon::fromTheme(QStringLiteral("deliciouslibrary"), QIcon(QLatin1String(":/icons/deliciouslibrary"))));

  IMPORT_ACTION(Import::Collectorz, "file_import_collectorz", i18n("Import Collectorz Data..."),
                i18n("Import data from Collectorz"),
                QIcon::fromTheme(QStringLiteral("collectorz"), QIcon(QLatin1String(":/icons/collectorz"))));

  IMPORT_ACTION(Import::DataCrow, "file_import_datacrow", i18n("Import Data Crow Data..."),
                i18n("Import data from Data Crow"),
                QIcon::fromTheme(QStringLiteral("datacrow"), QIcon(QLatin1String(":/icons/datacrow"))));

  IMPORT_ACTION(Import::Referencer, "file_import_referencer", i18n("Import Referencer Data..."),
                i18n("Import data from Referencer"),
                QIcon::fromTheme(QStringLiteral("referencer"), QIcon(QLatin1String(":/icons/referencer"))));

  IMPORT_ACTION(Import::Bibtex, "file_import_bibtex", i18n("Import Bibtex Data..."),
                i18n("Import a bibtex bibliography file"), mimeIcon("text/x-bibtex"));
#ifndef ENABLE_BTPARSE
  action->setEnabled(false);
#endif

  IMPORT_ACTION(Import::Bibtexml, "file_import_bibtexml", i18n("Import Bibtexml Data..."),
                i18n("Import a Bibtexml bibliography file"), mimeIcon("text/xml"));

  IMPORT_ACTION(Import::RIS, "file_import_ris", i18n("Import RIS Data..."),
                i18n("Import an RIS reference file"), QIcon::fromTheme(QStringLiteral(":/icons/cite")));

  IMPORT_ACTION(Import::MARC, "file_import_marc", i18n("Import MARC Data..."),
                i18n("Import MARC data"), QIcon::fromTheme(QStringLiteral(":/icons/cite")));
  // disable this import action if the necessary executable is not available
  QTimer::singleShot(1000, this, [action]() {
    const QString ymd = QStandardPaths::findExecutable(QStringLiteral("yaz-marcdump"));
    action->setEnabled(!ymd.isEmpty());
  });

  IMPORT_ACTION(Import::Goodreads, "file_import_goodreads", i18n("Import Goodreads Collection..."),
                i18n("Import a collection from Goodreads.com"), QIcon::fromTheme(QStringLiteral(":/icons/goodreads")));

  IMPORT_ACTION(Import::LibraryThing, "file_import_librarything", i18n("Import LibraryThing Collection..."),
                i18n("Import a collection from LibraryThing.com"), QIcon::fromTheme(QStringLiteral(":/icons/librarything")));

  IMPORT_ACTION(Import::PDF, "file_import_pdf", i18n("Import PDF File..."),
                i18n("Import a PDF file"), mimeIcon("application/pdf"));

  IMPORT_ACTION(Import::AudioFile, "file_import_audiofile", i18n("Import Audio File Metadata..."),
                i18n("Import meta-data from audio files"), mimeIcon("audio/mp3", "audio/x-mp3"));
#ifndef HAVE_TAGLIB
  action->setEnabled(false);
#endif

  IMPORT_ACTION(Import::FreeDB, "file_import_freedb", i18n("Import Audio CD Data..."),
                i18n("Import audio CD information"), mimeIcon("media/audiocd", "application/x-cda"));
#if !defined (HAVE_OLD_KCDDB) && !defined (HAVE_KCDDB)
  action->setEnabled(false);
#endif

  IMPORT_ACTION(Import::Discogs, "file_import_discogs", i18n("Import Discogs Collection..."),
                i18n("Import a collection from Discogs.com"), QIcon::fromTheme(QStringLiteral(":/icons/discogs")));

  IMPORT_ACTION(Import::GCstar, "file_import_gcstar", i18n("Import GCstar Data..."),
                i18n("Import a GCstar data file"),
                QIcon::fromTheme(QStringLiteral("gcstar"), QIcon(QLatin1String(":/icons/gcstar"))));

  IMPORT_ACTION(Import::Griffith, "file_import_griffith", i18n("Import Griffith Data..."),
                i18n("Import a Griffith database"),
                QIcon::fromTheme(QStringLiteral("griffith"), QIcon(QLatin1String(":/icons/griffith"))));

  IMPORT_ACTION(Import::AMC, "file_import_amc", i18n("Import Ant Movie Catalog Data..."),
                i18n("Import an Ant Movie Catalog data file"),
                QIcon::fromTheme(QStringLiteral("amc"), QIcon(QLatin1String(":/icons/amc"))));

  IMPORT_ACTION(Import::BoardGameGeek, "file_import_boardgamegeek", i18n("Import BoardGameGeek Collection..."),
                i18n("Import a collection from BoardGameGeek.com"), QIcon(QLatin1String(":/icons/boardgamegeek")));

  IMPORT_ACTION(Import::VinoXML, "file_import_vinoxml", i18n("Import VinoXML..."),
                i18n("Import VinoXML data"), QIcon(QLatin1String(":/icons/vinoxml")));

  IMPORT_ACTION(Import::FileListing, "file_import_filelisting", i18n("Import File Listing..."),
                i18n("Import information about files in a folder"), mimeIcon("inode/directory"));

  IMPORT_ACTION(Import::XSLT, "file_import_xslt", i18n("Import XSL Transform..."),
                i18n("Import using an XSL Transform"), mimeIcon("application/xslt+xml", "text/x-xslt"));

#undef IMPORT_ACTION

/**************** Export Menu ***************************/

  QSignalMapper* exportMapper = new QSignalMapper(this);
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
  connect(exportMapper, mappedInt, this, &MainWindow::slotFileExport);
#else
  connect(exportMapper, &QSignalMapper::mappedInt, this, &MainWindow::slotFileExport);
#endif

  KActionMenu* exportMenu = new KActionMenu(i18n("&Export"), this);
  exportMenu->setIcon(QIcon::fromTheme(QStringLiteral("document-export")));
  exportMenu->setToolTip(i18n("Export the collection data to other formats"));
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5,77,0)
  exportMenu->setPopupMode(QToolButton::InstantPopup);
#else
  exportMenu->setDelayed(false);
#endif
  actionCollection()->addAction(QStringLiteral("file_export"), exportMenu);

#define EXPORT_ACTION(TYPE, NAME, TEXT, TIP, ICON) \
  action = actionCollection()->addAction(QStringLiteral(NAME), exportMapper, mapVoid); \
  action->setText(TEXT); \
  action->setToolTip(TIP); \
  action->setIcon(ICON); \
  exportMenu->addAction(action); \
  exportMapper->setMapping(action, TYPE);

  EXPORT_ACTION(Export::TellicoXML, "file_export_xml", i18n("Export to XML..."),
                i18n("Export to a Tellico XML file"),
                QIcon::fromTheme(QStringLiteral("tellico"), QIcon(QStringLiteral(":/icons/tellico"))));

  EXPORT_ACTION(Export::TellicoZip, "file_export_zip", i18n("Export to Zip..."),
                i18n("Export to a Tellico Zip file"),
                QIcon::fromTheme(QStringLiteral("tellico"), QIcon(QStringLiteral(":/icons/tellico"))));

  EXPORT_ACTION(Export::HTML, "file_export_html", i18n("Export to HTML..."),
                i18n("Export to an HTML file"), mimeIcon("text/html"));

  EXPORT_ACTION(Export::CSV, "file_export_csv", i18n("Export to CSV..."),
                i18n("Export to a comma-separated values file"), mimeIcon("text/csv", "text/x-csv"));

  EXPORT_ACTION(Export::Alexandria, "file_export_alexandria", i18n("Export to Alexandria..."),
                i18n("Export to an Alexandria library"),
                QIcon::fromTheme(QStringLiteral("alexandria"), QIcon(QStringLiteral(":/icons/alexandria"))));

  EXPORT_ACTION(Export::Bibtex, "file_export_bibtex", i18n("Export to Bibtex..."),
                i18n("Export to a bibtex file"), mimeIcon("text/x-bibtex"));

  EXPORT_ACTION(Export::Bibtexml, "file_export_bibtexml", i18n("Export to Bibtexml..."),
                i18n("Export to a Bibtexml file"), mimeIcon("text/xml"));

  EXPORT_ACTION(Export::ONIX, "file_export_onix", i18n("Export to ONIX..."),
                i18n("Export to an ONIX file"), mimeIcon("text/xml"));

  EXPORT_ACTION(Export::GCstar, "file_export_gcstar", i18n("Export to GCstar..."),
                i18n("Export to a GCstar data file"),
                QIcon::fromTheme(QStringLiteral("gcstar"), QIcon(QStringLiteral(":/icons/gcstar"))));

  EXPORT_ACTION(Export::XSLT, "file_export_xslt", i18n("Export XSL Transform..."),
                i18n("Export using an XSL Transform"), mimeIcon("application/xslt+xml", "text/x-xslt"));

#undef EXPORT_ACTION

  /*************************************************
   * Edit menu
   *************************************************/
  KStandardAction::undo(Kernel::self()->commandHistory(), SLOT(undo()), actionCollection());
  KStandardAction::redo(Kernel::self()->commandHistory(), SLOT(undo()), actionCollection());

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

  action = actionCollection()->addAction(QStringLiteral("filter_dialog"), this, SLOT(slotShowFilterDialog()));
  action->setText(i18n("Advanced &Filter..."));
  action->setIconText(i18n("Filter"));
  action->setIcon(QIcon::fromTheme(QStringLiteral("view-filter")));
  actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::Key_J));
  action->setToolTip(i18n("Filter the collection"));

  /*************************************************
   * Collection menu
   *************************************************/
  m_newEntry = actionCollection()->addAction(QStringLiteral("coll_new_entry"),
                                             this, SLOT(slotNewEntry()));
  m_newEntry->setText(i18n("&New Entry..."));
  m_newEntry->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
  m_newEntry->setIconText(i18n("New Entry"));
  actionCollection()->setDefaultShortcut(m_newEntry, QKeySequence(Qt::CTRL | Qt::Key_N));
  m_newEntry->setToolTip(i18n("Create a new entry"));

  KActionMenu* addEntryMenu = new KActionMenu(i18n("Add Entry"), this);
  addEntryMenu->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5,77,0)
  addEntryMenu->setPopupMode(QToolButton::InstantPopup);
#else
  addEntryMenu->setDelayed(false);
#endif
  actionCollection()->addAction(QStringLiteral("coll_add_entry"), addEntryMenu);

  action = actionCollection()->addAction(QStringLiteral("edit_search_internet"), this, SLOT(slotShowFetchDialog()));
  action->setText(i18n("Internet Search..."));
  action->setIconText(i18n("Internet Search"));
  action->setIcon(QIcon::fromTheme(QStringLiteral("tools-wizard")));
  actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::Key_I));
  action->setToolTip(i18n("Search the internet..."));

  addEntryMenu->addAction(m_newEntry);
  addEntryMenu->addAction(actionCollection()->action(QStringLiteral("edit_search_internet")));

  m_editEntry = actionCollection()->addAction(QStringLiteral("coll_edit_entry"),
                                              this, SLOT(slotShowEntryEditor()));
  m_editEntry->setText(i18n("&Edit Entry..."));
  m_editEntry->setIcon(QIcon::fromTheme(QStringLiteral("document-properties")));
  actionCollection()->setDefaultShortcut(m_editEntry, QKeySequence(Qt::CTRL | Qt::Key_E));
  m_editEntry->setToolTip(i18n("Edit the selected entries"));

  m_copyEntry = actionCollection()->addAction(QStringLiteral("coll_copy_entry"),
                                              Controller::self(), SLOT(slotCopySelectedEntries()));
  m_copyEntry->setText(i18n("D&uplicate Entry"));
  m_copyEntry->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
  actionCollection()->setDefaultShortcut(m_copyEntry, QKeySequence(Qt::CTRL | Qt::Key_Y));
  m_copyEntry->setToolTip(i18n("Copy the selected entries"));

  m_deleteEntry = actionCollection()->addAction(QStringLiteral("coll_delete_entry"),
                                                Controller::self(), SLOT(slotDeleteSelectedEntries()));
  m_deleteEntry->setText(i18n("&Delete Entry"));
  m_deleteEntry->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
  actionCollection()->setDefaultShortcut(m_deleteEntry, QKeySequence(Qt::CTRL | Qt::Key_D));
  m_deleteEntry->setToolTip(i18n("Delete the selected entries"));

  m_mergeEntry = actionCollection()->addAction(QStringLiteral("coll_merge_entry"),
                                               Controller::self(), SLOT(slotMergeSelectedEntries()));
  m_mergeEntry->setText(i18n("&Merge Entries"));
  m_mergeEntry->setIcon(QIcon::fromTheme(QStringLiteral("document-import")));
//  CTRL+G is ambiguous, pick another
//  actionCollection()->setDefaultShortcut(m_mergeEntry, QKeySequence(Qt::CTRL | Qt::Key_G));
  m_mergeEntry->setToolTip(i18n("Merge the selected entries"));
  m_mergeEntry->setEnabled(false); // gets enabled when more than 1 entry is selected

  m_checkOutEntry = actionCollection()->addAction(QStringLiteral("coll_checkout"), Controller::self(), SLOT(slotCheckOut()));
  m_checkOutEntry->setText(i18n("Check-&out..."));
  m_checkOutEntry->setIcon(QIcon::fromTheme(QStringLiteral("arrow-up-double")));
  m_checkOutEntry->setToolTip(i18n("Check-out the selected items"));

  m_checkInEntry = actionCollection()->addAction(QStringLiteral("coll_checkin"), Controller::self(), SLOT(slotCheckIn()));
  m_checkInEntry->setText(i18n("Check-&in"));
  m_checkInEntry->setIcon(QIcon::fromTheme(QStringLiteral("arrow-down-double")));
  m_checkInEntry->setToolTip(i18n("Check-in the selected items"));

  action = actionCollection()->addAction(QStringLiteral("coll_rename_collection"), this, SLOT(slotRenameCollection()));
  action->setText(i18n("&Rename Collection..."));
  action->setIcon(QIcon::fromTheme(QStringLiteral("edit-rename")));
  actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::Key_R));
  action->setToolTip(i18n("Rename the collection"));

  action = actionCollection()->addAction(QStringLiteral("coll_fields"), this, SLOT(slotShowCollectionFieldsDialog()));
  action->setText(i18n("Collection &Fields..."));
  action->setIconText(i18n("Fields"));
  action->setIcon(QIcon::fromTheme(QStringLiteral("preferences-other")));
  actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::Key_U));
  action->setToolTip(i18n("Modify the collection fields"));

  action = actionCollection()->addAction(QStringLiteral("coll_reports"), this, SLOT(slotShowReportDialog()));
  action->setText(i18n("&Generate Reports..."));
  action->setIconText(i18n("Reports"));
  action->setIcon(QIcon::fromTheme(QStringLiteral("text-rdf")));
  action->setToolTip(i18n("Generate collection reports"));

  action = actionCollection()->addAction(QStringLiteral("coll_convert_bibliography"), this, SLOT(slotConvertToBibliography()));
  action->setText(i18n("Convert to &Bibliography"));
  action->setIcon(QIcon(QLatin1String(":/icons/bibtex")));
  action->setToolTip(i18n("Convert a book collection to a bibliography"));

  action = actionCollection()->addAction(QStringLiteral("coll_string_macros"), this, SLOT(slotShowStringMacroDialog()));
  action->setText(i18n("String &Macros..."));
  action->setIcon(QIcon::fromTheme(QStringLiteral("view-list-text")));
  action->setToolTip(i18n("Edit the bibtex string macros"));

  action = actionCollection()->addAction(QStringLiteral("coll_key_manager"), this, SLOT(slotShowBibtexKeyDialog()));
  action->setText(i18n("Check for Duplicate Keys..."));
  action->setIcon(mimeIcon("text/x-bibtex"));
  action->setToolTip(i18n("Check for duplicate citation keys"));

  QSignalMapper* citeMapper = new QSignalMapper(this);
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
  connect(citeMapper, mappedInt, this, &MainWindow::slotCiteEntry);
#else
  connect(citeMapper, &QSignalMapper::mappedInt, this, &MainWindow::slotCiteEntry);
#endif

  action = actionCollection()->addAction(QStringLiteral("cite_clipboard"), citeMapper, mapVoid);
  action->setText(i18n("Copy Bibtex to Cli&pboard"));
  action->setToolTip(i18n("Copy bibtex citations to the clipboard"));
  action->setIcon(QIcon::fromTheme(QStringLiteral("edit-paste")));
  citeMapper->setMapping(action, Cite::CiteClipboard);

  action = actionCollection()->addAction(QStringLiteral("cite_lyxpipe"), citeMapper, mapVoid);
  action->setText(i18n("Cite Entry in &LyX"));
  action->setToolTip(i18n("Cite the selected entries in LyX"));
  action->setIcon(QIcon::fromTheme(QStringLiteral("lyx"), QIcon(QLatin1String(":/icons/lyx"))));
  citeMapper->setMapping(action, Cite::CiteLyxpipe);

  m_updateMapper = new QSignalMapper(this);
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
  void (QSignalMapper::* mappedString)(const QString&) = &QSignalMapper::mapped;
  connect(m_updateMapper, mappedString,
          Controller::self(), &Controller::slotUpdateSelectedEntries);
#else
  connect(m_updateMapper, &QSignalMapper::mappedString,
          Controller::self(), &Controller::slotUpdateSelectedEntries);
#endif

  m_updateEntryMenu = new KActionMenu(i18n("&Update Entry"), this);
  m_updateEntryMenu->setIcon(QIcon::fromTheme(QStringLiteral("document-export")));
  m_updateEntryMenu->setIconText(i18nc("Update Entry", "Update"));
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5,77,0)
  m_updateEntryMenu->setPopupMode(QToolButton::InstantPopup);
#else
  m_updateEntryMenu->setDelayed(false);
#endif
  actionCollection()->addAction(QStringLiteral("coll_update_entry"), m_updateEntryMenu);

  m_updateAll = actionCollection()->addAction(QStringLiteral("update_entry_all"), m_updateMapper, mapVoid);
  m_updateAll->setText(i18n("All Sources"));
  m_updateAll->setToolTip(i18n("Update entry data from all available sources"));
  m_updateMapper->setMapping(m_updateAll, QStringLiteral("_all"));

  /*************************************************
   * Settings menu
   *************************************************/
  setStandardToolBarMenuEnabled(true);
  createStandardStatusBarAction();
  // style config
#if HAVE_STYLE_MANAGER
  actionCollection()->addAction(QStringLiteral("settings_style"), KStyleManager::createConfigureAction(this));
#endif

  m_lockLayout = new KDualAction(this);
  connect(m_lockLayout, &KDualAction::activeChanged, this, &MainWindow::slotToggleLayoutLock);
  m_lockLayout->setActiveText(i18n("Unlock Layout"));
  m_lockLayout->setActiveToolTip(i18n("Unlock the window's layout"));
  m_lockLayout->setActiveIcon(QIcon::fromTheme(QStringLiteral("object-unlocked")));
  m_lockLayout->setInactiveText(i18n("Lock Layout"));
  m_lockLayout->setInactiveToolTip(i18n("Lock the window's layout"));
  m_lockLayout->setInactiveIcon(QIcon::fromTheme(QStringLiteral("object-locked")));
  actionCollection()->addAction(QStringLiteral("lock_layout"), m_lockLayout);

  action = actionCollection()->addAction(QStringLiteral("reset_layout"), this, SLOT(slotResetLayout()));
  action->setText(i18n("Reset Layout"));
  action->setToolTip(i18n("Reset the window's layout"));
  action->setIcon(QIcon::fromTheme(QStringLiteral("resetview")));

  m_toggleEntryEditor = new KToggleAction(i18n("Entry &Editor"), this);
  connect(m_toggleEntryEditor, &QAction::triggered, this, &MainWindow::slotToggleEntryEditor);
  m_toggleEntryEditor->setToolTip(i18n("Enable/disable the editor"));
  actionCollection()->addAction(QStringLiteral("toggle_edit_widget"), m_toggleEntryEditor);

  KStandardAction::preferences(this, SLOT(slotShowConfigDialog()), actionCollection());

  /*************************************************
   * Help menu
   *************************************************/
  action = actionCollection()->addAction(QStringLiteral("show_log"), this, SLOT(showLog()));
  action->setText(i18n("Show Log"));
  action->setIcon(QIcon::fromTheme(QStringLiteral("view-history")));
  if(Logger::self()->logFile().isEmpty()) {
    action->setEnabled(false);
  }

  /*************************************************
   * Short cuts
   *************************************************/
  KStandardAction::fullScreen(this, SLOT(slotToggleFullScreen()), this, actionCollection());
  KStandardAction::showMenubar(this, SLOT(slotToggleMenuBarVisibility()), actionCollection());

  /*************************************************
   * Collection Toolbar
   *************************************************/
  action = actionCollection()->addAction(QStringLiteral("change_entry_grouping_accel"), this, SLOT(slotGroupLabelActivated()));
  action->setText(i18n("Change Grouping"));
  actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::Key_G));

  m_entryGrouping = new KSelectAction(i18n("&Group Selection"), this);
  m_entryGrouping->setToolTip(i18n("Change the grouping of the collection"));
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5,78,0)
  void (KSelectAction::* triggeredInt)(int) = &KSelectAction::indexTriggered;
#else
  void (KSelectAction::* triggeredInt)(int) = &KSelectAction::triggered;
#endif
  connect(m_entryGrouping, triggeredInt, this, &MainWindow::slotChangeGrouping);
  actionCollection()->addAction(QStringLiteral("change_entry_grouping"), m_entryGrouping);

  action = actionCollection()->addAction(QStringLiteral("quick_filter_accel"), this, SLOT(slotFilterLabelActivated()));
  action->setText(i18n("Filter"));
  actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::Key_F));

  m_quickFilter = new GUI::LineEdit(this);
  m_quickFilter->setPlaceholderText(i18n("Filter here...")); // same text as kdepim and amarok
  m_quickFilter->setClearButtonEnabled(true);
  // same as Dolphin text edit
  m_quickFilter->setMinimumWidth(150);
  m_quickFilter->setMaximumWidth(300);
  // want to update every time the filter text changes
  connect(m_quickFilter, &QLineEdit::textChanged,
          this, &MainWindow::slotQueueFilter);
  connect(m_quickFilter, &KLineEdit::clearButtonClicked,
          this, &MainWindow::slotClearFilter);
  m_quickFilter->installEventFilter(this); // intercept keyEvents

  QWidgetAction* widgetAction = new QWidgetAction(this);
  widgetAction->setDefaultWidget(m_quickFilter);
  widgetAction->setText(i18n("Filter"));
  widgetAction->setToolTip(i18n("Filter the collection"));
  widgetAction->setProperty("isShortcutConfigurable", false);
  actionCollection()->addAction(QStringLiteral("quick_filter"), widgetAction);

  // final GUI setup is in initView()
}

#undef mimeIcon

void MainWindow::initDocument() {
  MARK;
  Data::Document* doc = Data::Document::self();
  Kernel::self()->resetHistory();

  KConfigGroup config(KSharedConfig::openConfig(), QLatin1String("General Options"));
  doc->setLoadAllImages(config.readEntry("Load All Images", false));

  // allow status messages from the document
  connect(doc, &Data::Document::signalStatusMsg,
          this, &MainWindow::slotStatusMsg);

  // do stuff that changes when the doc is modified
  connect(doc, &Data::Document::signalModified,
          this, &MainWindow::slotEnableModifiedActions);

  connect(doc, &Data::Document::signalCollectionAdded,
          Controller::self(), &Controller::slotCollectionAdded);
  connect(doc, &Data::Document::signalCollectionDeleted,
          Controller::self(), &Controller::slotCollectionDeleted);
  connect(doc, &Data::Document::signalCollectionModified,
          Controller::self(), &Controller::slotCollectionModified);

  connect(Kernel::self()->commandHistory(), &QUndoStack::cleanChanged,
          doc, &Data::Document::slotSetClean);
}

void MainWindow::initView() {
  MARK;
  // initialize the image factory before the entry models are created
  ImageFactory::init();

  m_entryView = new EntryView(this);
  connect(m_entryView, &EntryView::signalTellicoAction,
          this, &MainWindow::slotURLAction);

  // trick to make sure the group views always extend along the entire left or right side
  // using QMainWindow::setCorner does not seem to work
  // https://wiki.qt.io/Technical_FAQ#Is_it_possible_for_either_the_left_or_right_dock_areas_to_have_full_height_of_their_side_rather_than_having_the_bottom_take_the_full_width.3F
  m_dummyWindow = new QMainWindow(this);
#ifdef USE_KHTML
  m_entryView->view()->setWhatsThis(i18n("<qt>The <i>Entry View</i> shows a formatted view of the entry's contents.</qt>"));
  m_dummyWindow->setCentralWidget(m_entryView->view());
#else
  m_entryView->setWhatsThis(i18n("<qt>The <i>Entry View</i> shows a formatted view of the entry's contents.</qt>"));
  m_dummyWindow->setCentralWidget(m_entryView);
#endif
  m_dummyWindow->setWindowFlags(Qt::Widget);
  setCentralWidget(m_dummyWindow);

  m_collectionViewDock = new GUI::DockWidget(i18n("Collection View"), m_dummyWindow);
  m_collectionViewDock->setObjectName(QStringLiteral("collection_dock"));

  m_viewStack = new ViewStack(this);

  m_detailedView = m_viewStack->listView();
  Controller::self()->addObserver(m_detailedView);
  m_detailedView->setWhatsThis(i18n("<qt>The <i>Column View</i> shows the value of multiple fields "
                                    "for each entry.</qt>"));
  connect(Data::Document::self(), &Data::Document::signalCollectionImagesLoaded,
          m_detailedView, &DetailedListView::slotRefreshImages);

  m_iconView = m_viewStack->iconView();
  EntryIconModel* iconModel = new EntryIconModel(m_iconView);
  iconModel->setSourceModel(m_detailedView->model());
  m_iconView->setModel(iconModel);
  Controller::self()->addObserver(m_iconView);
  m_iconView->setWhatsThis(i18n("<qt>The <i>Icon View</i> shows each entry in the collection or group using "
                                "an icon, which may be an image in the entry.</qt>"));

  m_collectionViewDock->setWidget(m_viewStack);
  m_dummyWindow->addDockWidget(Qt::TopDockWidgetArea, m_collectionViewDock);
  actionCollection()->addAction(QStringLiteral("toggle_column_widget"), m_collectionViewDock->toggleViewAction());

  m_groupViewDock = new GUI::DockWidget(i18n("Group View"), this);
  m_groupViewDock->setObjectName(QStringLiteral("group_dock"));
  m_groupViewDock->setAllowedAreas(Qt::DockWidgetAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea));

  m_viewTabs = new GUI::TabWidget(this);
  m_viewTabs->setTabBarHidden(true);
  m_viewTabs->setDocumentMode(true);
  m_groupView = new GroupView(m_viewTabs);
  Controller::self()->addObserver(m_groupView);
  m_viewTabs->addTab(m_groupView, QIcon::fromTheme(QStringLiteral("folder")), i18n("Groups"));
  m_groupView->setWhatsThis(i18n("<qt>The <i>Group View</i> sorts the entries into groupings "
                                 "based on a selected field.</qt>"));
  m_groupViewDock->setWidget(m_viewTabs);
  addDockWidget(Qt::LeftDockWidgetArea, m_groupViewDock);
  actionCollection()->addAction(QStringLiteral("toggle_group_widget"), m_groupViewDock->toggleViewAction());

  EntrySelectionModel* proxySelect = new EntrySelectionModel(m_iconView->model(),
                                                             m_detailedView->selectionModel(),
                                                             this);
  m_iconView->setSelectionModel(proxySelect);

  // Do custom themes override widget palettes? Ensure the EntryView remains consistent with the others
  m_entryView->setPalette(m_iconView->palette());

  // setting up GUI now rather than in initActions
  // initial parameter is default window size
  setupGUI(QSize(1280,800), Keys | ToolBar);
  createGUI();
}

void MainWindow::initConnections() {
  // have to toggle the menu item if the dialog gets closed
  connect(m_editDialog, &QDialog::finished,
          this, &MainWindow::slotEditDialogFinished);

  EntrySelectionModel* proxySelect = static_cast<EntrySelectionModel*>(m_iconView->selectionModel());
  connect(proxySelect, &EntrySelectionModel::entriesSelected,
          Controller::self(), &Controller::slotUpdateSelection);
  connect(proxySelect, &EntrySelectionModel::entriesSelected,
          m_editDialog, &EntryEditDialog::setContents);
  connect(proxySelect, &EntrySelectionModel::entriesSelected,
          m_entryView, &EntryView::showEntries);

  // let the group view call filters, too
  connect(m_groupView, &GroupView::signalUpdateFilter,
          this, &MainWindow::slotUpdateFilter);
  // use the EntrySelectionModel as a proxy so when entries get selected in the group view
  // the edit dialog and entry view are updated
  proxySelect->addSelectionProxy(m_groupView->selectionModel());
}

void MainWindow::initFileOpen(bool nofile_) {
  MARK;
  slotInit();
  // check to see if most recent file should be opened
  bool happyStart = false;
  if(!nofile_ && Config::reopenLastFile()) {
    // Config::lastOpenFile() is the full URL, protocol included
    QUrl lastFile(Config::lastOpenFile()); // empty string is actually ok, it gets handled
    if(!lastFile.isEmpty() && lastFile.isValid()) {
      myLog() << "Opening previous file:" << lastFile.toDisplayString(QUrl::PreferLocalFile);
      slotFileOpen(lastFile);
      happyStart = true;
    }
  }
  if(!happyStart) {
    myLog() << "Creating default book collection";
    // the document is created with an initial book collection, continue with that
    Controller::self()->slotCollectionAdded(Data::Document::self()->collection());

    m_fileSave->setEnabled(false);
    slotEnableOpenedActions();
    slotEnableModifiedActions(false);

    slotEntryCount();
    // tell the entry views and models that there are no images to load
    m_detailedView->slotRefreshImages();
  }

  if(Config::showWelcome()) {
    // show welcome text, even when opening an existing collection
    QString welcomeFile = DataFileRegistry::self()->locate(QStringLiteral("welcome.html"));
    QString text = FileHandler::readTextFile(QUrl::fromLocalFile(welcomeFile));
    const int type = Kernel::self()->collectionType();
    text.replace(QLatin1String("$FGCOLOR$"), Config::templateTextColor(type).name());
    text.replace(QLatin1String("$BGCOLOR$"), Config::templateBaseColor(type).name());
    text.replace(QLatin1String("$COLOR1$"),  Config::templateHighlightedTextColor(type).name());
    text.replace(QLatin1String("$COLOR2$"),  Config::templateHighlightedBaseColor(type).name());
    text.replace(QLatin1String("$LINKCOLOR$"), Config::templateLinkColor(type).name());
    text.replace(QLatin1String("$IMGDIR$"),  ImageFactory::imageDir().url());
    text.replace(QLatin1String("$SUBTITLE$"),  i18n("Collection management software, free and simple"));
    text.replace(QLatin1String("$BANNER$"),
                 i18n("Welcome to the Tellico Collection Manager"));
    text.replace(QLatin1String("$WELCOMETEXT$"),
                 i18n("<h3>Tellico is a tool for managing collections of books, "
                      "videos, music, and whatever else you want to catalog.</h3>"
                      "<h3>New entries can be added to your collection by "
                      "<a href=\"tc:///coll_new_entry\">entering data manually</a> or by "
                      "<a href=\"tc:///edit_search_internet\">downloading data</a> from "
                      "various Internet sources.</h3>")
                 .replace(QLatin1String("<h3>"),  QLatin1String("<p>"))
                 .replace(QLatin1String("</h3>"), QLatin1String("</p>")));
    text.replace(QLatin1String("$FOOTER$"),
                 i18n("More information can be found in the <a href=\"help:/tellico\">documentation</a>. "
                      "You may also <a href=\"tc:///disable_welcome\">disable this welcome screen</a>."));
    QString iconPath = KIconLoader::global()->iconPath(QLatin1String("tellico"), -KIconLoader::SizeEnormous);
    if(iconPath.startsWith(QLatin1String(":/"))) {
      iconPath = QStringLiteral("qrc") + iconPath;
    } else {
      iconPath = QStringLiteral("file://") + iconPath;
    }

    text.replace(QLatin1String("$ICON$"),
                 QStringLiteral("<img src=\"%1\" align=\"top\" height=\"%2\" width=\"%2\" title=\"tellico\" />")
                 .arg(iconPath)
                 .arg(KIconLoader::SizeEnormous));
    m_entryView->showText(text);
  }

  m_initialized = true;
}

// These are general options.
// The options that can be changed in the "Configuration..." dialog
// are taken care of by the ConfigDialog object.
void MainWindow::saveOptions() {
  KConfigGroup config(KSharedConfig::openConfig(), QLatin1String("Main Window Options"));
  saveMainWindowSettings(config);
  config.writeEntry(QStringLiteral("Central Dock State"), m_dummyWindow->saveState());

  Config::setShowEditWidget(m_toggleEntryEditor->isChecked());
  // check any single dock widget, they all get locked together
  Config::setLockLayout(m_groupViewDock->isLocked());

  KConfigGroup filesConfig(KSharedConfig::openConfig(), QLatin1String("Recent Files"));
  m_fileOpenRecent->saveEntries(filesConfig);
  if(!isNewDocument()) {
    Config::setLastOpenFile(Data::Document::self()->URL().url());
  }

  Config::setViewWidget(m_viewStack->currentWidget());

  // historical reasons
  // sorting by count was faked by sorting by phantom second column
  const int sortColumn = m_groupView->sortRole() == RowCountRole ? 1 : 0;
  Config::setGroupViewSortColumn(sortColumn); // ok to use SortColumn key, save semantics
  Config::setGroupViewSortAscending(m_groupView->sortOrder() == Qt::AscendingOrder);

  if(m_loanView) {
    const int sortColumn = m_loanView->sortRole() == RowCountRole ? 1 : 0;
    Config::setLoanViewSortAscending(sortColumn); // ok to use SortColumn key, save semantics
    Config::setLoanViewSortAscending(m_loanView->sortOrder() == Qt::AscendingOrder);
  }

  if(m_filterView) {
    const int sortColumn = m_filterView->sortRole() == RowCountRole ? 1 : 0;
    Config::setFilterViewSortAscending(sortColumn); // ok to use SortColumn key, save semantics
    Config::setFilterViewSortAscending(m_filterView->sortOrder() == Qt::AscendingOrder);
  }

  // this is used in the EntryEditDialog constructor, too
  KConfigGroup editDialogConfig(KSharedConfig::openConfig(), QLatin1String("Edit Dialog Options"));
  KWindowConfig::saveWindowSize(m_editDialog->windowHandle(), editDialogConfig);

  saveCollectionOptions(Data::Document::self()->collection());
  Config::self()->save();
}

void MainWindow::readCollectionOptions(Tellico::Data::CollPtr coll_) {
  if(!coll_) {
    myDebug() << "Bad, no collection in MainWindow::readCollectionOptions()";
    return;
  }
  const QString configGroup = QStringLiteral("Options - %1").arg(CollectionFactory::typeName(coll_));
  KConfigGroup group(KSharedConfig::openConfig(), configGroup);

  QString defaultGroup = coll_->defaultGroupField();
  QString entryGroup, groupSortField;
  if(coll_->type() != Data::Collection::Base) {
    entryGroup = group.readEntry("Group By", defaultGroup);
    groupSortField = group.readEntry("GroupEntrySortField", QString());
  } else {
    QUrl url = Kernel::self()->URL();
    for(int i = 0; i < Config::maxCustomURLSettings(); ++i) {
      QUrl u(group.readEntry(QStringLiteral("URL_%1").arg(i)));
      if(url == u) {
        entryGroup = group.readEntry(QStringLiteral("Group By_%1").arg(i), defaultGroup);
        groupSortField = group.readEntry(QStringLiteral("GroupEntrySortField_%1").arg(i), QString());
        break;
      }
    }
    // fall back to old setting
    if(entryGroup.isEmpty()) {
      entryGroup = group.readEntry("Group By", defaultGroup);
    }
  }
  if(entryGroup.isEmpty() ||
     (!coll_->entryGroups().contains(entryGroup) && entryGroup != Data::Collection::s_peopleGroupName)) {
    entryGroup = defaultGroup;
  }
  m_groupView->setGroupField(entryGroup);

  if(!groupSortField.isEmpty()) {
    m_groupView->setEntrySortField(groupSortField);
  }

  QString entryXSLTFile;
  if(coll_->type() == Data::Collection::Base &&
     Data::Document::self()->URL().fileName() != TC_I18N1(Tellico::untitledFilename)) {
    // use a nested config group for template specific to custom collections
    // using the filename alone as a keyEvents
    KConfigGroup subGroup(&group, Data::Document::self()->URL().fileName());
    entryXSLTFile = subGroup.readEntry(QStringLiteral("Template Name"));
  }
  if(entryXSLTFile.isEmpty()) {
    // lookup by collection type
    entryXSLTFile = Config::templateName(coll_->type());
  }
  if(entryXSLTFile.isEmpty()) {
    entryXSLTFile = QStringLiteral("Fancy"); // should never happen, but just in case
  }
  m_entryView->setXSLTFile(entryXSLTFile + QLatin1String(".xsl"));

  // make sure the right combo element is selected
  slotUpdateCollectionToolBar(coll_);
}

void MainWindow::saveCollectionOptions(Tellico::Data::CollPtr coll_) {
  // don't save initial collection options, or empty collections
  if(!coll_ || coll_->entryCount() == 0 || isNewDocument()) {
    return;
  }

  int configIndex = -1;
  QString configGroup = QStringLiteral("Options - %1").arg(CollectionFactory::typeName(coll_));
  KConfigGroup config(KSharedConfig::openConfig(), configGroup);
  QString groupName;
  const QString groupEntrySort = m_groupView->entrySortField();
  if(m_entryGrouping->currentItem() > -1 &&
     static_cast<int>(coll_->entryGroups().count()) > m_entryGrouping->currentItem()) {
    if(m_entryGrouping->currentText() == (QLatin1Char('<') + i18n("People") + QLatin1Char('>'))) {
      groupName = Data::Collection::s_peopleGroupName;
    } else {
      groupName = Data::Document::self()->collection()->fieldNameByTitle(m_entryGrouping->currentText());
    }
    if(coll_->type() != Data::Collection::Base) {
      config.writeEntry("Group By", groupName);
      if(!groupEntrySort.isEmpty()) {
        config.writeEntry("GroupEntrySortField", groupEntrySort);
      }
    }
  }

  if(coll_->type() == Data::Collection::Base) {
    // all of this is to have custom settings on a per file basis
    QUrl url = Kernel::self()->URL();
    QList<QUrl> urls = QList<QUrl>() << url;
    QStringList groupBys = QStringList() << groupName;
    QStringList groupSorts = QStringList() << groupEntrySort;
    for(int i = 0; i < Config::maxCustomURLSettings(); ++i) {
      QUrl u = config.readEntry(QStringLiteral("URL_%1").arg(i), QUrl());
      if(!u.isEmpty() && url != u) {
        urls.append(u);
        QString g = config.readEntry(QStringLiteral("Group By_%1").arg(i), QString());
        groupBys.append(g);
        QString gs = config.readEntry(QStringLiteral("GroupEntrySortField_%1").arg(i), QString());
        groupSorts.append(gs);
      } else if(!u.isEmpty()) {
        configIndex = i;
      }
    }
    int limit = qMin(urls.count(), Config::maxCustomURLSettings());
    for(int i = 0; i < limit; ++i) {
      config.writeEntry(QStringLiteral("URL_%1").arg(i), urls[i].url());
      config.writeEntry(QStringLiteral("Group By_%1").arg(i), groupBys[i]);
      config.writeEntry(QStringLiteral("GroupEntrySortField_%1").arg(i), groupSorts[i]);
    }
  }
  m_detailedView->saveConfig(coll_, configIndex);
}

void MainWindow::readOptions() {
  KConfigGroup mainWindowConfig(KSharedConfig::openConfig(), QLatin1String("Main Window Options"));
  applyMainWindowSettings(mainWindowConfig);
  m_dummyWindow->restoreState(mainWindowConfig.readEntry(QStringLiteral("Central Dock State"), QByteArray()));

  m_viewStack->setCurrentWidget(Config::viewWidget());
  m_iconView->setMaxAllowedIconWidth(Config::maxIconSize());

  connect(toolBar(QStringLiteral("collectionToolBar")), &QToolBar::iconSizeChanged, this, &MainWindow::slotUpdateToolbarIcons);

  // initialize the recent file list
  KConfigGroup filesConfig(KSharedConfig::openConfig(), QLatin1String("Recent Files"));
  m_fileOpenRecent->loadEntries(filesConfig);

  // sort by count if column = 1
  int sortRole = Config::groupViewSortColumn() == 0 ? static_cast<int>(Qt::DisplayRole) : static_cast<int>(RowCountRole);
  Qt::SortOrder sortOrder = Config::groupViewSortAscending() ? Qt::AscendingOrder : Qt::DescendingOrder;
  m_groupView->setSorting(sortOrder, sortRole);

  BibtexHandler::s_quoteStyle = Config::useBraces() ? BibtexHandler::BRACES : BibtexHandler::QUOTES;

  // Don't read any options for the edit dialog here, since it's not yet initialized.
  // Put them in init()
}

bool MainWindow::querySaveModified() {
  bool completed = true;

  if(Data::Document::self()->isModified()) {
    QString str = i18n("The current file has been modified.\n"
                       "Do you want to save it?");
#if KWIDGETSADDONS_VERSION < QT_VERSION_CHECK(5, 100, 0)
    auto want_save = KMessageBox::warningYesNoCancel(this, str, i18n("Unsaved Changes"),
                                                     KStandardGuiItem::save(), KStandardGuiItem::discard());
    switch(want_save) {
      case KMessageBox::Yes:
        completed = fileSave();
        break;

      case KMessageBox::No:
        Data::Document::self()->setModified(false);
        completed = true;
        break;

      case KMessageBox::Cancel:
      default:
        completed = false;
        break;
    }
#else
    auto want_save = KMessageBox::warningTwoActionsCancel(this, str, i18n("Unsaved Changes"),
                                                          KStandardGuiItem::save(), KStandardGuiItem::discard());
    switch(want_save) {
      case KMessageBox::ButtonCode::PrimaryAction:
        completed = fileSave();
        break;

      case KMessageBox::ButtonCode::SecondaryAction:
        Data::Document::self()->setModified(false);
        completed = true;
        break;

      case KMessageBox::ButtonCode::Cancel:
      default:
        completed = false;
        break;
    }
#endif
  }

  return completed;
}

bool MainWindow::queryClose() {
  // in case we're still loading the images, cancel that
  Data::Document::self()->cancelImageWriting();
  const bool willClose = m_editDialog->queryModified() && querySaveModified();
  if(willClose) {
    ImageFactory::clean(true);
    saveOptions();
  }
  return willClose;
}

void MainWindow::slotFileNew(int type_) {
  slotStatusMsg(i18n("Creating new collection..."));

  // close the fields dialog
  slotHideCollectionFieldsDialog();

  if(m_editDialog->queryModified() && querySaveModified()) {
    // remove filter and loan tabs, they'll get re-added if needed
    if(m_filterView) {
      m_viewTabs->removeTab(m_viewTabs->indexOf(m_filterView));
      Controller::self()->removeObserver(m_filterView);
      delete m_filterView;
      m_filterView = nullptr;
    }
    if(m_loanView) {
      m_viewTabs->removeTab(m_viewTabs->indexOf(m_loanView));
      Controller::self()->removeObserver(m_loanView);
      delete m_loanView;
      m_loanView = nullptr;
    }
    m_viewTabs->setTabBarHidden(true);
    Data::Document::self()->newDocument(type_);
    myLog() << "Creating new collection, type" << CollectionFactory::typeName(type_);
    Kernel::self()->resetHistory();
    m_fileOpenRecent->setCurrentItem(-1);
    slotEnableOpenedActions();
    slotEnableModifiedActions(false);
    m_newDocument = true;
    ImageFactory::clean(false);
  }

  StatusBar::self()->clearStatus();
}

void MainWindow::slotFileNewByTemplate(const QString& collectionTemplate_) {
  slotStatusMsg(i18n("Creating new collection..."));

  // close the fields dialog
  slotHideCollectionFieldsDialog();

  if(m_editDialog->queryModified() && querySaveModified()) {
    openURL(QUrl::fromLocalFile(collectionTemplate_));
    myLog() << "Creating new collection from template:" << collectionTemplate_;
    Data::Document::self()->setURL(QUrl::fromLocalFile(TC_I18N1(Tellico::untitledFilename)));
    Kernel::self()->resetHistory();
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

  if(m_editDialog->queryModified() && querySaveModified()) {
    QString filter = i18n("Tellico Files") + QLatin1String(" (*.tc *.bc)");
    filter += QLatin1String(";;");
    filter += i18n("XML Files") + QLatin1String(" (*.xml)");
    filter += QLatin1String(";;");
    filter += i18n("All Files") + QLatin1String(" (*)");
    // keyword 'open'
    QString fileClass;
    const QUrl startUrl = KFileWidget::getStartUrl(QUrl(QStringLiteral("kfiledialog:///open")), fileClass);
    QUrl url = QFileDialog::getOpenFileUrl(this, i18n("Open File"), startUrl, filter);
    if(!url.isEmpty() && url.isValid()) {
      slotFileOpen(url);
      if(url.isLocalFile()) {
        KRecentDirs::add(fileClass, url.adjusted(QUrl::RemoveFilename|QUrl::StripTrailingSlash).path());
      }
    }
  }
  StatusBar::self()->clearStatus();
}

void MainWindow::slotFileOpen(const QUrl& url_) {
  slotStatusMsg(i18n("Opening file..."));

  // close the fields dialog
  slotHideCollectionFieldsDialog();

  // there seems to be a race condition at start between slotInit() and initFileOpen()
  // which means the edit dialog might not have been created yet
  if((!m_editDialog || m_editDialog->queryModified()) && querySaveModified()) {
    if(openURL(url_)) {
      m_fileOpenRecent->addUrl(url_);
      m_fileOpenRecent->setCurrentItem(-1);
    }
  }

  StatusBar::self()->clearStatus();
}

void MainWindow::slotFileOpenRecent(const QUrl& url_) {
  slotStatusMsg(i18n("Opening file..."));

  // close the fields dialog
  slotHideCollectionFieldsDialog();

  if(m_editDialog->queryModified() && querySaveModified()) {
    if(!openURL(url_)) {
      m_fileOpenRecent->removeUrl(url_);
      m_fileOpenRecent->setCurrentItem(-1);
    }
  } else {
    // the QAction shouldn't be checked now
    m_fileOpenRecent->setCurrentItem(-1);
  }

  StatusBar::self()->clearStatus();
}

void MainWindow::openFile(const QString& file_) {
  QUrl url(file_);
  if(!url.isEmpty() && url.isValid()) {
    slotFileOpen(url);
  }
}

bool MainWindow::openURL(const QUrl& url_) {
  MARK;
  // try to open document
  GUI::CursorSaver cs(Qt::WaitCursor);

  myLog() << "Opening collection file:" << url_.toDisplayString(QUrl::PreferLocalFile);
  bool success = Data::Document::self()->openDocument(url_);

  if(success) {
    Kernel::self()->resetHistory();
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
    m_filterView = nullptr;
  }
  if(m_loanView && m_loanView->isEmpty()) {
    m_viewTabs->removeTab(m_viewTabs->indexOf(m_loanView));
    Controller::self()->removeObserver(m_loanView);
    delete m_loanView;
    m_loanView = nullptr;
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
    // is set, then warn user that performance may suffer, and write result
    if(Config::imageLocation() == Config::ImagesInFile &&
       Config::askWriteImagesInFile() &&
       Data::Document::self()->imageCount() > MAX_IMAGES_WARN_PERFORMANCE) {
      QString msg = i18n("<qt><p>You are saving a file with many images, which causes Tellico to "
                         "slow down significantly. Do you want to save the images separately in "
                         "Tellico's data directory to improve performance?</p><p>Your choice can "
                         "always be changed in the configuration dialog.</p></qt>");

      KGuiItem yes(i18n("Save Images Separately"));
      KGuiItem no(i18n("Save Images in File"));

#if KWIDGETSADDONS_VERSION < QT_VERSION_CHECK(5, 100, 0)
      auto res = KMessageBox::warningYesNo(this, msg, QString() /* caption */, yes, no);
      if(res == KMessageBox::No) {
#else
      auto res = KMessageBox::warningTwoActions(this, msg, QString() /* caption */, yes, no);
      if(res == KMessageBox::ButtonCode::SecondaryAction) {
#endif
        Config::setImageLocation(Config::ImagesInAppDir);
      }
      Config::setAskWriteImagesInFile(false);
    }

    GUI::CursorSaver cs(Qt::WaitCursor);
    if(Data::Document::self()->saveDocument(Data::Document::self()->URL())) {
      Kernel::self()->resetHistory();
      m_newDocument = false;
      updateCaption(false);
      m_fileSave->setEnabled(false);
      // TODO: call a method of the model instead of the view here
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

  QString filter = i18n("Tellico Files") + QLatin1String(" (*.tc *.bc)");
  filter += QLatin1String(";;");
  filter += i18n("All Files") + QLatin1String(" (*)");

  // keyword 'open'
  QString fileClass;
  const QUrl startUrl = KFileWidget::getStartUrl(QUrl(QStringLiteral("kfiledialog:///open")), fileClass);
  const QUrl url = QFileDialog::getSaveFileUrl(this, i18n("Save As"), startUrl, filter);

  if(url.isEmpty()) {
    StatusBar::self()->clearStatus();
    return false;
  }
  if(url.isLocalFile()) {
    KRecentDirs::add(fileClass, url.adjusted(QUrl::RemoveFilename|QUrl::StripTrailingSlash).path());
  }

  bool ret = true;
  if(url.isValid()) {
    GUI::CursorSaver cs(Qt::WaitCursor);
    m_savingImageLocationChange = true;
    // Overwriting an existing file was already confirmed in QFileDialog::getSaveFileUrl()
    if(Data::Document::self()->saveDocument(url, true /* force */)) {
      Kernel::self()->resetHistory();
      KRecentDocument::add(url);
      m_fileOpenRecent->addUrl(url);
      updateCaption(false);
      m_newDocument = false;
      m_fileSave->setEnabled(false);
      m_detailedView->resetEntryStatus();
    } else {
      ret = false;
    }
    m_savingImageLocationChange = false;
  }

  StatusBar::self()->clearStatus();
  return ret;
}

void MainWindow::slotFileSaveAsTemplate() {
  QScopedPointer<CollectionTemplateDialog> dlg(new CollectionTemplateDialog(this));
  if(dlg->exec() != QDialog::Accepted) {
    return;
  }

  const QString templateName = dlg->templateName();
  if(templateName.isEmpty()) {
    return;
  }
  const QString baseName = Tellico::saveLocation(QStringLiteral("collection-templates/")) + templateName;

  // first, save the collection template, which copies the collection fields and filters, but nothing else
  const QString collFile = baseName + QLatin1String(".tc");
  Data::Document::self()->saveDocumentTemplate(QUrl::fromLocalFile(collFile), templateName);

  // next, save the template descriptions in a config file
  const QString specFile = baseName + QLatin1String(".spec");
  auto spec = KSharedConfig::openConfig(specFile, KConfig::SimpleConfig)->group(QString());
  spec.writeEntry("Name", templateName);
  spec.writeEntry("Comment", dlg->templateComment());
  spec.writeEntry("Icon", dlg->templateIcon());
}

void MainWindow::slotFilePrint() {
  doPrint(Print);
}

void MainWindow::slotFilePrintPreview() {
  doPrint(PrintPreview);
}

void MainWindow::doPrint(PrintAction action_) {
  slotStatusMsg(i18n("Printing..."));

  // If the collection is being filtered, warn the user
  if(m_detailedView->filter()) {
    QString str = i18n("The collection is currently being filtered to show a limited subset of "
                       "the entries. Only the visible entries will be printed. Continue?");
    int ret = KMessageBox::warningContinueCancel(this, str, QString(), KStandardGuiItem::print(),
                                                 KStandardGuiItem::cancel(), QStringLiteral("WarnPrintVisible"));
    if(ret == KMessageBox::Cancel) {
      StatusBar::self()->clearStatus();
      return;
    }
  }

  if(!m_printHandler) {
    m_printHandler = new PrintHandler(this);
  }
  m_printHandler->setEntries(m_detailedView->visibleEntries());
  m_printHandler->setColumns(m_detailedView->visibleColumns());
  if(action_ == Print) {
    m_printHandler->print();
  } else {
    m_printHandler->printPreview();
  }

  StatusBar::self()->clearStatus();
}

void MainWindow::slotFileQuit() {
  slotStatusMsg(i18n("Exiting..."));

  close(); // will call queryClose()

  StatusBar::self()->clearStatus();
}

void MainWindow::slotEditCut() {
  activateEditSlot("cut()");
}

void MainWindow::slotEditCopy() {
  activateEditSlot("copy()");
}

void MainWindow::slotEditPaste() {
  activateEditSlot("paste()");
}

void MainWindow::activateEditSlot(const char* slot_) {
  // the edit widget is the only one that copies, cuts, and pastes
  // the entry view can copy
  QWidget* w;
  if(m_editDialog->isVisible()) {
    w = m_editDialog->focusWidget();
  } else {
    w = qApp->focusWidget();
  }

  while(w && w->isVisible()) {
    const QMetaObject* meta = w->metaObject();
    const int idx = meta->indexOfSlot(slot_);
    if(idx > -1) {
//      myDebug() << "MainWindow invoking" << meta->method(idx).methodSignature();
      meta->method(idx).invoke(w, Qt::DirectConnection);
      break;
    } else {
//      myDebug() << "did not find" << slot_ << "in" << meta->className();
      w = qobject_cast<QWidget*>(w->parent());
    }
  }
}

void MainWindow::slotEditSelectAll() {
  m_detailedView->selectAllVisible();
}

void MainWindow::slotEditDeselect() {
  Controller::self()->slotUpdateSelection(Data::EntryList());
}

void MainWindow::slotToggleEntryEditor() {
  if(m_toggleEntryEditor->isChecked()) {
    m_editDialog->show();
  } else {
    m_editDialog->hide();
  }
}

void MainWindow::slotShowConfigDialog() {
  if(!m_configDlg) {
    m_configDlg = new ConfigDialog(this);
    connect(m_configDlg, &ConfigDialog::signalConfigChanged,
            this, &MainWindow::slotHandleConfigChange);
    connect(m_configDlg, &QDialog::finished,
            this, &MainWindow::slotHideConfigDialog);
  } else {
    activateDialog(m_configDlg);
  }
  m_configDlg->show();
}

void MainWindow::slotHideConfigDialog() {
  if(m_configDlg) {
    m_configDlg->hide();
    m_configDlg->deleteLater();
    m_configDlg = nullptr;
  }
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
    text += QLatin1Char(' ');
    text += i18n("(%1 filtered; %2 selected)", filterCount, selectCount);
  } else if(filterCount < count) {
    text += QLatin1Char(' ');
    text += i18n("(%1 filtered)", filterCount);
  } else if(selectCount > 1) {
    text += QLatin1Char(' ');
    text += i18n("(%1 selected)", selectCount);
  }

  m_statusBar->setCount(text);
}

void MainWindow::slotEnableOpenedActions() {
  slotUpdateToolbarIcons();

  updateCollectionActions();

  // close the filter dialog when a new collection is opened
  slotHideFilterDialog();
  slotStringMacroDialogFinished();
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
  const QStringList articles = Config::articleList();
  const QStringList nocaps = Config::noCapitalizationList();
  const QStringList suffixes = Config::nameSuffixList();
  const QStringList prefixes = Config::surnamePrefixList();

  m_configDlg->saveConfiguration();

  // only modified if there are entries and image location is changed
  if(imageLocation != Config::imageLocation() && !Data::Document::self()->isEmpty()) {
    slotImageLocationChanged();
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
    Controller::self()->slotRefreshField(Data::Document::self()->collection()->fieldByName(QStringLiteral("title")));
  }

  QString entryXSLTFile = Config::templateName(Kernel::self()->collectionType());
  m_entryView->setXSLTFile(entryXSLTFile + QLatin1String(".xsl"));
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
      groupMap.insert(groupName, QLatin1Char('<') + i18n("People") + QLatin1Char('>'));
    } else {
      groupMap.insert(groupName, coll_->fieldTitleByName(groupName));
    }
  }

  const QStringList titles = groupMap.values();
  if(titles == m_entryGrouping->items()) {
    // no need to update anything
    return;
  }
  const QStringList names = groupMap.keys();
  int index = names.indexOf(current);
  if(index == -1) {
    current = names[0];
    index = 0;
  }
  m_entryGrouping->setItems(titles);
  m_entryGrouping->setCurrentItem(index);
  // in case the current grouping field get modified to be non-grouping...
  m_groupView->setGroupField(current); // don't call slotChangeGrouping() since it adds an undo item

  // TODO::I have no idea how to get the combobox to update its size
  // this is the hackiest of hacks, taken from KXmlGuiWindow::saveNewToolbarConfig()
  // the window flickers as toolbar resizes, unavoidable?
  // crashes if removeClient//addClient is called here, need to do later in event loop
  QTimer::singleShot(0, this, &MainWindow::guiFactoryReset);
}

void MainWindow::slotChangeGrouping() {
  const QString title = m_entryGrouping->currentText();

  QString groupName = Data::Document::self()->collection()->fieldNameByTitle(title);
  if(groupName.isEmpty()) {
    if(title == (QLatin1Char('<') + i18n("People") + QLatin1Char('>'))) {
      groupName = Data::Collection::s_peopleGroupName;
    } else {
      groupName = Data::Document::self()->collection()->defaultGroupField();
    }
  }
  m_groupView->setGroupField(groupName);
  m_viewTabs->setCurrentWidget(m_groupView);
}

void MainWindow::slotShowReportDialog() {
  if(!m_reportDlg) {
    m_reportDlg = new ReportDialog(this);
    connect(m_reportDlg, &QDialog::finished,
            this, &MainWindow::slotHideReportDialog);
  } else {
    activateDialog(m_reportDlg);
  }
  m_reportDlg->show();
}

void MainWindow::slotHideReportDialog() {
  if(m_reportDlg) {
    m_reportDlg->hide();
    m_reportDlg->deleteLater();
    m_reportDlg = nullptr;
  }
}

void MainWindow::XSLTError() {
  QString str = i18n("Tellico encountered an error in XSLT processing.") + QLatin1Char('\n');
  str += i18n("Please check your installation.");
  Kernel::self()->sorry(str);
}

void MainWindow::slotShowFilterDialog() {
  if(!m_filterDlg) {
    m_filterDlg = new FilterDialog(FilterDialog::CreateFilter, this); // allow saving
    m_quickFilter->setEnabled(false);
    connect(m_filterDlg, &FilterDialog::signalCollectionModified,
            Data::Document::self(), &Data::Document::slotSetModified);
    connect(m_filterDlg, &FilterDialog::signalUpdateFilter,
            this, &MainWindow::slotUpdateFilter);
    connect(m_filterDlg, &QDialog::finished,
            this, &MainWindow::slotHideFilterDialog);
  } else {
    activateDialog(m_filterDlg);
  }
  m_filterDlg->setFilter(m_detailedView->filter());
  m_filterDlg->show();
}

void MainWindow::slotHideFilterDialog() {
//  m_quickFilter->blockSignals(false);
  m_quickFilter->setEnabled(true);
  if(m_filterDlg) {
    m_filterDlg->hide();
    m_filterDlg->deleteLater();
    m_filterDlg = nullptr;
  }
}

void MainWindow::slotQueueFilter() {
  if(m_dontQueueFilter) {
    return;
  }
  m_queuedFilters++;
  QTimer::singleShot(200, this, &MainWindow::slotCheckFilterQueue);
}

void MainWindow::slotCheckFilterQueue() {
  m_queuedFilters--;
  if(m_queuedFilters > 0) {
    return;
  }

  setFilter(m_quickFilter->text());
}

void MainWindow::slotUpdateFilter(FilterPtr filter_) {
  // Can't just block signals because clear button won't show then
  m_dontQueueFilter = true;
  if(filter_) {
    // for a saved filter, show the filter name and a leading icon
    if(m_quickFilter->actions().isEmpty()) {
      m_quickFilter->addAction(QIcon::fromTheme(QStringLiteral("view-filter")), QLineEdit::LeadingPosition);
    }
    m_quickFilter->setText(QLatin1Char('<') + filter_->name() + QLatin1Char('>'));
  } else {
    m_quickFilter->setText(QStringLiteral(" ")); // To be able to clear custom filter
  }
  Controller::self()->slotUpdateFilter(filter_);
  m_dontQueueFilter = false;
}

void MainWindow::setFilter(const QString& text_) {
  // might have an "action" associated if a saved filter was displayed
  auto actions = m_quickFilter->actions();
  if(!actions.isEmpty()) {
    // clear all of the saved filter name
    slotClearFilter();
    return;
  }
  // update the line edit in case the filter was set by DBUS
  m_quickFilter->setText(text_);

  FilterParser parser(text_.trimmed(), Config::quickFilterRegExp());
  parser.setCollection(Data::Document::self()->collection());
  FilterPtr filter = parser.filter();
  // only update filter if one exists or did exist
  if(filter || m_detailedView->filter()) {
    Controller::self()->slotUpdateFilter(filter);
  }
}

void MainWindow::slotShowCollectionFieldsDialog() {
  if(!m_collFieldsDlg) {
    m_collFieldsDlg = new CollectionFieldsDialog(Data::Document::self()->collection(), this);
    m_collFieldsDlg->setNotifyKernel(true);
    connect(m_collFieldsDlg, &CollectionFieldsDialog::beginCommandGroup,
            Kernel::self(), &Kernel::beginCommandGroup);
    connect(m_collFieldsDlg, &CollectionFieldsDialog::endCommandGroup,
            Kernel::self(), &Kernel::endCommandGroup);
    connect(m_collFieldsDlg, &CollectionFieldsDialog::addField,
            Kernel::self(), &Kernel::addField);
    connect(m_collFieldsDlg, &CollectionFieldsDialog::modifyField,
            Kernel::self(), &Kernel::modifyField);
    connect(m_collFieldsDlg, &CollectionFieldsDialog::removeField,
            Kernel::self(), &Kernel::removeField);
    connect(m_collFieldsDlg, &CollectionFieldsDialog::reorderFields,
            Kernel::self(), &Kernel::reorderFields);
    connect(m_collFieldsDlg, &QDialog::finished,
            this, &MainWindow::slotHideCollectionFieldsDialog);
  } else {
    activateDialog(m_collFieldsDlg);
  }
  m_collFieldsDlg->show();
}

void MainWindow::slotHideCollectionFieldsDialog() {
  if(m_collFieldsDlg) {
    m_collFieldsDlg->hide();
    m_collFieldsDlg->deleteLater();
    m_collFieldsDlg = nullptr;
  }
}

void MainWindow::slotFileImport(int format_) {
  slotStatusMsg(i18n("Importing data..."));
  m_quickFilter->clear();

  Import::Format format = static_cast<Import::Format>(format_);
  bool checkURL = true;
  QUrl url;
  switch(ImportDialog::importTarget(format)) {
    case Import::File:
      {
        QString fileClass;
        const QUrl startUrl = KFileWidget::getStartUrl(QUrl(QStringLiteral("kfiledialog:///import")), fileClass);
        url = QFileDialog::getOpenFileUrl(this, i18n("Import File"), startUrl, ImportDialog::fileFilter(format));
        KRecentDirs::add(fileClass, url.adjusted(QUrl::RemoveFilename|QUrl::StripTrailingSlash).path());
      }
      break;

    case Import::Dir:
      // TODO: allow remote audiofile importing
      {
        const QString fileClass(QStringLiteral("ImportDir"));
        QString dirName = ImportDialog::startDir(format);
        if(dirName.isEmpty()) {
          dirName = KRecentDirs::dir(fileClass);
        }
        QString chosenDir = QFileDialog::getExistingDirectory(this, i18n("Import Directory"), dirName);
        url = QUrl::fromLocalFile(chosenDir);
        KRecentDirs::add(fileClass, chosenDir);
      }
      break;

    case Import::None:
    default:
      checkURL = false;
      break;
  }

  if(checkURL) {
    bool ok = !url.isEmpty() && url.isValid() && QFile::exists(url.toLocalFile());
    if(!ok) {
      StatusBar::self()->clearStatus();
      return;
    }
  }
  importFile(format, QList<QUrl>() << url);
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
      QString fileClass;
      const QUrl startUrl = KFileWidget::getStartUrl(QUrl(QStringLiteral("kfiledialog:///export")), fileClass);
      QUrl url = QFileDialog::getSaveFileUrl(this, i18n("Export As"), startUrl, dlg.fileFilter());
      if(url.isEmpty()) {
        StatusBar::self()->clearStatus();
        return;
      }

      if(url.isValid()) {
        if(url.isLocalFile()) {
          KRecentDirs::add(fileClass, url.adjusted(QUrl::RemoveFilename|QUrl::StripTrailingSlash).path());
        }
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
    m_stringMacroDlg->setWindowTitle(i18n("String Macros"));
    m_stringMacroDlg->setLabels(i18n("Macro"), i18n("String"));
    connect(m_stringMacroDlg, &QDialog::finished, this, &MainWindow::slotStringMacroDialogFinished);
  } else {
    activateDialog(m_stringMacroDlg);
  }
  m_stringMacroDlg->show();
}

void MainWindow::slotStringMacroDialogFinished(int result_) {
  // no point in checking if collection is bibtex, as dialog would never have been created
  if(!m_stringMacroDlg) {
    return;
  }
  if(result_ == QDialog::Accepted) {
    static_cast<Data::BibtexCollection*>(Data::Document::self()->collection().data())->setMacroList(m_stringMacroDlg->stringMap());
    Data::Document::self()->setModified(true);
  }
  m_stringMacroDlg->hide();
  m_stringMacroDlg->deleteLater();
  m_stringMacroDlg = nullptr;
}

void MainWindow::slotShowBibtexKeyDialog() {
  if(Data::Document::self()->collection()->type() != Data::Collection::Bibtex) {
    return;
  }

  if(!m_bibtexKeyDlg) {
    m_bibtexKeyDlg = new BibtexKeyDialog(Data::Document::self()->collection(), this);
    connect(m_bibtexKeyDlg, &QDialog::finished, this, &MainWindow::slotHideBibtexKeyDialog);
    connect(m_bibtexKeyDlg, &BibtexKeyDialog::signalUpdateFilter,
            this, &MainWindow::slotUpdateFilter);
  } else {
    activateDialog(m_bibtexKeyDlg);
  }
  m_bibtexKeyDlg->show();
}

void MainWindow::slotHideBibtexKeyDialog() {
  if(m_bibtexKeyDlg) {
    m_bibtexKeyDlg->deleteLater();
    m_bibtexKeyDlg = nullptr;
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
  activateDialog(m_editDialog);
}

void MainWindow::slotConvertToBibliography() {
  // only book collections can be converted to bibtex
  Data::CollPtr coll = Data::Document::self()->collection();
  if(!coll || coll->type() != Data::Collection::Book) {
    return;
  }

  GUI::CursorSaver cs;

  // need to make sure all images are transferred
  Data::Document::self()->loadAllImagesNow();

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
  Cite::ActionManager* man = Cite::ActionManager::self();
  man->cite(static_cast<Cite::CiteAction>(action_), Controller::self()->selectedEntries());
  if(man->hasError()) {
    Kernel::self()->sorry(man->errorString());
  }
  StatusBar::self()->clearStatus();
}

void MainWindow::slotShowFetchDialog() {
  if(!m_fetchDlg) {
    m_fetchDlg = new FetchDialog(this);
    connect(m_fetchDlg, &QDialog::finished, this, &MainWindow::slotHideFetchDialog);
    connect(Controller::self(), &Controller::collectionAdded, m_fetchDlg, &FetchDialog::slotResetCollection);
  } else {
    activateDialog(m_fetchDlg);
  }
  m_fetchDlg->show();
}

void MainWindow::slotHideFetchDialog() {
  if(m_fetchDlg) {
    m_fetchDlg->hide();
    m_fetchDlg->deleteLater();
    m_fetchDlg = nullptr;
  }
}

bool MainWindow::importFile(Tellico::Import::Format format_, const QUrl& url_, Tellico::Import::Action action_) {
  // try to open document
  GUI::CursorSaver cs(Qt::WaitCursor);

  bool failed = false;
  Data::CollPtr coll;
  if(!url_.isEmpty() && url_.isValid() && NetAccess::exists(url_, true, this)) {
    coll = ImportDialog::importURL(format_, url_);
  } else {
    Kernel::self()->sorry(TC_I18N2(errorLoad, url_.fileName()));
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

bool MainWindow::exportCollection(Tellico::Export::Format format_, const QUrl& url_, bool filtered_) {
  if(!url_.isValid()) {
    myDebug() << "invalid URL:" << url_;
    return false;
  }

  GUI::CursorSaver cs;
  const Data::CollPtr coll = Data::Document::self()->collection();
  if(!coll) {
    return false;
  }

  // only bibliographies can export to bibtex or bibtexml
  const bool isBibtex = (coll->type() == Data::Collection::Bibtex);
  if(!isBibtex && (format_ == Export::Bibtex || format_ == Export::Bibtexml)) {
    return false;
  }
  // only books and bibliographies can export to alexandria
  const bool isBook = (coll->type() == Data::Collection::Book);
  if(!isBibtex && !isBook && format_ == Export::Alexandria) {
    return false;
  }

  return ExportDialog::exportCollection(coll, filtered_ ? Controller::self()->visibleEntries() : coll->entries(),
                                        format_, url_);
}

bool MainWindow::showEntry(Data::ID id) {
  Data::EntryPtr entry = Data::Document::self()->collection()->entryById(id);
  if(entry) {
    m_entryView->showEntry(entry);
  }
  return entry;
}

void MainWindow::addFilterView() {
  if(m_filterView) {
    return;
  }

  m_filterView = new FilterView(m_viewTabs);
  Controller::self()->addObserver(m_filterView);
  m_viewTabs->insertTab(1, m_filterView, QIcon::fromTheme(QStringLiteral("view-filter")), i18n("Filters"));
  m_filterView->setWhatsThis(i18n("<qt>The <i>Filter View</i> shows the entries which meet certain "
                                  "filter rules.</qt>"));

  connect(m_filterView, &FilterView::signalUpdateFilter,
          this, &MainWindow::slotUpdateFilter);
  // use the EntrySelectionModel as a proxy so when entries get selected in the filter view
  // the edit dialog and entry view are updated
  // TODO: consider using KSelectionProxyModel
  static_cast<EntrySelectionModel*>(m_iconView->selectionModel())->addSelectionProxy(m_filterView->selectionModel());

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
  m_viewTabs->insertTab(2, m_loanView, QIcon::fromTheme(QStringLiteral("kaddressbook")), i18n("Loans"));
  m_loanView->setWhatsThis(i18n("<qt>The <i>Loan View</i> shows a list of all the people who "
                                "have borrowed items from your collection.</qt>"));

  // use the EntrySelectionModel as a proxy so when entries get selected in the loan view
  // the edit dialog and entry view are updated
  // TODO: consider using KSelectionProxyModel
  static_cast<EntrySelectionModel*>(m_iconView->selectionModel())->addSelectionProxy(m_loanView->selectionModel());

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
       caption += QLatin1String(" - ");
    }
    QUrl u = Data::Document::self()->URL();
    if(u.isLocalFile() && u.fileName() == TC_I18N1(Tellico::untitledFilename)) {
      // for new files, the filename is set to Untitled in Data::Document
      caption += u.fileName();
    } else {
      caption += u.toDisplayString(QUrl::PreferLocalFile);
    }
  }
  setCaption(caption, modified_);
}

void MainWindow::slotUpdateToolbarIcons() {
  // first change the icon for the menu item
  if(Kernel::self()->collectionType() == Data::Collection::Base) {
    m_newEntry->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
  } else {
    m_newEntry->setIcon(QIcon(QLatin1String(":/icons/") + Kernel::self()->collectionTypeName()));
  }
}

void MainWindow::slotGroupLabelActivated() {
  // need entry grouping combo id
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  for(auto obj : m_entryGrouping->associatedWidget()) {
#else
  for(auto obj : m_entryGrouping->associatedObjects()) {
#endif
    if(auto widget = ::qobject_cast<KToolBar*>(obj)) {
      auto container = m_entryGrouping->requestWidget(widget);
      auto combo = ::qobject_cast<QComboBox*>(container); //krazy:exclude=qclasses
      if(combo) {
        combo->showPopup();
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
  auto actions = m_quickFilter->actions();
  if(!actions.isEmpty()) {
    m_quickFilter->removeAction(actions.first());
  }
  m_quickFilter->clear();
  slotQueueFilter();
}

void MainWindow::slotRenameCollection() {
  Kernel::self()->renameCollection();
}

void MainWindow::slotImageLocationMismatch() {
  // TODO: having a single image location mismatch should not be reason to completely save the whole document
  QTimer::singleShot(0, this, &MainWindow::slotImageLocationChanged);
}

void MainWindow::slotImageLocationChanged() {
  if(m_savingImageLocationChange) {
    return;
  }
  m_savingImageLocationChange = true;
  Data::Document::self()->slotSetModified();
  KMessageBox::information(this, QLatin1String("<qt>") +
                                 i18n("Some images are not saved in the configured location. The current file "
                                      "must be saved and the images will be transferred to the new location.") +
                                 QLatin1String("</qt>"));
  fileSave();
  m_savingImageLocationChange = false;
}

void MainWindow::updateCollectionActions() {
  if(!Data::Document::self()->collection()) {
    return;
  }

  stateChanged(QStringLiteral("collection_reset"));

  Data::Collection::Type type = Data::Document::self()->collection()->type();
  stateChanged(QLatin1String("is_") + CollectionFactory::typeName(type));

  Controller::self()->updateActions();
  // special case when there are no available data sources
  if(m_fetchActions.isEmpty() && m_updateAll) {
    m_updateAll->setEnabled(false);
  }
}

void MainWindow::updateEntrySources() {
  const QString actionListName = QStringLiteral("update_entry_actions");
  unplugActionList(actionListName);
  for(auto action : m_fetchActions) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    for(auto obj : action->associatedWidgets()) {
#else
    for(auto obj : action->associatedObjects()) {
#endif
      if(auto widget = ::qobject_cast<QWidget*>(obj)) {
        widget->removeAction(action);
      }
    }
    m_updateMapper->removeMappings(action);
  }
  qDeleteAll(m_fetchActions);
  m_fetchActions.clear();

  void (QAction::* triggeredBool)(bool) = &QAction::triggered;
  void (QSignalMapper::* mapVoid)() = &QSignalMapper::map;
  Fetch::FetcherVec vec = Fetch::Manager::self()->fetchers(Kernel::self()->collectionType());
  foreach(Fetch::Fetcher::Ptr fetcher, vec) {
    QAction* action = new QAction(Fetch::Manager::fetcherIcon(fetcher.data()), fetcher->source(), actionCollection());
    action->setToolTip(i18n("Update entry data from %1", fetcher->source()));
    connect(action, triggeredBool, m_updateMapper, mapVoid);
    m_updateMapper->setMapping(action, fetcher->source());
    m_fetchActions.append(action);
  }

  plugActionList(actionListName, m_fetchActions);
}

void MainWindow::importFile(Tellico::Import::Format format_, const QList<QUrl>& urls_) {
  QList<QUrl> urls = urls_;
  // update as DropHandler and Importer classes are updated
  if(urls_.count() > 1 &&
     format_ != Import::Bibtex &&
     format_ != Import::RIS &&
     format_ != Import::CIW &&
     format_ != Import::PDF) {
    QUrl u = urls_.front();
    QString url = u.isLocalFile() ? u.path() : u.toDisplayString();
    Kernel::self()->sorry(i18n("Tellico can only import one file of this type at a time. "
                               "Only %1 will be imported.", url));
    urls.clear();
    urls += u;
  }

  ImportDialog dlg(format_, urls, this);
  if(dlg.exec() != QDialog::Accepted) {
    return;
  }

//  if edit dialog is saved ok and if replacing, then the doc is saved ok
  if(m_editDialog->queryModified() &&
     (dlg.action() != Import::Replace || querySaveModified())) {
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

void MainWindow::importText(Tellico::Import::Format format_, const QString& text_) {
  if(text_.isEmpty()) {
    return;
  }
  Data::CollPtr coll = ImportDialog::importText(format_, text_);
  if(coll) {
    importCollection(coll, Import::Merge);
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
          Kernel::self()->sorry(TC_I18N1(errorAppendType));
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
          Kernel::self()->sorry(TC_I18N1(errorMergeType));
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
  // tell the entry views and models that there are no further images to load
  m_detailedView->slotRefreshImages();
  return !failed;
}

void MainWindow::slotURLAction(const QUrl& url_) {
  Q_ASSERT(url_.scheme() == QLatin1String("tc"));
  const QString actionName = url_.fileName();
  if(actionName == QLatin1String("disable_welcome")) {
    Config::setShowWelcome(false);
    m_entryView->showText(QString());
    return; // done
  }
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  QAction* action = this->action(actionName.toLatin1().constData());
#else
  QAction* action = this->action(actionName);
#endif
  if(action) {
    action->activate(QAction::Trigger);
  } else {
    myWarning() << "unknown action: " << actionName;
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
  return KXmlGuiWindow::eventFilter(obj_, ev_);
}

void MainWindow::slotToggleFullScreen() {
  Qt::WindowStates ws = windowState();
  setWindowState((ws & Qt::WindowFullScreen) ? (ws & ~Qt::WindowFullScreen) : (ws | Qt::WindowFullScreen));
}

void MainWindow::slotToggleMenuBarVisibility() {
  QMenuBar* mb = menuBar();
  mb->isHidden() ? mb->show() : mb->hide();
}

void MainWindow::slotToggleLayoutLock(bool lock_) {
  m_groupViewDock->setLocked(lock_);
  m_collectionViewDock->setLocked(lock_);
}

void MainWindow::slotResetLayout() {
  removeDockWidget(m_groupViewDock);
  addDockWidget(Qt::LeftDockWidgetArea, m_groupViewDock);
  m_groupViewDock->show();

  m_dummyWindow->removeDockWidget(m_collectionViewDock);
  m_dummyWindow->addDockWidget(Qt::TopDockWidgetArea, m_collectionViewDock);
  m_collectionViewDock->show();
}

void MainWindow::guiFactoryReset() {
  guiFactory()->removeClient(this);
  guiFactory()->reset();
  guiFactory()->addClient(this);

  // set up custom actions for collection templates, have to do this AFTER createGUI() or factory() reset
  const QString actionListName = QStringLiteral("collection_template_list");
  unplugActionList(actionListName);
  QSignalMapper* collectionTemplateMapper = new QSignalMapper(this);
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
  void (QSignalMapper::* mappedString)(QString) = &QSignalMapper::mapped;
  connect(collectionTemplateMapper, mappedString, this, &MainWindow::slotFileNewByTemplate);
#else
  connect(collectionTemplateMapper, &QSignalMapper::mappedString, this, &MainWindow::slotFileNewByTemplate);
#endif

  void (QAction::* triggeredBool)(bool) = &QAction::triggered;
  void (QSignalMapper::* mapVoid)() = &QSignalMapper::map;
  QList<QAction*> coll_actions;
  const QStringList customCollections = Tellico::locateAllFiles(QStringLiteral("tellico/collection-templates/*.tc"));
  if(!customCollections.isEmpty()) {
    m_newCollectionMenu->addSeparator();
  }
  foreach(const QString& collectionFile, customCollections) {
    QFileInfo info(collectionFile);
    auto action = new QAction(info.completeBaseName(), actionCollection());
    connect(action, triggeredBool, collectionTemplateMapper, mapVoid);
    const QString specFile = info.canonicalPath() + QDir::separator() + info.completeBaseName() + QLatin1String(".spec");
    if(QFileInfo::exists(specFile)) {
      KConfig config(specFile, KConfig::SimpleConfig);
      const KConfigGroup cg = config.group(QString());
      action->setText(cg.readEntry("Name", info.completeBaseName()));
      action->setToolTip(cg.readEntry("Comment"));
      action->setIcon(QIcon::fromTheme(cg.readEntry("Icon"), QIcon::fromTheme(QStringLiteral("document-new"))));
    } else {
      myDebug() << "No spec file for" << info.completeBaseName();
      action->setText(info.completeBaseName());
      action->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
    }
    collectionTemplateMapper->setMapping(action, collectionFile);
    coll_actions.append(action);
    m_newCollectionMenu->addAction(action);
  }
  plugActionList(actionListName, coll_actions);
}

void MainWindow::showLog() {
  auto dlg = new QDialog(this);
  auto layout = new QVBoxLayout();
  dlg->setLayout(layout);
  dlg->setWindowTitle(i18nc("@title:window", "Tellico Log"));

  auto viewer = new QTextEdit(dlg);
  viewer->setWordWrapMode(QTextOption::NoWrap);
  viewer->setReadOnly(true);
  viewer->setStyleSheet(QStringLiteral("QTextEdit { font-family: monospace; }"));
  layout->addWidget(viewer);

  auto buttonBox = new QDialogButtonBox(dlg);
  buttonBox->setStandardButtons(QDialogButtonBox::Close);
  connect(buttonBox, &QDialogButtonBox::rejected, dlg, &QDialog::reject);
  layout->addWidget(buttonBox);

  auto logFile = Logger::self()->logFile();
  if(!logFile.isEmpty()) {
    auto timer = new QTimer(dlg);
    timer->setSingleShot(true);
    timer->setInterval(1000);
    timer->callOnTimeout([logFile, viewer]() {
      Logger::self()->flush();
      QFile file(logFile);
      if(file.open(QFile::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        viewer->setPlainText(in.readAll());
      }
    });
    connect(Logger::self(), &Logger::updated, timer, QOverload<>::of(&QTimer::start));
    myLog() << "Showing log viewer"; // this triggers the first read of the log file
  }

  dlg->setMinimumSize(800, 600);
  dlg->setAttribute(Qt::WA_DeleteOnClose, true);
  dlg->show();
}
