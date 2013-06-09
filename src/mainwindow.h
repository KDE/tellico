/***************************************************************************
    Copyright (C) 2001-2009 Robby Stephenson <robby@periapsis.org>
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
 *   defined in Section 14 of version 3 of the license.                    *
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

#ifndef TELLICO_MAINWINDOW_H
#define TELLICO_MAINWINDOW_H

#include <config.h>

#include "translators/translators.h"
#include "datavectors.h"

#include <kxmlguiwindow.h>
#include <kurl.h>

#include <QList>

class KToolBar;
class KAction;
class KSelectAction;
class KToggleAction;
class KRecentFilesAction;
class KActionMenu;
class KDialogBase;
class KTabWidget;

class QCloseEvent;
class QSplitter;
class QSignalMapper;

namespace Tellico {
// forward declarations
  namespace GUI {
    class LineEdit;
  }
  class Controller;
  class ViewStack;
  class DetailedListView;
  class EntryIconView;
  class EntryView;
  class FilterDialog;
  class EntryEditDialog;
  class GroupView;
  class FilterView;
  class LoanView;
  class ConfigDialog;
  class CollectionFieldsDialog;
  class StringMapDialog;
  class BibtexKeyDialog;
  class EntryItem;
  class FetchDialog;
  class ReportDialog;
  class StatusBar;
  class DropHandler;

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
class MainWindow : public KXmlGuiWindow {
Q_OBJECT

friend class Controller;
friend class DropHandler;

public:
  /**
   * The main window constructor, calls all init functions to create the application.
   */
  MainWindow(QWidget* parent=0);
  ~MainWindow();

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
   * Used by main() and DBUS to import file.
   *
   * @param format The file format
   * @param url The url
   */
  virtual bool importFile(Import::Format format, const KUrl& url, Import::Action action);
  /**
   * Used by DCOP to export to a file.
   */
  virtual bool exportCollection(Export::Format format, const KUrl& url);
  /**
   * Used by DCOP
   */
  virtual void openFile(const QString& file);
  virtual void setFilter(const QString& text);
  virtual bool showEntry(Data::ID id);

  bool eventFilter(QObject* watched, QEvent* event);

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
  void slotFileOpen(const KUrl& url);
  /**
   * Opens a file from the recent files menu
   *
   * @param url The url sent by the RecentFilesAction
   */
  void slotFileOpenRecent(const KUrl& url);
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
  /**
   * Shows the citation key dialog
   */
  void slotShowBibtexKeyDialog();
  /**
   * Hides the citation key dialog
   */
  void slotHideBibtexKeyDialog();
  /**
   * Handle a url that indicates some actino should be taken
   */
  void slotURLAction(const KUrl& url);

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

  bool querySaveModified();

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
  bool openURL(const KUrl& url);
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
  void slotUpdateFilter(Tellico::FilterPtr filter);
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
  /**
   * Show the report window.
   */
  void slotHideReportDialog();
  /**
   * Focus the filter
   */
  void slotGroupLabelActivated();
  void slotFilterLabelActivated();
  void slotClearFilter();
  void slotRenameCollection();
  void slotImageLocationChanged();
  /**
   * Toggle full screen mode
   */
  void slotToggleFullScreen();
  /**
   * Toggle menubar visibility
   */
  void slotToggleMenuBarVisibility();

private:
  void importFile(Import::Format format, const KUrl::List& kurls);
  void importText(Import::Format format, const QString& text);
  bool importCollection(Data::CollPtr coll, Import::Action action);

  // the reason that I have to keep pointers to all these
  // is because they get plugged into menus later in Controller
  KRecentFilesAction* m_fileOpenRecent;
  KAction* m_fileSave;
  KAction* m_newEntry;
  KAction* m_editEntry;
  KAction* m_copyEntry;
  KAction* m_deleteEntry;
  KAction* m_mergeEntry;
  KActionMenu* m_updateEntryMenu;
  KAction* m_updateAll;
  KAction* m_checkInEntry;
  KAction* m_checkOutEntry;
  KToggleAction* m_toggleGroupWidget;
  KToggleAction* m_toggleEntryEditor;

  KSelectAction* m_entryGrouping;
  GUI::LineEdit* m_quickFilter;

  // m_split is used between the stuff on the left and stuff on the right
  QSplitter* m_split;
  // m_leftSplit is used between detailed view and entry view
  QSplitter* m_rightSplit;

  Tellico::StatusBar* m_statusBar;

  DetailedListView* m_detailedView;
  EntryEditDialog* m_editDialog;
  KTabWidget* m_viewTabs;
  GroupView* m_groupView;
  FilterView* m_filterView;
  LoanView* m_loanView;
  EntryView* m_entryView;
  EntryIconView* m_iconView;
  ViewStack* m_viewStack;
  QSignalMapper* m_updateMapper;

  ConfigDialog* m_configDlg;
  FilterDialog* m_filterDlg;
  CollectionFieldsDialog* m_collFieldsDlg;
  StringMapDialog* m_stringMacroDlg;
  BibtexKeyDialog* m_bibtexKeyDlg;
  FetchDialog* m_fetchDlg;
  ReportDialog* m_reportDlg;

  QList<QAction*> m_fetchActions;

  // keep track of the number of queued filter updates
  uint m_queuedFilters;

  // keep track whether everything gets initialized
  bool m_initialized;
  // need to keep track of whether the current collection has never been saved
  bool m_newDocument;
  // Don't queue filter if true
  bool m_dontQueueFilter;
  bool m_savingImageLocationChange;
};

} // end namespace
#endif // TELLICO_MAINWINDOW_H
