/***************************************************************************
                         bookcase.h
                     -------------------
        begin        : Wed Aug 29 21:00:54 CEST 2001
        copyright    : (C) 2001 by Robby Stephenson
        email        : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BOOKCASE_H
#define BOOKCASE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// forward declarations
class BookcaseDoc;
class BCDetailedListView;
class BCUnitEditWidget;
class BCGroupView;
class BCUnit;
class ConfigDialog;
class BCCollection;
class FindDialog;
class BCUnitItem;
class LookupDialog;
class BCLineEditAction;
class BCFilterDialog;
class BCCollectionPropDialog;

class KProgress;
class KToolBar;

class QCloseEvent;
class QSplitter;

// include files for KDE
#include <kapplication.h>
#include <kmainwindow.h>
#include <kaccel.h>
#include <kaction.h>

// include files for Qt
#include <qvaluelist.h>

/**
 * The base class for Bookcase application windows. It sets up the main
 * window and reads the config file as well as providing a menubar, toolbar
 * and statusbar. Bookcase reimplements the methods that KMainWindow provides
 * for main window handling and supports full session management as well as
 * using KActions.
 * @see KMainWindow
 * @see KApplication
 * @see KConfig
 *
 * @author Robby Stephenson
 * @version $Id: bookcase.h,v 1.4 2003/05/03 05:39:08 robby Exp $
 */
class Bookcase : public KMainWindow {
Q_OBJECT

public:
  /**
   * Constructor of Bookcase, calls all init functions to create the application.
   */
  Bookcase(QWidget* parent=0, const char* name=0);

  /**
   * Returns a pointer to the document object.
   *
   * @return The document pointer
   */
  BookcaseDoc* doc();
  /**
   * Returns a pointer to the selected item in the detailed list view.
   * Used for searching, to go sequentially through the collections.
   * If there is no selected item, then the first child is returned.
   *
   * @return The item pointer
   */
  BCUnitItem* selectedOrFirstItem();
  void readCollectionOptions(BCCollection* coll);

public slots:
  /**
   * Cleans up everything and then opens a new document.
   */
  void slotFileNew();
  /**
   * Opens a file and loads it into the document
   */
  void slotFileOpen();
  /**
   * Opens a file by URL and loads it into the document
   *
   * @param url The url to open
   */
  void slotFileOpen(const KURL& url);
  /**
   * Opens a file from the recent files menu
   *
   * @param url The url sent by the RecentFilesAction
   */
  void slotFileOpenRecent(const KURL& url);
  /**
   * Saves the document
   */
  void slotFileSave();
  /**
   * Saves a document by a new filename
   */
  void slotFileSaveAs();
  /**
   * Prints the current document. Not yet implemented!
   */
  void slotFilePrint();
  /**
   * Quits the application.
   */
  void slotFileQuit();
  /**
   * Puts the marked text/object into the clipboard and removes it from the document.
   * Not yet implemented!
   */
  void slotEditCut();
  /*
   * Puts the marked text/object into the clipboard.
   * Not yet implemented!
   */
  void slotEditCopy();
  /**
   * Pastes the clipboard into the document.
   * Not yet implemented!
   */
  void slotEditPaste();
  /**
   * Shows the find dialog to search in the current document.
   * Not yet implemented!
   */
  void slotEditFind();
  /**
   * Finds the next match.
   * Not yet implemented!
   */
  void slotEditFindNext();
  /**
   * Toggles the toolbar. Not needed for KDE 3.1 or greater.
   */
  void slotToggleToolBar();
  /**
   * Toggles the statusbars
   */
  void slotToggleStatusBar();
  /**
   * Shows the configuration dialog for the application.
   */
  void slotShowConfigDialog();
  /**
   * Hides the configuration dialog for the application.
   */
  void slotHideConfigDialog();
  /**
   * Shows the new collection dialog and then adds it to the document.
   * Not yet implemented!
   */
//  void slotFileNewCollection();
  /**
   * Changes the statusbar contents for the standard label permanently,
   * used to indicate current actions being made.
   *
   * @param text The text that is displayed in the statusbar
   */
  void slotStatusMsg(const QString& text);
  /**
   * Updates the unit count int he status bar.
   */
  void slotUnitCount();
  /**
   * Updates the progress bar in the status bar.
   *
   * @param f The fraction completed
   */
  void slotUpdateFractionDone(float f);
  /**
   * Handles updating everything when the configuration is changed
   * via the configuration dialog. This slot is called when the OK or Apply
   * button is clicked in the dialog
   */
  void slotHandleConfigChange();
  /**
   * Changes the grouping of the units in the @ref BCGroupView. The current value
   * of the combobox in the toolbar is used.
   */
  void slotChangeGrouping();
  /**
   * When a collection is added to the document, certain actions need to be taken
   * by the parent app. The colleection toolbar is updated, the unit count is set, and
   * the collection's modified signal is connected to the @ref BCGroupView widget.
   *
   * @param coll A pointer to the collection being added
   */
  void slotUpdateCollection(BCCollection* coll);
  /**
   * Toggles the collection toolbar. Not needed for KDE 3.1 or greater.
   */
  void slotToggleCollectionBar();
  /**
   * Imports a bibtex file.
   */
  void slotImportBibtex();
  /**
   * Imports a bibtexml file.
   */
  void slotImportBibtexml();
  /**
   * Exports the document into Bibtex format.
   */
  void slotExportBibtex();
  /**
   * Exports the document into Bibtexml format.
   */
  void slotExportBibtexml();
  /**
   * Exports using any XSLT file.
   */
  void slotExportXSLT();
  /**
   * Checks to see if the last file should be opened, or opens a command-line file
   */
  void slotInitFileOpen();
  /**
   * Shows the lookup dialog for the application.
   */
  void slotShowLookupDialog();
  /**
   * Hides the lookup dialog for the application.
   */
  void slotHideLookupDialog();
  /**
   * Shows the filter dialog for the application.
   */
  void slotShowFilterDialog();
  /**
   * Hides the filter dialog for the application.
   */
  void slotHideFilterDialog();
  /**
   * Shows the collection propreties dialog for the application.
   */
  void slotShowCollectionPropertiesDialog(int id=0);
  /**
   * Hides the collection properties dialog for the application.
   */
  void slotHideCollectionPropertiesDialog();

private:
  /**
   * Saves the general options like all toolbar positions and status as well as the
   * geometry and the recent file list to the configuration file.
   */
  void saveOptions();
  /**
   * Reads the general options and initialize variables like the recent file list.
   */
  void readOptions();
  /**
   * Initializes the KActions of the application
   */
  void initActions();
  /**
   * Sets up the statusbar for the main window by initializing a status label
   * and inserting a progress bar and unit counter.
   */
  void initStatusBar();
  /**
   * Initiates the view, setting up all the dock windows and so on.
   */
  void initView();
  /**
   * Initiates the window for such things as the application icon.
   */
  void initWindow();
  /**
   * Initiates the document.
   */
  void initDocument();
  /**
   * Initiates all the signal and slot connections between major objects in the view.
   */
  void initConnections();
  /**
   * Initiates shutdown
   */
//  void closeEvent(QCloseEvent *e);
  /**
   * Saves the window properties for each open window during session end to the
   * session config file, including saving the currently opened file by a temporary
   * filename provided by KApplication.
   * @see KMainWindow::saveProperties
   *
   * @param cfg The config class with the properties to restore
   */
  void saveProperties(KConfig* cfg);
  /**
   * Reads the session config file and restores the application's state including
   * the last opened files and documents by reading the temporary files saved by
   * @ref saveProperties().
   * @see KMainWindow::readProperties
   *
   * @param cfg The config class with the properties to restore
   */
  void readProperties(KConfig* cfg);
  /**
   * Called before the window is closed, either by the user or indirectely by the
   * session manager.
   *
   * The purpose of this function is to prepare the window in a way that it is safe to close it,
   * i.e. without the user losing some data.
   * @see KMainWindow::queryClose
   */
  bool queryClose();
  /**
   * Called before the very last window is closed, either by the user
   * or indirectly by the session manager.
   * @see KMainWindow::queryExit
   */
  bool queryExit();
  /**
   * Actual method used when opening a URL. Updating for the list views is turned off
   * as well as sorting, in order to more quickly load the document.
   *
   * @param url The url to open
   */
  bool openURL(const KURL& url);
  /*
   * Helper method to handle the printing duties.
   *
   * @param html The HTML string representing the doc to print
   */
  void doPrint(const QString& html);

