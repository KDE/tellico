/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "controller.h"
#include "mainwindow.h"
#include "groupview.h"
#include "detailedlistview.h"
#include "entryeditdialog.h"
#include "viewstack.h"
#include "entryview.h"
#include "entryiconview.h"
#include "entry.h"
#include "field.h"
#include "filter.h"
#include "filterdialog.h"
#include "tellico_kernel.h"
#include "collection.h"
#include "document.h"
#include "borrower.h"
#include "filterview.h"
#include "loanview.h"
#include "utils/calendarhandler.h"
#include "tellico_debug.h"
#include "groupiterator.h"
#include "entryupdater.h"
#include "entrymerger.h"
#include "gui/treeview.h"
#include "gui/cursorsaver.h"
#include "gui/lineedit.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kaction.h>
#include <kactionmenu.h>
#include <kmenu.h>
#include <ktabwidget.h>

using Tellico::Controller;

Controller* Controller::s_self = 0;

Controller::Controller(Tellico::MainWindow* parent_)
    : QObject(parent_), m_mainWindow(parent_), m_working(false), m_widgetWithSelection(0) {
}

void Controller::addObserver(Tellico::Observer* obs) {
  m_observers.append(obs);
}

void Controller::removeObserver(Tellico::Observer* obs) {
  m_observers.removeAll(obs);
}

Tellico::GroupIterator Controller::groupIterator() const {
  return GroupIterator(m_mainWindow->m_groupView->model());
}

QString Controller::groupBy() const {
  return m_mainWindow->m_groupView->groupBy();
}

QStringList Controller::expandedGroupBy() const {
  QStringList g;
  g << groupBy();
  // special case for pseudo-group
  if(g[0] == Data::Collection::s_peopleGroupName) {
    g.clear();
    Data::FieldList fields = Data::Document::self()->collection()->peopleFields();
    foreach(Data::FieldPtr it, fields) {
      g << it->name();
    }
  }
  // special case for no groups
  if(g[0].isEmpty()) {
    g.clear();
  }
  return g;
}

QStringList Controller::sortTitles() const {
  QStringList list;
  list << m_mainWindow->m_detailedView->sortColumnTitle1();
  list << m_mainWindow->m_detailedView->sortColumnTitle2();
  list << m_mainWindow->m_detailedView->sortColumnTitle3();
  return list;
}

QStringList Controller::visibleColumns() const {
  return m_mainWindow->m_detailedView->visibleColumns();
}

Tellico::Data::EntryList Controller::visibleEntries() {
  return m_mainWindow->m_detailedView->visibleEntries();
}

void Controller::slotCollectionAdded(Tellico::Data::CollPtr coll_) {
  // at start-up, this might get called too early, so check and bail
  if(!m_mainWindow->m_groupView) {
    return;
  }

  // do this first because the group view will need it later
  m_mainWindow->readCollectionOptions(coll_);
  m_mainWindow->slotUpdateToolbarIcons();
  m_mainWindow->updateEntrySources(); // has to be called before all the addCollection()
  // calls in the widgets since they may want menu updates

//  blockAllSignals(true);
  m_mainWindow->m_detailedView->addCollection(coll_);
  m_mainWindow->m_groupView->addCollection(coll_);
  m_mainWindow->m_editDialog->setLayout(coll_);
  if(!coll_->filters().isEmpty()) {
    m_mainWindow->addFilterView();
    m_mainWindow->m_filterView->addCollection(coll_);
    m_mainWindow->m_viewTabs->setTabBarHidden(false);
  }
  if(!coll_->borrowers().isEmpty()) {
    m_mainWindow->addLoanView();
    m_mainWindow->m_loanView->addCollection(coll_);
    m_mainWindow->m_viewTabs->setTabBarHidden(false);
  }
//  blockAllSignals(false);

  m_mainWindow->slotStatusMsg(i18n("Ready."));

  m_selectedEntries.clear();
  m_mainWindow->slotEntryCount();

  // there really should be a lot of signals to connect to, but right now, the only one
  // is used when a field is added on a merge
  connect(coll_.data(), SIGNAL(mergeAddedField(Tellico::Data::CollPtr, Tellico::Data::FieldPtr)),
          this, SLOT(slotFieldAdded(Tellico::Data::CollPtr, Tellico::Data::FieldPtr)));

  emit collectionAdded(coll_->type());

  updateActions();

  connect(&*coll_, SIGNAL(signalGroupsModified(Tellico::Data::CollPtr, QList<Tellico::Data::EntryGroup*>)),
          m_mainWindow->m_groupView, SLOT(slotModifyGroups(Tellico::Data::CollPtr, QList<Tellico::Data::EntryGroup*>)));

  connect(&*coll_, SIGNAL(signalRefreshField(Tellico::Data::FieldPtr)),
          this, SLOT(slotRefreshField(Tellico::Data::FieldPtr)));
}

