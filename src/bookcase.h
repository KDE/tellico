/* *************************************************************************
                   bookcase.h  -  description
                     -------------------
        begin        : Wed Aug 29 21:00:54 CEST 2001
        copyright    : (C) 2001 by Robby Stephenson
        email        : robby@radiojodi.com
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
class BCListView;
class BCUnitEditWidget;
class BCCollectionView;
class BCUnit;
class QCloseEvent;

// include files for KDE
#include <kapp.h>
#include <kdockwidget.h>
#include <kaccel.h>
#include <kaction.h>

// include files for Qt
//#include <qmap.h>
//#include <qdict.h>

/**
  * The base class for Bookcase application windows. It sets up the main
  * window and reads the config file as well as providing a menubar, toolbar
  * and statusbar. Bookcase reimplements the methods that KDockMainWindow provides
  * for main window handling and supports full session management as well as using KActions.
  * @see KDockMainWindow
  * @see KApplication
  * @see KConfig
  *
  * @author Robby Stephenson
  * @version $Id: bookcase.h,v 1.9 2001/11/05 05:56:43 robby Exp $
  */
class Bookcase : public KDockMainWindow {
Q_OBJECT

public:
  /** constructor of Bookcase, calls all init functions to create the application.
   */
  Bookcase(QWidget* parent=0, const char* name=0);
  ~Bookcase();

public slots:
  /** clears the document in the actual view to reuse it as the new document */
  void slotFileNew();
  /** open a file and load it into the document*/
  void slotFileOpen();
  /** open a file by URL and load it into the document*/
  void slotFileOpen(const KURL& url);
  /** opens a file from the recent files menu */
  void slotFileOpenRecent(const KURL& url);
  /** save a document */
  void slotFileSave();
  /** save a document by a new filename*/
  void slotFileSaveAs();
  /** asks for saving if the file is modified, then closes the actual file and window*/
  void slotFileClose();
  /** print the actual file */
  void slotFilePrint();
  /** closes all open windows by calling close() on each memberList item until the list is empty, then quits the application.
   * If queryClose() returns false because the user canceled the saveModified() dialog, the closing breaks.
   */
  void slotFileQuit();
  /** put the marked text/object into the clipboard and remove
   *  it from the document
   */
  void slotEditCut();
  /** put the marked text/object into the clipboard
   */
  void slotEditCopy();
  /** paste the clipboard into the document
   */
  void slotEditPaste();
  /** popup a find dialog
   */
  void slotEditFind();
  /** find next match
   */
  void slotEditFindNext();
  /** toggles the toolbar
   */
  void slotToggleToolBar();
  /** toggles the toolbar
   */
//  void slotToggleBrowseToolBar();
  /** toggles the statusbar
   */
  void slotToggleStatusBar();
  /** edit the preferences for the application
   */
  void slotPreferences();
  /** changes the statusbar contents for the standard label permanently, used to indicate current actions.
   * @param text the text that is displayed in the statusbar
   */
  void slotStatusMsg(const QString& text);
//  void slotBrowseToolBarClicked(int id);
//  void slotBrowseToolBarToggled(int id);

  void slotDeleteUnit(BCUnit* unit);
  void slotFileNewCollection();

  /**
   * Returns a pointer to the document object.
   *
   * @return The document pointer
   */
  BookcaseDoc* doc();

protected:
  /** save general options like all bar positions and status as well as the geometry and the recent file list to the configuration
   * file
   */
  void saveOptions();
  /** read general options again and initialize all variables like the recent file list
   */
  void readOptions();
  /** initializes the KActions of the application */
  void initActions();
  /** sets up the statusbar for the main window by initialzing a statuslabel.
   */
  void initStatusBar();
  /** creates the centerwidget of the KTMainWindow instance and sets it as the view
   */
  void initView();
  void initWindow();
  void initDocument();
  /** creates browse tool bar
   */
//  void initBrowseToolBar();
  /** initiates shutdown
   */
  //void closeEvent(QCloseEvent *e);
  /** saves the window properties for each open window during session end to the session config file, including saving the currently
   * opened file by a temporary filename provided by KApplication.
   * @see KTMainWindow#saveProperties
   */
  virtual void saveProperties(KConfig* cfg);
  /** reads the session config file and restores the application's state including the last opened files and documents by reading the
   * temporary files saved by saveProperties()
   * @see KTMainWindow#readProperties
   */
  virtual void readProperties(KConfig* cfg);

  bool queryClose();
  bool queryExit();

protected:
  bool openURL(const KURL& url);

private:
  /** the configuration object of the application */
  KConfig* m_config;

  // KAction pointers to enable/disable actions
  KAction* m_fileNew;
  KAction* m_fileOpen;
  KRecentFilesAction* m_fileOpenRecent;
  KAction* m_fileSave;
  KAction* m_fileSaveAs;
  KAction* m_fileClose;
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

  BCListView* m_bclv;
  BCUnitEditWidget* m_ew;
  BCCollectionView* m_cv;
  BookcaseDoc* m_doc;
};
 
#endif // BOOKCASE_H