  bool exportUsingXSLT(const QString& xsltFileName, const QString& filter, bool locale=false);
  
  void XSLTError();

  void FileError(const QString& filename);
  
protected slots:
  /**
   * Updates the actions when a file is opened.
   */
  void slotEnableOpenedActions(bool opened = true);
  /**
   * Updates the save action and the caption when the document is modified.
   */
  void slotEnableModifiedActions(bool modified = true);
  /**
   * Saves the options relevant for a collection. I was having problems with the collection
   * being destructed before I could save info.
   *
   * @param coll A pointer to the collection
   */
  void saveCollectionOptions(BCCollection* coll);
  void slotUpdateFilter(const QString& text);
  /**
   * Updates the collection toolbar.
   */
  void slotUpdateCollectionToolBar(BCCollection* coll=0);

private:
  BookcaseDoc* m_doc;

  KConfig* m_config;

  KAction* m_fileNew;
  KAction* m_fileOpen;
  KRecentFilesAction* m_fileOpenRecent;
  KAction* m_fileSave;
  KAction* m_fileSaveAs;
  KAction* m_filePrint;
  KAction* m_fileQuit;
  KAction* m_editCut;
  KAction* m_editCopy;
  KAction* m_editPaste;
  KAction* m_editFind;
  KAction* m_editFindNext;
  KAction* m_editFields;
  KToggleAction* m_toggleToolBar;
  KToggleAction* m_toggleStatusBar;
  KAction* m_preferences;

//  KAction* m_fileNewCollection;
  KAction* m_importBibtex;
  KAction* m_importBibtexml;
  KAction* m_exportBibtex;
  KAction* m_exportBibtexml;
  KAction* m_exportXSLT;
  KAction* m_lookup;
  KAction* m_filter;
  KSelectAction* m_unitGrouping;
  KToggleAction* m_toggleCollectionBar;
  BCLineEditAction* m_quickFilter;

  QSplitter* m_split;

  KProgress* m_progress;
  BCDetailedListView* m_detailedView;
  BCUnitEditWidget* m_editWidget;
  BCGroupView* m_groupView;
  ConfigDialog* m_configDlg;
  FindDialog* m_findDlg;
  LookupDialog* m_lookupDlg;
  BCFilterDialog* m_filterDlg;
  BCCollectionPropDialog* m_collPropDlg;
};
 
#endif // BOOKCASE_H