void Controller::slotCollectionModified(Tellico::Data::CollPtr coll_) {
  // easiest thing is to signal collection deleted, then added?
  // FIXME: Signals for delete collection and then added are yucky
  slotCollectionDeleted(coll_);
  slotCollectionAdded(coll_);
}

void Controller::slotCollectionDeleted(Tellico::Data::CollPtr coll_) {
  blockAllSignals(true);
  m_mainWindow->saveCollectionOptions(coll_);
  m_mainWindow->m_groupView->removeCollection(coll_);
  if(m_mainWindow->m_filterView) {
    m_mainWindow->m_filterView->slotReset();
  }
  if(m_mainWindow->m_loanView) {
    m_mainWindow->m_loanView->slotReset();
  }
  m_mainWindow->m_detailedView->removeCollection(coll_);
  m_mainWindow->m_viewStack->clear();
  blockAllSignals(false);

  // disconnect all signals from the collection
  // this is needed because the Collection::appendCollection() and mergeCollection()
  // functions signal collection deleted then added for the same collection
  coll_->disconnect();
}

void Controller::slotFieldAdded(Tellico::Data::CollPtr coll_, Tellico::Data::FieldPtr field_) {
  addedField(coll_, field_);
}

void Controller::addedEntries(Tellico::Data::EntryList entries_) {
  blockAllSignals(true);
  foreach(Observer* it, m_observers) {
    it->addEntries(entries_);
  }
  m_mainWindow->slotQueueFilter();
  blockAllSignals(false);
}

void Controller::modifiedEntries(Tellico::Data::EntryList entries_) {
  // when a new document is being loaded, loans are added to borrowers, which
  // end up calling Entry::checkIn() which called Document::saveEntry() which calls here
  // ignore that
  if(!m_mainWindow->m_initialized) {
    return;
  }
  blockAllSignals(true);
  foreach(Observer* it, m_observers) {
    it->modifyEntries(entries_);
  }
  m_mainWindow->m_viewStack->entryView()->slotRefresh(); // special case
  m_mainWindow->slotQueueFilter();
  blockAllSignals(false);
}

void Controller::removedEntries(Tellico::Data::EntryList entries_) {
  blockAllSignals(true);
  foreach(Observer* obs, m_observers) {
    obs->removeEntries(entries_);
  }
  foreach(Data::EntryPtr entry, entries_) {
    m_selectedEntries.removeAll(entry);
    m_currentEntries.removeAll(entry);
  }
  if(m_currentEntries.isEmpty()) {
    m_mainWindow->m_viewStack->entryView()->clear();
    m_mainWindow->m_editDialog->clear();
  }
  m_mainWindow->slotEntryCount();
  m_mainWindow->slotQueueFilter();
  blockAllSignals(false);
}

void Controller::addedField(Tellico::Data::CollPtr coll_, Tellico::Data::FieldPtr field_) {
  foreach(Observer* obs, m_observers) {
    obs->addField(coll_, field_);
  }
  m_mainWindow->m_viewStack->refresh();
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
  m_mainWindow->slotQueueFilter();
}

