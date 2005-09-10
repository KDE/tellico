/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
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

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kaction.h>

using Tellico::Controller;

Controller* Controller::s_self = 0;

Controller::Controller(MainWindow* parent_, const char* name_)
    : QObject(parent_, name_), m_mainWindow(parent_), m_working (false) {
  m_widgetBlocks.setAutoDelete(true);
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

void Controller::slotCollectionAdded(Data::Collection* coll_) {
//  myDebug() << "Controller::slotCollectionAdded()" << endl;
  // at start-up, this might get called too early, so check and bail
  if(!m_mainWindow->m_groupView) {
    return;
  }

  // do this first because the group view will need it later
  m_mainWindow->readCollectionOptions(coll_);

  // these might take some time, change the status message
  // the detailed view also has half the progress bar
  // this slot gets called after the importer has loaded the collection
  // so first bump it to 100% of the step, increase the step, then load
  // the document view
  m_mainWindow->slotUpdateFractionDone(1.0);
  m_mainWindow->m_currentStep = m_mainWindow->m_maxSteps;

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

  // The importer doesn't ever finish the import, better to do it here
  m_mainWindow->slotUpdateFractionDone(1.0);
  m_mainWindow->m_currentStep = 1;
  m_mainWindow->slotStatusMsg(i18n("Ready."));

  m_selectedEntries.clear();
  m_mainWindow->slotEntryCount();
  updateActions();

  connect(coll_, SIGNAL(signalGroupModified(Tellico::Data::Collection*, Tellico::Data::EntryGroup*)),
          m_mainWindow->m_groupView, SLOT(slotModifyGroup(Tellico::Data::Collection*, Tellico::Data::EntryGroup*)));

  connect(coll_, SIGNAL(signalRefreshField(Tellico::Data::Field*)),
          this, SLOT(slotRefreshField(Tellico::Data::Field*)));
}

void Controller::slotCollectionModified(Data::Collection* coll_) {
  // easiest thing is to signal collection deleted, then added?
  // FIXME: fixme Signals for delete collection and then added are yucky
  slotCollectionDeleted(coll_);
  slotCollectionAdded(coll_);
}

void Controller::slotCollectionDeleted(Data::Collection* coll_) {
//  kdDebug() << "Controller::slotCollectionDeleted()" << endl;

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

  // disconnect all signals from the collection to the controller
  // this is needed because the Collection::appendCollection() and mergeCollection()
  // functions signal collection deleted then added for the same collection
  disconnect(coll_, 0, this, 0);
}

void Controller::addedEntries(Data::EntryVec entries_) {
  blockAllSignals(true);
  for(Data::EntryVecIt entry = entries_.begin(); entry != entries_.end(); ++entry) {
    for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
      it->addEntry(entry);
    }
  }
  slotUpdateSelection(0, entries_);
  blockAllSignals(false);
}

void Controller::modifiedEntry(Data::Entry* entry_) {
  // when a new document is being loaded, loans are added to borrowers, which
  // end up calling Entry::checkIn() which called Document::saveEntry() which calls here
  // ignore that
  if(!m_mainWindow->m_initialized) {
    return;
  }
  blockAllSignals(true);
  for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
    it->modifyEntry(entry_);
  }
  m_mainWindow->m_viewStack->refresh(); // special case
  blockAllSignals(false);
}

void Controller::removedEntries(Data::EntryVec entries_) {
  blockAllUpdates(true);
  blockAllSignals(true);
  for(Data::EntryVecIt entry = entries_.begin(); entry != entries_.end(); ++entry) {
    for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
      it->removeEntry(entry);
      if(!m_selectedEntries.isEmpty() && entry == m_selectedEntries[0]) {
        m_mainWindow->m_viewStack->entryView()->clear();
      }
      m_selectedEntries.remove(entry);
      m_currentEntries.remove(entry);
    }
  }
  blockAllSignals(false);
  blockAllUpdates(false);
}

void Controller::addedField(Data::Collection* coll_, Data::Field* field_) {
  for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
    it->addField(coll_, field_);
  }
  m_mainWindow->m_viewStack->refresh();
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
}

void Controller::removedField(Data::Collection* coll_, Data::Field* field_) {
//  kdDebug() << "Controller::slotFieldDeleted() - " << field_->name() << endl;
  for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
    it->removeField(coll_, field_);
  }
  m_mainWindow->m_viewStack->refresh();
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
}

void Controller::modifiedField(Data::Collection* coll_, Data::Field* oldField_, Data::Field* newField_) {
  for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
    it->modifyField(coll_, oldField_, newField_);
  }
  m_mainWindow->m_viewStack->refresh();
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
}

