/* *************************************************************************
                         bookcase.h
                     -------------------
        begin        : Wed Aug 29 21:00:54 CEST 2001
        copyright    : (C) 2001 by Robby Stephenson
        email        : robby@periapsis.org
 * *************************************************************************/

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 * *************************************************************************/

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
//#include <qmap.h>
//#include <qdict.h>
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
 * @version $Id: bookcase.h,v 1.30 2002/11/25 00:56:22 robby Exp $
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
   * Returns a list of the column widths for collections using a certain unit type,
   * read from the config file.
   *
   * @param unitName The name of the unit
   * @return The ValueList of column widths
   */
  QValueList<int> readColumnWidths(const QString& unitName);

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
   * Toggles the toolbar.
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
   * Hide the configuration dialog for the application.
   */
  void slotHideConfigDialog();
  /**
   * Shows the new collection dialog and then adds it to the document.
   * Not yet implemented!
   */
  void slotFileNewCollection();
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
   * Handles deleting a unit from a collection
   *
   * @param unit The unit to be deleted
   */
  void slotDeleteUnit(BCUnit* unit);
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
   * @param coll A pointer the collection being added
   */
  void slotUpdateCollection(BCCollection* coll);
  /**
   * Toggles the collection toolbar.
   */
  void slotToggleCollectionBar();

protected:
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
  /**
   * Initializes the collection toolbar.
   */
  void updateCollectionToolBar();
  /*
   * Helper method to handle the printing duties.
   *
   * @param html The HTML string representing the doc to print
   */
  void doPrint(const QString& html);
  
protected slots:
  /**
   * Updates the save action and the caption when the document is modified.
   */
  void slotEnableModifiedActions();

private:
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
  KToggleAction* m_toggleToolBar;
  KToggleAction* m_toggleStatusBar;
  KAction* m_preferences;

  KAction* m_fileNewCollection;
  KSelectAction* m_unitGrouping;
  KToggleAction* m_toggleCollectionBar;

  QSplitter* m_split;

  KProgress* m_progress;
  BCDetailedListView* m_detailedView;
  BCUnitEditWidget* m_editWidget;
  BCGroupView* m_groupView;
  BookcaseDoc* m_doc;
  ConfigDialog* m_configDlg;
};
 
#endif // BOOKCASE_H