void Controller::removedField(Tellico::Data::CollPtr coll_, Tellico::Data::FieldPtr field_) {
  foreach(Observer* obs, m_observers) {
    obs->removeField(coll_, field_);
  }
  m_mainWindow->m_viewStack->refresh();
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
  m_mainWindow->slotQueueFilter();
}

void Controller::modifiedField(Tellico::Data::CollPtr coll_, Tellico::Data::FieldPtr oldField_, Tellico::Data::FieldPtr newField_) {
  foreach(Observer* obs, m_observers) {
    obs->modifyField(coll_, oldField_, newField_);
  }
  m_mainWindow->m_viewStack->refresh();
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
  m_mainWindow->slotQueueFilter();
}

void Controller::reorderedFields(Tellico::Data::CollPtr coll_) {
  m_mainWindow->m_editDialog->setLayout(coll_);
  m_mainWindow->m_detailedView->reorderFields(coll_->fields());
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
  m_mainWindow->m_viewStack->refresh();
}

void Controller::slotClearSelection() {
  if(m_working) {
    return;
  }

  m_working = true;
  blockAllSignals(true);

  m_mainWindow->m_detailedView->clearSelection();
  m_mainWindow->m_groupView->clearSelection();
  if(m_mainWindow->m_loanView) {
    m_mainWindow->m_loanView->clearSelection();
  }
//  m_mainWindow->m_editDialog->clear(); let this stay
//  m_mainWindow->m_viewStack->clear(); let this stay

  blockAllSignals(false);

  m_selectedEntries.clear();
  updateActions();
  m_mainWindow->slotEntryCount();
  m_working = false;
}

void Controller::slotUpdateSelection(QWidget* widget_, const Tellico::Data::EntryList& entries_) {
  if(m_working) {
    return;
  }
  m_working = true;

  if(widget_) {
    m_widgetWithSelection = widget_;
  }

  blockAllSignals(true);
// in the list view and group view, if entries are selected in one, clear selection in other
  if(m_widgetWithSelection != m_mainWindow->m_detailedView) {
    m_mainWindow->m_detailedView->clearSelection();
  }
  if(m_widgetWithSelection != m_mainWindow->m_groupView) {
    m_mainWindow->m_groupView->clearSelection();
  }
  if(m_mainWindow->m_filterView && m_widgetWithSelection != m_mainWindow->m_filterView) {
    m_mainWindow->m_filterView->clearSelection();
  }
  if(m_mainWindow->m_loanView && m_widgetWithSelection != m_mainWindow->m_loanView) {
    m_mainWindow->m_loanView->clearSelection();
  }
  if(m_widgetWithSelection != m_mainWindow->m_editDialog) {
    m_mainWindow->m_editDialog->setContents(entries_);
  }
  // only show first one
  if(m_widgetWithSelection && m_widgetWithSelection != m_mainWindow->m_viewStack->iconView()) {
    if(entries_.count() > 1) {
      m_mainWindow->m_viewStack->showEntries(entries_);
    } else if(entries_.count() > 0) {
      m_mainWindow->m_viewStack->showEntry(entries_[0]);
    }
  }
  blockAllSignals(false);

  m_selectedEntries = entries_;
  updateActions();
  m_mainWindow->slotEntryCount();
  m_working = false;
}

void Controller::slotGoPrevEntry() {
  goEntrySibling(PrevEntry);
}

void Controller::slotGoNextEntry() {
  goEntrySibling(NextEntry);
}