void Controller::reorderedFields(Data::Collection* coll_) {
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
//  kdDebug() << "Controller::slotUpdateSelection() entryList - " << list_.count() << endl;

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

void Controller::slotRefreshField(Data::Field* field_) {
  kdDebug() << "Controller::slotRefreshField()" << endl;
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

  // need to create copies
  Data::EntryVec entries;
  for(Data::EntryVecIt it = m_selectedEntries.begin(); it != m_selectedEntries.end(); ++it) {
    entries.append(new Data::Entry(*it));
  }
  Kernel::self()->saveEntries(Data::EntryVec(), entries);
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

void Controller::blockAllUpdates(bool block_) {
  if(!m_mainWindow->m_initialized) {
    return;
  }

  // if we don't want to block, clear and auto-delete all blocks
  if(!block_) {
    m_widgetBlocks.clear();
    return;
  }

  m_widgetBlocks.append(new GUI::WidgetUpdateBlocker(m_mainWindow->m_detailedView));
  m_widgetBlocks.append(new GUI::WidgetUpdateBlocker(m_mainWindow->m_groupView));
  m_widgetBlocks.append(new GUI::WidgetUpdateBlocker(m_mainWindow->m_viewStack->iconView()));
  if(m_mainWindow->m_loanView) {
    m_widgetBlocks.append(new GUI::WidgetUpdateBlocker(m_mainWindow->m_loanView));
  }
  if(m_mainWindow->m_filterView) {
    m_widgetBlocks.append(new GUI::WidgetUpdateBlocker(m_mainWindow->m_filterView));
  }
}

void Controller::slotUpdateFilter(Filter* filter_) {
//  kdDebug() << "Controller::slotUpdateFilter()" << endl;
  blockAllSignals(true);

  // the view takes over ownership of the filter
  m_mainWindow->m_detailedView->clearSelection();
  m_selectedEntries.clear();
  updateActions();
  m_mainWindow->m_viewStack->iconView()->clearSelection();

  m_mainWindow->m_detailedView->setFilter(filter_); // takes ownership

  blockAllSignals(false);

  m_mainWindow->slotEntryCount();
}

void Controller::editEntry(const Data::Entry&) const {
  m_mainWindow->slotShowEntryEditor();
}

void Controller::plugCollectionActions(QWidget* widget_) {
  m_mainWindow->action("coll_rename_collection")->plug(widget_);
  m_mainWindow->action("coll_fields")->plug(widget_);
  m_mainWindow->action("change_entry_grouping")->plug(widget_);
}

void Controller::plugEntryActions(QWidget* widget_) {
//  m_mainWindow->m_newEntry->plug(widget_);
  m_mainWindow->m_editEntry->plug(widget_);
  m_mainWindow->m_copyEntry->plug(widget_);
  m_mainWindow->m_deleteEntry->plug(widget_);
  m_mainWindow->m_checkOutEntry->plug(widget_);
}

void Controller::updateActions() const {
  bool emptySelection = m_selectedEntries.isEmpty();
  m_mainWindow->m_editEntry->setEnabled(!emptySelection);
  m_mainWindow->m_copyEntry->setEnabled(!emptySelection);
  m_mainWindow->m_deleteEntry->setEnabled(!emptySelection);
  m_mainWindow->m_checkOutEntry->setEnabled(!emptySelection);
  m_mainWindow->m_checkInEntry->setEnabled(canCheckIn());

  if(m_selectedEntries.count() < 2) {
    m_mainWindow->m_editEntry->setText(i18n("&Edit Entry..."));
    m_mainWindow->m_copyEntry->setText(i18n("&Copy Entry"));
    m_mainWindow->m_deleteEntry->setText(i18n("&Delete Entry"));
  } else {
    m_mainWindow->m_editEntry->setText(i18n("&Edit Entries..."));
    m_mainWindow->m_copyEntry->setText(i18n("&Copy Entries"));
    m_mainWindow->m_deleteEntry->setText(i18n("&Delete Entries"));
  }
}

void Controller::addedBorrower(Data::Borrower* borrower_) {
  m_mainWindow->addLoanView(); // just in case
  for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
    it->addBorrower(borrower_);
  }
  m_mainWindow->m_viewTabs->setTabBarHidden(false);
}

void Controller::modifiedBorrower(Data::Borrower* borrower_) {
  for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
    it->modifyBorrower(borrower_);
  }
  hideTabs();
}

void Controller::addedFilter(Filter* filter_) {
  m_mainWindow->addFilterView(); // just in case
  for(ObserverVec::Iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
    it->addFilter(filter_);
  }
  m_mainWindow->m_viewTabs->setTabBarHidden(false);
}

void Controller::removedFilter(Filter* filter_) {
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
  QMap<QString, Data::Entry*> alreadyLoaned;
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
    QMapConstIterator<QString, Data::Entry*> it = alreadyLoaned.constBegin();
    QMapConstIterator<QString, Data::Entry*> end = alreadyLoaned.constEnd();
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
      Data::Loan* l = bIt->loan(it);
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

#include "controller.moc"
