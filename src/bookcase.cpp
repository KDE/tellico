/***************************************************************************
                          bookcase.cpp  -  description
                             -------------------
    begin                : Wed Aug 29 21:00:54 CEST 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@radiojodi.com
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
#include "bclistview.h"
#include "bcunit.h"
#include "bcunititem.h"
#include "bookcase.h"
#include "bcuniteditwidget.h"
#include "bccollectionview.h"

//#include <kiconloader.h>
//#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h>
#include <ktoolbarradiogroup.h>
//#include <kprogress.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstdaction.h>
#include <kaction.h>
#include <kdebug.h>

// include files for QT
#include <qdir.h>
//#include <qprinter.h>
//#include <qpainter.h>

#define ID_STATUS_MSG 1

Bookcase::Bookcase(QWidget* parent_/*=0*/, const char* name_/*=0*/)
 : KDockMainWindow(parent_, name_) {
  m_config = kapp->config();

  initWindow();
  initStatusBar();
  initActions();
  initDocument();
  initView();

  readOptions();

  // allow status messages from the document
  connect(m_doc, SIGNAL(signalStatusMsg(const QString&)),
          this, SLOT(slotStatusMsg(const QString&)));

  // when a new document is initialized, the listviews must be reset
  connect(m_doc, SIGNAL(signalNewDoc()),
          m_ew, SLOT(slotReset()));
  connect(m_doc, SIGNAL(signalNewDoc()),
          m_cv, SLOT(slotReset()));
  connect(m_doc, SIGNAL(signalNewDoc()),
          m_bclv, SLOT(slotReset()));

  // the two listviews signal when a unit is selected, pass it to the edit widget
  connect(m_cv, SIGNAL(signalUnitSelected(BCUnit*)),
          m_ew,  SLOT(slotSetContents(BCUnit*)));
  connect(m_bclv, SIGNAL(signalUnitSelected(BCUnit*)),
          m_ew,  SLOT(slotSetContents(BCUnit*)));

  // synchronize the selected signal between listviews
  connect(m_cv, SIGNAL(signalUnitSelected(BCUnit*)),
          m_bclv, SLOT(slotSetSelected(BCUnit*)));
  connect(m_bclv, SIGNAL(signalUnitSelected(BCUnit*)),
          m_cv, SLOT(slotSetSelected(BCUnit*)));

  // the two listviews also signal when they're cleared
  connect(m_cv, SIGNAL(signalClear()),
          m_ew,  SLOT(slotHandleClear()));
  connect(m_bclv, SIGNAL(signalClear()),
          m_ew,  SLOT(slotHandleClear()));

  connect(m_ew, SIGNAL(signalDoUnitSave(BCUnit*)),
          m_doc, SLOT(slotSaveUnit(BCUnit*)));

  connect(m_bclv, SIGNAL(signalDoUnitDelete(BCUnit*)),
          this, SLOT(slotDeleteUnit(BCUnit*)));
  connect(m_ew, SIGNAL(signalDoUnitDelete(BCUnit*)),
          this, SLOT(slotDeleteUnit(BCUnit*)));

  // connect the modified signal
  connect(m_doc, SIGNAL(signalCollectionModified(BCCollection*)),
          m_cv, SLOT(slotReset()));
  connect(m_doc, SIGNAL(signalCollectionModified(BCCollection*)),
          m_bclv, SLOT(slotReset()));
  connect(m_doc, SIGNAL(signalCollectionModified(BCCollection*)),
          m_ew, SLOT(slotReset()));

  connect(m_doc, SIGNAL(signalCollectionAdded(BCCollection*)),
          m_cv, SLOT(slotAddItem(BCCollection*)));
  connect(m_doc, SIGNAL(signalCollectionAdded(BCCollection*)),
          m_bclv, SLOT(slotAddPage(BCCollection*)));
  connect(m_doc, SIGNAL(signalCollectionAdded(BCCollection*)),
          m_ew, SLOT(slotAddPage(BCCollection*)));

  connect(m_doc, SIGNAL(signalCollectionDeleted(BCCollection*)),
          m_cv, SLOT(slotRemoveItem(BCCollection*)));
  connect(m_doc, SIGNAL(signalCollectionDeleted(BCCollection*)),
          m_bclv, SLOT(slotRemovePage(BCCollection*)));
  connect(m_doc, SIGNAL(signalCollectionDeleted(BCCollection*)),
          m_ew, SLOT(slotRemovePage(BCCollection*)));

  // connect the added signal to both listviews
  connect(m_doc, SIGNAL(signalUnitAdded(BCUnit*)),
          m_cv, SLOT(slotAddItem(BCUnit*)));
  connect(m_doc, SIGNAL(signalUnitAdded(BCUnit*)),
          m_bclv, SLOT(slotAddItem(BCUnit*)));

  // connect the modified signal to both
  connect(m_doc, SIGNAL(signalUnitModified(BCUnit*)),
          m_cv, SLOT(slotModifyItem(BCUnit*)));
  connect(m_doc, SIGNAL(signalUnitModified(BCUnit*)),
          m_bclv, SLOT(slotModifyItem(BCUnit*)));

  // connect the deleted signal to both listviews
  connect(m_doc, SIGNAL(signalUnitDeleted(BCUnit*)),
          m_cv, SLOT(slotRemoveItem(BCUnit*)));
  connect(m_doc, SIGNAL(signalUnitDeleted(BCUnit*)),
          m_bclv, SLOT(slotRemoveItem(BCUnit*)));

  connect(m_cv, SIGNAL(signalDoCollectionRename(int, const QString&)),
          m_doc, SLOT(slotRenameCollection(int, const QString&)));

  //m_fileSave->setEnabled(false);
  //m_fileSaveAs->setEnabled(false);
  m_filePrint->setEnabled(false);
  m_editCut->setEnabled(false);
  m_editCopy->setEnabled(false);
  m_editPaste->setEnabled(false);
  m_editFind->setEnabled(false);
  m_editFindNext->setEnabled(false);
  m_preferences->setEnabled(false);
}