void Controller::goEntrySibling(EntryDirection dir_) {
  Q_UNUSED(dir_)
  // if there are currently multiple selected, then do nothing
  if(m_selectedEntries.count() != 1) {
    return;
  }
  // find the widget that has an entry selected
  GUI::TreeView* view = ::qobject_cast<GUI::TreeView*>(m_widgetWithSelection);
  if(!view) {
    return;
  }

/*
GUI::ListViewItemList items = view->selectedItems();
  if(items.count() != 1) {
    return;
  }
  GUI::ListViewItem* item = items.first();
  if(item->isEntryItem()) {
    bool looped = false;
    // check sanity
    if(m_selectedEntries.front() != static_cast<EntryItem*>(item)->entry()) {
      myDebug() << "entries don't match!";
    }
    GUI::ListViewItem* nextItem = static_cast<GUI::ListViewItem*>(dir_ == PrevEntry
                                                                         ? item->itemAbove()
                                                                         : item->itemBelow());
    if(!nextItem) {
      // cycle through
      nextItem = static_cast<GUI::ListViewItem*>(dir_ == PrevEntry
                                                       ? view->lastItem()
                                                       : view->firstChild());
      looped = true;
    }
    while(!nextItem->isVisible()) {
      nextItem = static_cast<GUI::ListViewItem*>(dir_ == PrevEntry
                                                       ? nextItem->itemAbove()
                                                       : nextItem->itemBelow());
    }
    while(nextItem && !nextItem->isEntryItem()) {
      nextItem->setOpen(true); // have to be open to find the next one
      nextItem = static_cast<GUI::ListViewItem*>(dir_ == PrevEntry
                                                       ? nextItem->itemAbove()
                                                       : nextItem->itemBelow());
      if(!nextItem && !looped) {
        // cycle through
        nextItem = static_cast<GUI::ListViewItem*>(dir_ == PrevEntry
                                                         ? view->lastItem()
                                                         : view->firstChild());
        looped = true;
      }
    }
    if(nextItem) {
      Data::EntryPtr e = static_cast<EntryItem*>(nextItem)->entry();
      view->blockSignals(true);
      view->setSelected(item, false);
      view->setSelected(nextItem, true);
      view->ensureItemVisible(nextItem);
      view->blockSignals(false);
      slotUpdateSelection(view, Data::EntryList() << e);
    }
  }
  */
}

void Controller::slotUpdateCurrent(const Tellico::Data::EntryList& entries_) {
  if(m_working) {
    return;
  }
  m_working = true;

  blockAllSignals(true);
  m_mainWindow->m_viewStack->showEntries(entries_);
  blockAllSignals(false);

  m_currentEntries = entries_;
  m_working = false;
}

void Controller::slotUpdateSelectedEntries(const QString& source_) {
  if(m_selectedEntries.isEmpty()) {
    return;
  }

  // it deletes itself when done
  // signal mapper strings can't be empty, "_all" is set in mainwindow
  if(source_.isEmpty() || source_ == QLatin1String("_all")) {
    new EntryUpdater(m_selectedEntries.front()->collection(), m_selectedEntries, this);
  } else {
    new EntryUpdater(source_, m_selectedEntries.front()->collection(), m_selectedEntries, this);
  }
}

void Controller::slotDeleteSelectedEntries() {
  if(m_selectedEntries.isEmpty()) {
    return;
  }

  m_working = true;

  // confirm delete
  if(m_selectedEntries.count() == 1) {
    QString str = i18n("Do you really want to delete this entry?");
    QString dontAsk = QLatin1String("DeleteEntry");
    int ret = KMessageBox::warningContinueCancel(Kernel::self()->widget(), str, i18n("Delete Entry"),
                                                 KGuiItem(i18n("&Delete"), QLatin1String("edit-delete")),
                                                 KStandardGuiItem::cancel(), dontAsk);
    if(ret != KMessageBox::Continue) {
      m_working = false;
      return;
    }
  } else {
    QStringList names;
    foreach(Data::EntryPtr entry, m_selectedEntries) {
      names += entry->title();
    }
    QString str = i18n("Do you really want to delete these entries?");
    // historically called DeleteMultipleBooks, don't change
    QString dontAsk = QLatin1String("DeleteMultipleBooks");
    int ret = KMessageBox::warningContinueCancelList(Kernel::self()->widget(), str, names,
                                                     i18n("Delete Multiple Entries"),
                                                     KGuiItem(i18n("&Delete"), QLatin1String("edit-delete")),
                                                     KStandardGuiItem::cancel(), dontAsk);
    if(ret != KMessageBox::Continue) {
      m_working = false;
      return;
    }
  }

  GUI::CursorSaver cs;
  Kernel::self()->removeEntries(m_selectedEntries);
  updateActions();

  m_working = false;

  // special case, the detailed list view selects the next item, so handle that
//  Data::EntryList newList;
//  for(GUI::ListViewItemListIt it(m_mainWindow->m_detailedView->selectedItems()); it.current(); ++it) {
//    newList.append(static_cast<EntryItem*>(it.current())->entry());
//  }
//  slotUpdateSelection(m_mainWindow->m_detailedView, newList);
  slotClearSelection();
}

