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

#ifndef TELLICO_MAINWINDOW_H
#define TELLICO_MAINWINDOW_H

class KToolBar;
class KURL;
class KAction;
class KSelectAction;
class KToggleAction;
class KRecentFilesAction;
class KActionMenu;
class KDialogBase;

class QCloseEvent;
class QSplitter;
class QListViewItem;

#include <config.h>

#include "core/dcopinterface.h"
#include "translators/translators.h"
#include "datavectors.h"

#include <kmainwindow.h>

namespace Tellico {
// forward declarations
  namespace GUI {
    class LineEdit;
    class TabControl;
  }
  class Controller;
  class ViewStack;
  class DetailedListView;
  class FilterDialog;
  class EntryEditDialog;
  class GroupView;
  class FilterView;
  class LoanView;
  class ConfigDialog;
  class CollectionFieldsDialog;
  class StringMapDialog;
  class EntryItem;
  class FetchDialog;
  class ReportDialog;
  class StatusBar;

/**
 * The base class for Tellico application windows. It sets up the main
 * window and reads the config file as well as providing a menubar, toolbar
 * and statusbar. Tellico reimplements the methods that KMainWindow provides
 * for main window handling and supports full session management as well as
 * using KActions.
 * @see KMainWindow
 * @see KApplication
 * @see KConfig
 *
 * @author Robby Stephenson
 */
class MainWindow : public KMainWindow, public DCOPInterface {
Q_OBJECT

friend class Controller;

public:
  /**
   * The main window constructor, calls all init functions to create the application.
   */
  MainWindow(QWidget* parent=0, const char* name=0);

  /**
   * Opens the initial file.
   *
   * @param nofile If true, even if the config option to reopen last file is set, no file is opened
   */
  void initFileOpen(bool nofile);
  /**
   * Saves the document
   *
   * @return Returns @p true if successful
   */
  bool fileSave();
  /**
   * Saves a document by a new filename
   *
   * @return Returns @p true if successful
   */
  bool fileSaveAs();
  /**
   * @return Returns whether the current collection is still the non-saved default one
   */
  bool isNewDocument() const { return m_newDocument; }
  /**
   * Used by main() and DCOP to import file.
   *
   * @param format The file format
   * @param url The url
   */
  virtual bool importFile(Import::Format format, const KURL& url, Import::Action action);
  /**
   * Used by DCOP to export to a file.
   */
  virtual bool exportCollection(Export::Format format, const KURL& url);
  /**
   * DCOP call to return bibtex keys. Returnd empty list if the current
   * collection is not a bibliography.
   */
  virtual QStringList bibtexKeys() const;

public slots:
  /**
   * Initializes some stuff after the object is created.
   */
  void slotInit();
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
   */
  void slotEditCut();
  /*
   * Puts the marked text/object into the clipboard.
   */
  void slotEditCopy();
  /**
   * Pastes the clipboard into the document.
   */
  void slotEditPaste();
  /**
   * Selects all the entries in the collection.
   */
  void slotEditSelectAll();
  /**
   * Deselects all the entries in the collection.
   */
  void slotEditDeselect();
  /**
   * Toggles the group widget.
   */
  void slotToggleGroupWidget();
  /**
   * Toggles the edit widget.
   */
  void slotToggleEntryEditor();
  /**
   * Toggles the edit widget.
   */
  void slotToggleEntryView();
  /**
   * Shows the configuration dialog for the application.
   */
  void slotShowConfigDialog();
  /**
   * Hides the configuration dialog for the application.
   */
  void slotHideConfigDialog();
  /**
   * Shows the fetch dialog.
   */
  void slotShowFetchDialog();
  /**
   * Hides the fetch dialog.
   */
  void slotHideFetchDialog();
  /**
   * Changes the statusbar contents for the standard label permanently,
   * used to indicate current actions being made.
   *
   * @param text The text that is displayed in the statusbar
   */
  void slotStatusMsg(const QString& text);
  void slotClearStatus();
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
   * Updates the entry count in the status bar.
   */
  void slotEntryCount();
  /**
   * Handles updating everything when the configuration is changed
   * via the configuration dialog. This slot is called when the OK or Apply
   * button is clicked in the dialog
   */
  void slotHandleConfigChange();
  /**
   * Changes the grouping of the entries in the @ref GroupView. The current value
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
   * Shows the string macro editor dialog for the application.
   */
  void slotShowStringMacroDialog();
  /**
   * Hides the string macro editor dialog for the application.
   */
  void slotHideStringMacroDialog();

private:
  /**
   * Saves the general options like all toolbar positions and status as well as the
   * geometry and the recent file list to the configuration file.
   */
  void saveOptions();
  /**
   * Reads the specific options.
   */
  void readOptions();
  /**
   * Initializes the KActions of the application
   */
  void initActions();
  /**
   * Sets up the statusbar for the main window by initializing a status label
   * and inserting a progress bar and entry counter.
   */
  void initStatusBar();
  /**
   * Initiates the view, setting up all the dock windows and so on.
   */
  void initView();
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
  /**
   * Helper function to activate a slot in the edit widget.
   * Primarily used for copy, cut, and paste.
   *
   * @param slot The slot name
   */
  void activateEditSlot(const char* slot);
  void addFilterView();
  void addLoanView();
  void updateCaption(bool modified);
  void updateCollectionActions();
  void updateEntrySources();

private slots:
  /**
   * Updates the actions when a file is opened.
   */
  void slotEnableOpenedActions();
  /**
   * Updates the save action and the caption when the document is modified.
   */
  void slotEnableModifiedActions(bool modified = true);
  /**
   * Read the options specific to a collection
   *
   * @param coll The collection pointer
   */
  void readCollectionOptions(Tellico::Data::CollPtr coll);
  /**
   * Saves the options relevant for a collection. I was having problems with the collection
   * being destructed before I could save info.
   *
   * @param coll A pointer to the collection
   */
  void saveCollectionOptions(Tellico::Data::CollPtr coll);
  /**
   * Queue a filter update. The timer adds a 200 millisecond delay before actually
   * updating the filter.
   */
  void slotQueueFilter();
  /**
   * Update the filter to match any field with text. If a non-word character appears, the
   * text is interpreted as a regexp.
   */
  void slotUpdateFilter();
  /**
   * Updates the collection toolbar.
   */
  void slotUpdateCollectionToolBar(Tellico::Data::CollPtr coll);
  /**
   * Make sure the edit dialog is visible and start a new entry.
   */
  void slotNewEntry();
  /**
   * Handle the entry editor dialog being closed.
   */
  void slotEditDialogFinished();
  /**
   * Handle the Ok button being clicked in the string macros dialog.
   */
  void slotStringMacroDialogOk();
  /**
   * Since I use an application icon in the toolbar, I need to change its size whenever
   * the toolbar changes mode
   */
  void slotUpdateToolbarIcons();
  /**
   * Convert current collection to a bibliography.
   */
  void slotConvertToBibliography();
  /**
   * Send a citation for the selected entries
   */
  void slotCiteEntry(int action);
  /**
   * Show the entry editor and update menu item.
   */
  void slotShowEntryEditor();
  /**
   * Show the report window.
   */
  void slotShowReportDialog();
  void slotGroupLabelActivated();
  /**
   * Show the report window.
   */
  void slotHideReportDialog();
  /**
   * Focus the filter
   */
  void slotFilterLabelActivated();
  void slotClearFilter();
  void slotRenameCollection();

private:
  KConfig* m_config;