Bookcase::~Bookcase() {
  delete m_doc;
  m_doc = NULL;
  delete m_cv;
  m_cv = NULL;
  delete m_bclv;
  m_bclv = NULL;
  delete m_ew;
  m_ew = NULL;
}

void Bookcase::initActions() {
  m_fileNew = KStdAction::openNew(this, SLOT(slotFileNew()), actionCollection());
  m_fileOpen = KStdAction::open(this, SLOT(slotFileOpen()), actionCollection());
  m_fileOpenRecent = KStdAction::openRecent(this, SLOT(slotFileOpenRecent(const KURL&)),
                                             actionCollection());
  m_fileSave = KStdAction::save(this, SLOT(slotFileSave()), actionCollection());
  m_fileSaveAs = KStdAction::saveAs(this, SLOT(slotFileSaveAs()), actionCollection());
  m_fileClose = KStdAction::close(this, SLOT(slotFileClose()), actionCollection());
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
  m_preferences = KStdAction::preferences(this, SLOT(slotPreferences()), actionCollection());

  m_fileNewCollection = new KAction(i18n("New &Collection"), 0, this,
                        SLOT(slotFileNewCollection()), actionCollection(), "file_new_collection");

  m_fileNew->setStatusText(i18n("Creates a new document"));
  m_fileOpen->setStatusText(i18n("Opens an existing document"));
  m_fileOpenRecent->setStatusText(i18n("Opens a recently used file"));
  m_fileSave->setStatusText(i18n("Saves the actual document"));
  m_fileSaveAs->setStatusText(i18n("Saves the actual document as..."));
  m_fileClose->setStatusText(i18n("Closes the actual document"));
  m_filePrint ->setStatusText(i18n("Prints out the actual document"));
  m_fileQuit->setStatusText(i18n("Quits the application"));
  m_editCut->setStatusText(i18n("Cuts the selected section and puts it to the clipboard"));
  m_editCopy->setStatusText(i18n("Copies the selected section to the clipboard"));
  m_editPaste->setStatusText(i18n("Pastes the clipboard contents to actual position"));
  m_editFind->setStatusText(i18n("Searches in the document"));
  m_toggleToolBar->setStatusText(i18n("Enables/disables the toolbar"));
  m_toggleStatusBar->setStatusText(i18n("Enables/disables the statusbar"));
  m_preferences->setStatusText(i18n("Configure the options for the application"));

  createGUI();
}

void Bookcase::initStatusBar() {
  statusBar()->insertItem(i18n("Ready."), ID_STATUS_MSG);
  statusBar()->setItemAlignment(ID_STATUS_MSG, Qt::AlignLeft | Qt::AlignVCenter);
}


void Bookcase::initWindow() {
  KIconLoader* loader = KGlobal::iconLoader();
  if(loader) {
    setIcon(loader->loadIcon("bookcase", KIcon::Desktop));
  }
}

void Bookcase::initView() {
  KDockWidget* mainDock = createDockWidget(i18n("Collection Contents"), QPixmap());
  mainDock->setToolTipString(i18n("Collection Contents"));
  m_cv = new BCCollectionView(mainDock);
//  m_bclv->slotReset();
  mainDock->setWidget(m_cv);
  setView(mainDock);
  setMainDockWidget(mainDock);

  KDockWidget* listDock = createDockWidget(i18n("Collection Details"), QPixmap());
  listDock->setToolTipString(i18n("Collection Details"));
  m_bclv = new BCListView(listDock);
//  m_bclv->slotReset();
  listDock->setWidget(m_bclv);
  listDock->manualDock(mainDock, KDockWidget::DockRight, 20);

  KDockWidget* editDock = createDockWidget(i18n("Collection Editing"), QPixmap());
  editDock->setToolTipString(i18n("Collection Editing"));
  m_ew = new BCUnitEditWidget(editDock);
  editDock->setWidget(m_ew);
  editDock->manualDock(listDock, KDockWidget::DockBottom, 70);
//  editDock->manualDock(mainDock, KDockWidget::DockRight, 20);

  // since the document is already initialized, this just lays out things
  // a little nicer
  QListIterator<BCCollection> it(*(m_doc->collectionList()));
  for( ; it.current(); ++it) {
    m_cv->slotAddItem(it.current());
    m_bclv->slotAddPage(it.current());
    m_ew->slotAddPage(it.current());
  }
}