void Controller::slotMergeSelectedEntries() {
  // merge requires at least 2 entries
  if(m_selectedEntries.count() < 2) {
    return;
  }

  new EntryMerger(m_selectedEntries, this);
}

void Controller::slotRefreshField(Tellico::Data::FieldPtr field_) {
//  DEBUG_LINE;
  // group view only needs to refresh if it's the title
  if(field_->name() == QLatin1String("title")) {
    m_mainWindow->m_groupView->populateCollection();
  }
  m_mainWindow->m_detailedView->slotRefresh();
  m_mainWindow->m_viewStack->refresh();
}

void Controller::slotCopySelectedEntries() {
  if(m_selectedEntries.isEmpty()) {
    return;
  }

  // keep copy of selected entries
  Data::EntryList old = m_selectedEntries;

  GUI::CursorSaver cs;
  // need to create copies
  Data::EntryList entries;
  foreach(Data::EntryPtr it, m_selectedEntries) {
    entries.append(Data::EntryPtr(new Data::Entry(*it)));
  }
  Kernel::self()->addEntries(entries, false);
  slotUpdateSelection(0, old);
}

void Controller::blockAllSignals(bool block_) const {
// sanity check
  if(!m_mainWindow->m_initialized) {
    return;
  }
  m_mainWindow->m_detailedView->blockSignals(block_);
  m_mainWindow->m_groupView->blockSignals(block_);
  m_mainWindow->m_quickFilter->blockSignals(block_);
  if(m_mainWindow->m_loanView) {
    m_mainWindow->m_loanView->blockSignals(block_);
  }
  if(m_mainWindow->m_filterView) {
    m_mainWindow->m_filterView->blockSignals(block_);
  }
  m_mainWindow->m_editDialog->blockSignals(block_);
  m_mainWindow->m_viewStack->iconView()->blockSignals(block_);
}

void Controller::slotUpdateFilter(Tellico::FilterPtr filter_) {
  blockAllSignals(true);

  // the view takes over ownership of the filter
  if(filter_ && !filter_->isEmpty()) {
    // clear the icon view selection only
    // the detailed view takes care of itself
    m_mainWindow->m_viewStack->iconView()->clearSelection();
    m_selectedEntries.clear();
  }
  updateActions();

  m_mainWindow->m_detailedView->setFilter(filter_); // takes ownership

  blockAllSignals(false);

  m_mainWindow->slotEntryCount();
}

void Controller::clearFilter() {
  blockAllSignals(true);
  m_mainWindow->m_quickFilter->clear();
  m_mainWindow->m_detailedView->setFilter(Tellico::FilterPtr());
  blockAllSignals(false);
}

void Controller::editEntry(Tellico::Data::EntryPtr) const {
  m_mainWindow->slotShowEntryEditor();
}

void Controller::plugCollectionActions(KMenu* popup_) {
  if(!popup_) {
    return;
  }

  popup_->addAction(m_mainWindow->action("coll_rename_collection"));
  popup_->addAction(m_mainWindow->action("coll_fields"));
  popup_->addAction(m_mainWindow->action("change_entry_grouping"));
}