  KRecentFilesAction* m_fileOpenRecent;
  KAction* m_fileSave;
  KAction* m_newEntry;
  KAction* m_editEntry;
  KAction* m_copyEntry;
  KAction* m_deleteEntry;
  KActionMenu* m_updateEntryMenu;
  KAction* m_updateAll;
  KAction* m_checkInEntry;
  KAction* m_checkOutEntry;
  KToggleAction* m_toggleGroupWidget;
  KToggleAction* m_toggleEntryEditor;
  KToggleAction* m_toggleEntryView;

  KSelectAction* m_entryGrouping;
  GUI::LineEdit* m_quickFilter;

  // m_split is used between the stuff on the left and stuff on the right
  QSplitter* m_split;
  // m_leftSplit is used between detailed view and entry view
  QSplitter* m_rightSplit;

  Tellico::StatusBar* m_statusBar;

  DetailedListView* m_detailedView;
  EntryEditDialog* m_editDialog;
  GUI::TabControl* m_viewTabs;
  GroupView* m_groupView;
  FilterView* m_filterView;
  LoanView* m_loanView;
  ViewStack* m_viewStack;

  ConfigDialog* m_configDlg;
  FilterDialog* m_filterDlg;
  CollectionFieldsDialog* m_collFieldsDlg;
  StringMapDialog* m_stringMacroDlg;
  FetchDialog* m_fetchDlg;
  ReportDialog* m_reportDlg;

  QPtrList<KAction> m_fetchActions;

  // keep track of the number of queued filter updates
  uint m_queuedFilters;

  // keep track whether everything gets initialized
  bool m_initialized : 1;
  // need to keep track of whether the current collection has never been saved
  bool m_newDocument : 1;
};

} // end namespace
#endif // TELLICO_MAINWINDOW_H
