/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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
#include "latin1literal.h"
#include "collection.h"
#include "document.h"
#include "borrower.h"
#include "filterview.h"
#include "loanview.h"
#include "entryitem.h"
#include "gui/tabcontrol.h"
#include "calendarhandler.h"
#include "tellico_debug.h"
#include "groupiterator.h"
#include "tellico_utils.h"
#include "entryupdater.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kaction.h>
#include <ktoolbarbutton.h>

using Tellico::Controller;

Controller* Controller::s_self = 0;

Controller::Controller(MainWindow* parent_, const char* name_)
    : QObject(parent_, name_), m_mainWindow(parent_), m_working (false) {
}

void Controller::addObserver(Observer* obs) {
  m_observers.push_back(obs);
}

void Controller::removeObserver(Observer* obs) {
  m_observers.remove(obs);
}

Tellico::GroupIterator Controller::groupIterator() const {
  return GroupIterator(m_mainWindow->m_groupView);
}

QString Controller::groupBy() const {
  return m_mainWindow->m_groupView->groupBy();
}

QStringList Controller::expandedGroupBy() const {
  QStringList g = groupBy();
  // special case for pseudo-group
  if(g[0] == Data::Collection::s_peopleGroupName) {
    g.clear();
    Data::FieldVec fields = Data::Document::self()->collection()->peopleFields();
    for(Data::FieldVec::Iterator it = fields.begin(); it != fields.end(); ++it) {
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

Tellico::Data::EntryVec Controller::visibleEntries() {
  return m_mainWindow->m_detailedView->visibleEntries();
}

void Controller::slotCollectionAdded(Data::CollPtr coll_) {
//  myDebug() << "Controller::slotCollectionAdded()" << endl;
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

  updateActions();

  connect(coll_, SIGNAL(signalGroupsModified(Tellico::Data::CollPtr, PtrVector<Tellico::Data::EntryGroup>)),
          m_mainWindow->m_groupView, SLOT(slotModifyGroups(Tellico::Data::CollPtr, PtrVector<Tellico::Data::EntryGroup>)));

  connect(coll_, SIGNAL(signalRefreshField(Tellico::Data::FieldPtr)),
          this, SLOT(slotRefreshField(Tellico::Data::FieldPtr)));
}

void Controller::slotCollectionModified(Data::CollPtr coll_) {
  // easiest thing is to signal collection deleted, then added?
  // FIXME: Signals for delete collection and then added are yucky
  slotCollectionDeleted(coll_);
  slotCollectionAdded(coll_);
}

void Controller::slotCollectionDeleted(Data::CollPtr coll_) {
//  myDebug() << "Controller::slotCollectionDeleted()" << endl;

  blockAllSignals(true);
  m_mainWindow->saveCollectionOptions(coll_);
  m_mainWindow->m_groupView->removeCollection(coll_);
  if(m_mainWindow->m_filterView) {
    m_mainWindow->m_filterView->clear();
  }
  if(m_mainWindow->m_loanView) {
    m_mainWindow->m_loanView->clear();
  }
  m_mainWindow->m_detailedView->removeCollection(coll_);
  m_mainWindow->m_viewStack->clear();
  blockAllSignals(false);

  // disconnect all signals from the collection
  // this is needed because the Collection::appendCollection() and mergeCollection()
  // functions signal collection deleted then added for the same collection
  coll_->disconnect();
}

void Controller::addedEntries(Data::EntryVec entries_) {
  blockAllSignals(true);
  for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
    it->addEntries(entries_);
  }
  m_mainWindow->slotQueueFilter();
  slotUpdateSelection(0, entries_);
  blockAllSignals(false);
}

void Controller::modifiedEntries(Data::EntryVec entries_) {
  // when a new document is being loaded, loans are added to borrowers, which
  // end up calling Entry::checkIn() which called Document::saveEntry() which calls here
  // ignore that
  if(!m_mainWindow->m_initialized) {
    return;
  }
  blockAllSignals(true);
  for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
    it->modifyEntries(entries_);
  }
  m_mainWindow->m_viewStack->entryView()->slotRefresh(); // special case
  m_mainWindow->slotQueueFilter();
  blockAllSignals(false);
}

void Controller::removedEntries(Data::EntryVec entries_) {
  blockAllSignals(true);
  for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
    it->removeEntries(entries_);
    m_mainWindow->m_viewStack->entryView()->clear();
    m_selectedEntries.clear();
    m_currentEntries.clear();
  }
  m_mainWindow->slotQueueFilter();
  blockAllSignals(false);
}