void Controller::plugEntryActions(KMenu* popup_) {
  if(!popup_) {
    return;
  }

//  m_mainWindow->m_newEntry->plug(popup_);
  popup_->addAction(m_mainWindow->m_editEntry);
  popup_->addAction(m_mainWindow->m_copyEntry);
  popup_->addAction(m_mainWindow->m_deleteEntry);
  popup_->addAction(m_mainWindow->m_mergeEntry);
  popup_->addAction(m_mainWindow->m_updateEntryMenu);
  // there's a bug in KActionMenu with KXMLGUIFactory::plugActionList
  // pluging the menu action isn't enough to have the popup get populated
  plugUpdateMenu(popup_);
  popup_->addSeparator();
  popup_->addAction(m_mainWindow->m_checkOutEntry);
}

void Controller::plugUpdateMenu(KMenu* popup_) {
  QMenu* updatePopup = 0;
  foreach(QAction* action, popup_->actions()) {
    if(action && action->text() == m_mainWindow->m_updateEntryMenu->text()) {
      updatePopup = action->menu();
      break;
    }
  }

  if(!updatePopup) {
    return;
  }

  // I can't figure out why the actions get duplicated, but they do
  // so clear them all
  updatePopup->removeAction(m_mainWindow->m_updateAll);
  foreach(QAction* action, m_mainWindow->m_fetchActions) {
    updatePopup->removeAction(action);
  }

  // clear separator, too
  updatePopup->clear();

  updatePopup->addAction(m_mainWindow->m_updateAll);
  updatePopup->addSeparator();
  foreach(QAction* action, m_mainWindow->m_fetchActions) {
    updatePopup->addAction(action);
  }
}

void Controller::updateActions() const {
  bool emptySelection = m_selectedEntries.isEmpty();
  m_mainWindow->stateChanged(QLatin1String("empty_selection"),
                             emptySelection ? KXMLGUIClient::StateNoReverse : KXMLGUIClient::StateReverse);
  foreach(QAction* action, m_mainWindow->m_fetchActions) {
    action->setEnabled(!emptySelection);
  }
  //only enable citation items when it's a bibliography
  bool isBibtex = Kernel::self()->collectionType() == Data::Collection::Bibtex;
  if(isBibtex) {
    m_mainWindow->action("cite_clipboard")->setEnabled(!emptySelection);
    m_mainWindow->action("cite_lyxpipe")->setEnabled(!emptySelection);
    m_mainWindow->action("cite_openoffice")->setEnabled(!emptySelection);
  }
  m_mainWindow->m_checkInEntry->setEnabled(canCheckIn());

  if(m_selectedEntries.count() < 2) {
    m_mainWindow->m_editEntry->setText(i18n("&Edit Entry..."));
    m_mainWindow->m_copyEntry->setText(i18n("D&uplicate Entry"));
    m_mainWindow->m_updateEntryMenu->setText(i18n("&Update Entry"));
    m_mainWindow->m_deleteEntry->setText(i18n("&Delete Entry"));
    m_mainWindow->m_mergeEntry->setEnabled(false);
  } else {
    m_mainWindow->m_editEntry->setText(i18n("&Edit Entries..."));
    m_mainWindow->m_copyEntry->setText(i18n("D&uplicate Entries"));
    m_mainWindow->m_updateEntryMenu->setText(i18n("&Update Entries"));
    m_mainWindow->m_deleteEntry->setText(i18n("&Delete Entries"));
    m_mainWindow->m_mergeEntry->setEnabled(true);
  }
}

void Controller::addedBorrower(Tellico::Data::BorrowerPtr borrower_) {
  m_mainWindow->addLoanView(); // just in case
  foreach(Observer* it, m_observers) {
    it->addBorrower(borrower_);
  }
  m_mainWindow->m_viewTabs->setTabBarHidden(false);
}

