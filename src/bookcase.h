/***************************************************************************
                         bookcase.h
                     -------------------
        begin        : Wed Aug 29 21:00:54 CEST 2001
        copyright    : (C) 2001, 2002, 2003 by Robby Stephenson
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
class ConfigDialog;
class BCCollection;
class FindDialog;
class BCUnitItem;
class LookupDialog;
class BCLineEditAction;
class BCFilterDialog;
class BCCollectionFieldsDialog;
class BookcaseController;

class KProgress;
class KToolBar;
class KURL;
class KAction;
class KSelectAction;
class KToggleAction;
class KRecentFilesAction;
class KActionMenu;

class QCloseEvent;
class QSplitter;

#include <kmainwindow.h>
#include <kdeversion.h>

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
 * @version $Id: bookcase.h 307 2003-11-26 01:45:44Z robby $
 */
class Bookcase : public KMainWindow {
Q_OBJECT

friend class BookcaseController;

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
  /**
   * @return Returns the name of the field being used to group the entries.
   */
  QStringList groupBy() const;
  /**
   * @return Returns a list of the names of the fields being used to sort the entries.
   */
  QStringList sortTitles() const;
  /**
   * @return Returns the name of the fields currently visible in the column view.
   */
  QStringList visibleColumns() const;
  /**
   * @return Returns whether the current collection is still the non-saved default one
   */
  bool isNewDocument() const { return m_newDocument; }

public slots:
  /**
   * Cleans up everything and then opens a new document.
   *
   * @param type Type of collection to add
   */
  void slotFileNew(int type);
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
   * Prints the current document.
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
   * Toggles the collection toolbar. Not needed for KDE 3.1 or greater.
   */
  void slotToggleCollectionBar();
  /**
   * Toggles the statusbar. Not needed for KDE 3.2 or greater.
   */
  void slotToggleStatusBar();
  /**
   * Toggles the group widget.
   */
  void slotToggleGroupWidget();
  /**
   * Toggles the edit widget.
   */
  void slotToggleEditWidget();
  /**
   * Shows the configuration dialog for the application.
   */
  void slotShowConfigDialog();
  /**
   * Hides the configuration dialog for the application.
   */
  void slotHideConfigDialog();
  /**
   * Changes the statusbar contents for the standard label permanently,
   * used to indicate current actions being made.
   *
   * @param text The text that is displayed in the statusbar
   */
  void slotStatusMsg(const QString& text);
  /**
   * Shows the configuration window for the toolbars.
   */
  void slotConfigToolbar();
  /**
   * Updates the toolbars;
   */
  void slotNewToolbarConfig();
  /**
   * Shows the configuration window for the key bindgins.
   */
  void slotConfigKeys();
  /**
   * Updates the unit count in the status bar.
   *
   * @param count The number of units currently selected
   */
  void slotUnitCount(int count);
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
   * Imports data.
   *
   * @param format The import format
   */
  void slotFileImport(int format);
  /**
   * Exports the current document.
   *
   * @param format The export format
   */
  void slotFileExport(int format);
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
   * Shows the collection properties dialog for the application.
   */
  void slotShowCollectionFieldsDialog();
  /**
   * Hides the collection properties dialog for the application.
   */
  void slotHideCollectionFieldsDialog();
  /**
   * Shows the "Tip of the Day" dialog.
   *
   * @param force Whether the configuration setting should be ignored
   */
  void slotShowTipOfDay(bool force=true);
  /**
   * Shows the string macro editor dialog.
   */
  void slotEditStringMacros();

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
  
  void XSLTError();

  void FileError(const QString& filename);
  
private slots:
  /**
   * Updates the actions when a file is opened.
   */
  void slotEnableOpenedActions(bool opened = true);
  /**
   * Updates the save action and the caption when the document is modified.
   */
  void slotEnableModifiedActions(bool modified = true);
  void readCollectionOptions(BCCollection* coll);
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

  KRecentFilesAction* m_fileOpenRecent;
  KAction* m_fileSave;
  KAction* m_fileSaveAs;
  KAction* m_filePrint;
  KActionMenu* m_fileImportMenu;
  KActionMenu* m_fileExportMenu;
  KAction* m_exportBibtex;
  KAction* m_exportBibtexml;
//  KAction* m_editCut;
//  KAction* m_editCopy;
//  KAction* m_editPaste;
  KAction* m_editFind;
  KAction* m_editFindNext;
  KAction* m_editConvertBibtex;
  KAction* m_editBibtexMacros;
  KToggleAction* m_toggleStatusBar;
#if KDE_VERSION < 306
  KToggleAction* m_toggleToolBar;
  KToggleAction* m_toggleCollectionBar;
#endif
  KToggleAction* m_toggleGroupWidget;
  KToggleAction* m_toggleEditWidget;

//  KAction* m_lookup;
  KSelectAction* m_unitGrouping;
  BCLineEditAction* m_quickFilter;

  QSplitter* m_split;

  KProgress* m_progress;
  BookcaseController* m_controller;
  BCDetailedListView* m_detailedView;
  BCUnitEditWidget* m_editWidget;
  BCGroupView* m_groupView;
  ConfigDialog* m_configDlg;
  FindDialog* m_findDlg;
  LookupDialog* m_lookupDlg;
  BCFilterDialog* m_filterDlg;
  BCCollectionFieldsDialog* m_collFieldsDlg;

  // the loading process goes through several steps, keep track of the factor
  unsigned m_currentStep;
  unsigned m_maxSteps;

  // need to keep track of whether the current collection is the initial default one
  bool m_newDocument;
};
 
#endif // BOOKCASE_H