void Bookcase::initDocument() {
  m_doc = new BookcaseDoc(this);
}

void Bookcase::saveOptions() {
  m_config->setGroup("General Options");
  m_config->writeEntry("Geometry", size());
  m_config->writeEntry("Show Toolbar", m_toggleToolBar->isChecked());
  m_config->writeEntry("Show Statusbar",m_toggleStatusBar->isChecked());
  m_config->writeEntry("ToolBarPos", static_cast<int>(toolBar("mainToolBar")->barPos()));
  m_fileOpenRecent->saveEntries(m_config, "Recent Files");

  QValueList<int> widthList;
  for(int i = 0; i < m_bclv->columns(); i++) {
    widthList.append(m_bclv->columnWidth(i));
  }
  m_config->writeEntry("ListView Column Widths", widthList);

  writeDockConfig(m_config, "Dock Config");
}

void Bookcase::readOptions() {
  m_config->setGroup("General Options");

  // bar status settings
  bool bViewToolbar = m_config->readBoolEntry("Show Toolbar", true);
  m_toggleToolBar->setChecked(bViewToolbar);
  slotToggleToolBar();

  bool bViewStatusbar = m_config->readBoolEntry("Show Statusbar", true);
  m_toggleStatusBar->setChecked(bViewStatusbar);
  slotToggleStatusBar();

  // bar position settings
  KToolBar::BarPosition toolBarPos;
  toolBarPos=(KToolBar::BarPosition) m_config->readNumEntry("ToolBarPos", KToolBar::Top);
  toolBar("mainToolBar")->setBarPos(toolBarPos);

  // initialize the recent file list
  m_fileOpenRecent->loadEntries(m_config, "Recent Files");

  QSize size = m_config->readSizeEntry("Geometry");
  if(!size.isEmpty()) {
    resize(size);
  }

  QValueList<int> widthList = m_config->readIntListEntry("ListView Column Widths");
  QValueList<int>::Iterator it;
  int i = 0;
  for(it = widthList.begin(); it != widthList.end() && i < m_bclv->columns(); ++it, ++i) {
    m_bclv->setColumnWidthMode(i, QListView::Manual);
    m_bclv->setColumnWidth(i, static_cast<int>(*it));
  }

  readDockConfig(m_config, "Dock Config");
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
  return m_doc->saveModified();
}

bool Bookcase::queryExit() {
  saveOptions();
  return true;
}

BookcaseDoc* Bookcase::doc() {
  return m_doc;
}

/////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATION
/////////////////////////////////////////////////////////////////////

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
        i18n("*.bc|Bookcase files (*.bc)\n*.xml|XML files (*.xml)\n*|All files"), this, i18n("Open File..."));
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
  bool success = m_doc->openDocument(url_);
  if(success) {
    m_doc->setModified(false);
    setCaption(url_.fileName(), false);
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

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotFileSaveAs() {
  slotStatusMsg(i18n("Saving file with a new filename..."));

  KURL url = KFileDialog::getSaveURL(QDir::currentDirPath(),
        i18n("*.bc|Bookcase files (*.bc)\n*.xml|XML files (*.xml)\n*|All files"), this, i18n("Save as..."));
  if(!url.isEmpty()) {
    m_doc->saveDocument(url);
    m_fileOpenRecent->addURL(url);
    setCaption(url.fileName(), m_doc->isModified());
  }

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotFileClose() {
  slotStatusMsg(i18n("Closing file..."));

  close();

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotFilePrint() {
  slotStatusMsg(i18n("Printing..."));
//
//  QPrinter printer;
//  if (printer.setup(this)) {
//    view->print(&printer);
//  }

  slotStatusMsg(i18n("Ready."));
}

void Bookcase::slotFileQuit() {
  slotStatusMsg(i18n("Exiting..."));

  saveOptions();
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

void Bookcase::slotPreferences() {
}

void Bookcase::slotStatusMsg(const QString &text) {
  statusBar()->clear();
  statusBar()->changeItem(text, ID_STATUS_MSG);
}

void Bookcase::slotDeleteUnit(BCUnit* unit_) {
  m_doc->slotDeleteUnit(unit_);
  m_ew->slotHandleClear();
  m_bclv->slotSetSelected(0);
  m_cv->slotSetSelected(0);
}

void Bookcase::slotFileNewCollection() {
  kdDebug() << "Bookcase::slotFileNewCollection()\n";
}