void Controller::addedField(Data::CollPtr coll_, Data::FieldPtr field_) {
  for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
    it->addField(coll_, field_);
  }
  m_mainWindow->m_viewStack->refresh();
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
  m_mainWindow->slotQueueFilter();
}

void Controller::removedField(Data::CollPtr coll_, Data::FieldPtr field_) {
//  myDebug() << "Controller::slotFieldDeleted() - " << field_->name() << endl;
  for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
    it->removeField(coll_, field_);
  }
  m_mainWindow->m_viewStack->refresh();
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
  m_mainWindow->slotQueueFilter();
}

void Controller::modifiedField(Data::CollPtr coll_, Data::FieldPtr oldField_, Data::FieldPtr newField_) {
  for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
    it->modifyField(coll_, oldField_, newField_);
  }
  m_mainWindow->m_viewStack->refresh();
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
  m_mainWindow->slotQueueFilter();
}

void Controller::reorderedFields(Data::CollPtr coll_) {
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
  m_mainWindow->m_editDialog->clear();
//  m_mainWindow->m_viewStack->clear(); let this stay

  blockAllSignals(false);

  m_selectedEntries.clear();
  updateActions();
  m_mainWindow->slotEntryCount();
  m_working = false;
}

void Controller::slotUpdateSelection(QWidget* widget_, const Data::EntryVec& entries_) {
  if(m_working) {
    return;
  }
  m_working = true;
//  myDebug() << "Controller::slotUpdateSelection() - " << (widget_ ? widget_->className() : "") << endl;

  blockAllSignals(true);
// in the list view and group view, if entries are selected in one, clear selection in other
  if(widget_ != m_mainWindow->m_detailedView) {
    m_mainWindow->m_detailedView->clearSelection();
  }
  if(widget_ != m_mainWindow->m_groupView) {
    m_mainWindow->m_groupView->clearSelection();
  }
  if(m_mainWindow->m_filterView && widget_ != m_mainWindow->m_filterView) {
    m_mainWindow->m_filterView->clearSelection();
  }
  if(m_mainWindow->m_loanView && widget_ != m_mainWindow->m_loanView) {
    m_mainWindow->m_loanView->clearSelection();
  }
  if(widget_ != m_mainWindow->m_editDialog) {
    m_mainWindow->m_editDialog->setContents(entries_);
  }
  // only show first one
  if(widget_ && widget_ != m_mainWindow->m_viewStack->iconView()) {
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
  // if there are currently multiple selected, then do nothing
  if(m_selectedEntries.count() != 1) {
    return;
  }
  // find the widget that has an entry selected
  GUI::ListView* view = m_mainWindow->m_detailedView;
  GUI::ListViewItemList items = view->selectedItems();
  if(items.isEmpty()) {
    view = m_mainWindow->m_groupView;
    items = view->selectedItems();
  }
  if(items.isEmpty() && m_mainWindow->m_filterView) {
    view = m_mainWindow->m_filterView;
    items = view->selectedItems();
  }
  if(items.isEmpty() && m_mainWindow->m_loanView) {
    view = m_mainWindow->m_loanView;
    items = view->selectedItems();
  }
  if(items.count() != 1) {
    return;
  }
  GUI::ListViewItem* item = items.first();
  if(item->isEntryItem()) {
    bool looped = false;
    // check sanity
    if(m_selectedEntries.front() != static_cast<EntryItem*>(item)->entry()) {
      myDebug() << "Controller::slotGoNextEntry() - entries don't match!" << endl;
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
      slotUpdateSelection(view, e);
    }
  }
}

void Controller::slotUpdateCurrent(const Data::EntryVec& entries_) {
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
  if(source_.isEmpty() || source_ == Latin1Literal("_all")) {
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
    QString dontAsk = QString::fromLatin1("DeleteEntry");
    int ret = KMessageBox::warningContinueCancel(Kernel::self()->widget(), str, i18n("Delete Entry"),
                                                 KGuiItem(i18n("&Delete"), QString::fromLatin1("editdelete")), dontAsk);
    if(ret != KMessageBox::Continue) {
      return;
    }
  } else {
    QStringList names;
    for(Data::EntryVecIt entry = m_selectedEntries.begin(); entry != m_selectedEntries.end(); ++entry) {
      names += entry->title();
    }
    QString str = i18n("Do you really want to delete these entries?");
    // historically called DeleteMultipleBooks, don't change
    QString dontAsk = QString::fromLatin1("DeleteMultipleBooks");
    int ret = KMessageBox::warningContinueCancelList(Kernel::self()->widget(), str, names,
                                                     i18n("Delete Multiple Entries"),
                                                     KGuiItem(i18n("&Delete"), QString::fromLatin1("editdelete")), dontAsk);
    if(ret != KMessageBox::Continue) {
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
}

void Controller::slotRefreshField(Data::FieldPtr field_) {
//  myDebug() << "Controller::slotRefreshField()" << endl;
  // group view only needs to refresh if it's the title
  if(field_->name() == Latin1Literal("title")) {
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
  Data::EntryVec old = m_selectedEntries;

  GUI::CursorSaver cs;
  // need to create copies
  Data::EntryVec entries;
  for(Data::EntryVecIt it = m_selectedEntries.begin(); it != m_selectedEntries.end(); ++it) {
    entries.append(new Data::Entry(*it));
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
  if(m_mainWindow->m_loanView) {
    m_mainWindow->m_loanView->blockSignals(block_);
  }
  if(m_mainWindow->m_filterView) {
    m_mainWindow->m_filterView->blockSignals(block_);
  }
  m_mainWindow->m_editDialog->blockSignals(block_);
  m_mainWindow->m_viewStack->iconView()->blockSignals(block_);
}

void Controller::slotUpdateFilter(FilterPtr filter_) {
//  myDebug() << "Controller::slotUpdateFilter()" << endl;
  blockAllSignals(true);

  updateActions();
//  m_selectedEntries.clear();
//  m_mainWindow->m_detailedView->clearSelection();
//  m_mainWindow->m_viewStack->iconView()->clearSelection();

  // the view takes over ownership of the filter
  m_mainWindow->m_detailedView->setFilter(filter_);

  blockAllSignals(false);

  m_mainWindow->slotEntryCount();
}

void Controller::editEntry(Data::EntryPtr) const {
  m_mainWindow->slotShowEntryEditor();
}

void Controller::plugCollectionActions(QPopupMenu* popup_) {
  if(!popup_) {
    return;
  }

  m_mainWindow->action("coll_rename_collection")->plug(popup_);
  m_mainWindow->action("coll_fields")->plug(popup_);
  m_mainWindow->action("change_entry_grouping")->plug(popup_);
}

void Controller::plugEntryActions(QPopupMenu* popup_) {
  if(!popup_) {
    return;
  }

//  m_mainWindow->m_newEntry->plug(popup_);
  m_mainWindow->m_editEntry->plug(popup_);
  m_mainWindow->m_copyEntry->plug(popup_);
  m_mainWindow->m_deleteEntry->plug(popup_);
  m_mainWindow->m_updateEntryMenu->plug(popup_);
  // there's a bug in KActionMenu with KXMLGUIFactory::plugActionList
  // pluging the menu action isn't enough to have the popup get populated
  plugUpdateMenu(popup_);
  popup_->insertSeparator();
  m_mainWindow->m_checkOutEntry->plug(popup_);
}

void Controller::plugUpdateMenu(QPopupMenu* popup_) {
  QPopupMenu* updatePopup = 0;
  const uint count = popup_->count();
  for(uint i = 0; i < count; ++i) {
    QMenuItem* item = popup_->findItem(popup_->idAt(i));
    if(item && item->text() == m_mainWindow->m_updateEntryMenu->text()) {
      updatePopup = item->popup();
      break;
    }
  }

  if(!updatePopup) {
    return;
  }

  // I can't figure out why the actions get duplicated, but they do
  // so clear them all
  m_mainWindow->m_updateAll->unplug(updatePopup);
  for(QPtrListIterator<KAction> it(m_mainWindow->m_fetchActions); it.current(); ++it) {
    it.current()->unplug(updatePopup);
  }

  // clear separator, too
  updatePopup->clear();

  m_mainWindow->m_updateAll->plug(updatePopup);
  updatePopup->insertSeparator();
  for(QPtrListIterator<KAction> it(m_mainWindow->m_fetchActions); it.current(); ++it) {
    it.current()->plug(updatePopup);
  }
}

void Controller::updateActions() const {
  bool emptySelection = m_selectedEntries.isEmpty();
  m_mainWindow->stateChanged(QString::fromLatin1("empty_selection"),
                             emptySelection ? KXMLGUIClient::StateNoReverse : KXMLGUIClient::StateReverse);
  for(QPtrListIterator<KAction> it(m_mainWindow->m_fetchActions); it.current(); ++it) {
    it.current()->setEnabled(!emptySelection);
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
  } else {
    m_mainWindow->m_editEntry->setText(i18n("&Edit Entries..."));
    m_mainWindow->m_copyEntry->setText(i18n("D&uplicate Entries"));
    m_mainWindow->m_updateEntryMenu->setText(i18n("&Update Entries"));
    m_mainWindow->m_deleteEntry->setText(i18n("&Delete Entries"));
  }
}

void Controller::addedBorrower(Data::BorrowerPtr borrower_) {
  m_mainWindow->addLoanView(); // just in case
  for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
    it->addBorrower(borrower_);
  }
  m_mainWindow->m_viewTabs->setTabBarHidden(false);
}

void Controller::modifiedBorrower(Data::BorrowerPtr borrower_) {
  for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
    it->modifyBorrower(borrower_);
  }
  hideTabs();
}

void Controller::addedFilter(FilterPtr filter_) {
  m_mainWindow->addFilterView(); // just in case
  for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
    it->addFilter(filter_);
  }
  m_mainWindow->m_viewTabs->setTabBarHidden(false);
}

void Controller::removedFilter(FilterPtr filter_) {
  for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
    it->removeFilter(filter_);
  }
  hideTabs();
}

void Controller::slotCheckOut() {
  if(m_selectedEntries.isEmpty()) {
    return;
  }

  Data::EntryVec loanedEntries = m_selectedEntries;

  // check to see if any of the entries are already on-loan, and warn user
  QMap<QString, Data::EntryPtr> alreadyLoaned;
  const Data::BorrowerVec& borrowers = Data::Document::self()->collection()->borrowers();
  for(Data::BorrowerVec::ConstIterator it = borrowers.begin(); it != borrowers.end(); ++it) {
    const Data::LoanVec& loans = it->loans();
    for(Data::LoanVec::ConstIterator it2 = loans.begin(); it2 != loans.end(); ++it2) {
      if(m_selectedEntries.contains(it2->entry())) {
        alreadyLoaned.insert(it2->entry()->title(), it2->entry());
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
    QMapConstIterator<QString, Data::EntryPtr> it = alreadyLoaned.constBegin();
    QMapConstIterator<QString, Data::EntryPtr> end = alreadyLoaned.constEnd();
    for( ; it != end; ++it) {
      loanedEntries.remove(it.data());
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

void Controller::slotCheckIn(const Data::EntryVec& entries_) {
  if(entries_.isEmpty()) {
    return;
  }

  Data::LoanVec loans;
  for(Data::EntryVec::ConstIterator it = entries_.begin(); it != entries_.end(); ++it) {
    // these have to be in the loop since if a borrower gets empty
    // it will be deleted, so the vector could change, for every entry iterator
    Data::BorrowerVec vec = Data::Document::self()->collection()->borrowers();
    // vec.end() must be in the loop, do NOT cache the value, it could change!
    for(Data::BorrowerVec::Iterator bIt = vec.begin(); bIt != vec.end(); ++bIt) {
      Data::LoanPtr l = bIt->loan(it.data());
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
  if((!m_mainWindow->m_filterView || m_mainWindow->m_filterView->childCount() == 0) &&
     (!m_mainWindow->m_loanView || m_mainWindow->m_loanView->childCount() == 0)) {
    m_mainWindow->m_viewTabs->showPage(m_mainWindow->m_groupView);
    m_mainWindow->m_viewTabs->setTabBarHidden(true);
  }
}

inline
bool Controller::canCheckIn() const {
  for(Data::EntryVec::ConstIterator entry = m_selectedEntries.begin(); entry != m_selectedEntries.end(); ++entry) {
    if(entry->field(QString::fromLatin1("loaned")) == Latin1Literal("true")) {
      return true;
    }
  }
  return false;
}

void Controller::updatedFetchers() {
  m_mainWindow->updateEntrySources();
}

#include "controller.moc"