void Controller::modifiedBorrower(Tellico::Data::BorrowerPtr borrower_) {
  foreach(Observer* it, m_observers) {
    it->modifyBorrower(borrower_);
  }
  hideTabs();
}

void Controller::addedFilter(Tellico::FilterPtr filter_) {
  m_mainWindow->addFilterView(); // just in case
  foreach(Observer* it, m_observers) {
    it->addFilter(filter_);
  }
  m_mainWindow->m_viewTabs->setTabBarHidden(false);
}

void Controller::removedFilter(Tellico::FilterPtr filter_) {
  foreach(Observer* it, m_observers) {
    it->removeFilter(filter_);
  }
  hideTabs();
}

void Controller::slotCheckOut() {
  if(m_selectedEntries.isEmpty()) {
    return;
  }

  Data::EntryList loanedEntries = m_selectedEntries;

  // check to see if any of the entries are already on-loan, and warn user
  QMap<QString, Data::EntryPtr> alreadyLoaned;
  foreach(Data::BorrowerPtr borrower, Data::Document::self()->collection()->borrowers()) {
    foreach(Data::LoanPtr loan, borrower->loans()) {
      if(m_selectedEntries.contains(loan->entry())) {
        alreadyLoaned.insert(loan->entry()->title(), loan->entry());
      }
    }
  }
  if(!alreadyLoaned.isEmpty()) {
    KMessageBox::informationList(Kernel::self()->widget(),
                                 i18n("The following items are already loaned, but Tellico "
                                      "does not currently support lending an item multiple "
                                      "times. They will be removed from the list of items "
                                      "to lend."),
                                      alreadyLoaned.keys());
    QMap<QString, Data::EntryPtr>::const_iterator it = alreadyLoaned.constBegin();
    QMap<QString, Data::EntryPtr>::const_iterator end = alreadyLoaned.constEnd();
    for( ; it != end; ++it) {
      loanedEntries.removeAll(it.value());
    }
    if(loanedEntries.isEmpty()) {
      return;
    }
  }

  if(Kernel::self()->addLoans(loanedEntries)) {
    m_mainWindow->m_checkInEntry->setEnabled(true);
  }
}

void Controller::slotCheckIn() {
  slotCheckIn(m_selectedEntries);
}

void Controller::slotCheckIn(const Tellico::Data::EntryList& entries_) {
  if(entries_.isEmpty()) {
    return;
  }

  Data::LoanList loans;
  foreach(Data::EntryPtr entry, entries_) {
    // these have to be in the loop since if a borrower gets empty
    // it will be deleted, so the vector could change, for every entry iterator
    Data::BorrowerList vec = Data::Document::self()->collection()->borrowers();
    foreach(Data::BorrowerPtr borrower, vec) {
      Data::LoanPtr l = borrower->loan(entry);
      if(l) {
        loans.append(l);
        // assume it's only loaned once
        break;
      }
    }
  }

  if(Kernel::self()->removeLoans(loans)) {
    m_mainWindow->m_checkInEntry->setEnabled(false);
  }
  hideTabs();
}

void Controller::hideTabs() const {
  if((!m_mainWindow->m_filterView || m_mainWindow->m_filterView->isEmpty()) &&
     (!m_mainWindow->m_loanView || m_mainWindow->m_loanView->isEmpty())) {
    int idx = m_mainWindow->m_viewTabs->indexOf(m_mainWindow->m_groupView);
    m_mainWindow->m_viewTabs->setCurrentIndex(idx);
    m_mainWindow->m_viewTabs->setTabBarHidden(true);
  }
}

bool Controller::canCheckIn() const {
  foreach(Data::EntryPtr entry, m_selectedEntries) {
    if(entry->field(QLatin1String("loaned")) == QLatin1String("true")) {
      return true;
    }
  }
  return false;
}

void Controller::updatedFetchers() {
  m_mainWindow->updateEntrySources();
}

#include "controller.moc"
