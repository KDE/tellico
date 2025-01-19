/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include "controller.h"
#include "mainwindow.h"
#include "groupview.h"
#include "detailedlistview.h"
#include "entryeditdialog.h"
#include "entryview.h"
#include "entryiconview.h"
#include "entry.h"
#include "entrygroup.h"
#include "field.h"
#include "filter.h"
#include "tellico_kernel.h"
#include "collection.h"
#include "document.h"
#include "borrower.h"
#include "filterview.h"
#include "loanview.h"
#include "entryupdater.h"
#include "entrymerger.h"
#include "utils/cursorsaver.h"
#include "gui/lineedit.h"
#include "gui/tabwidget.h"
#include "tellico_debug.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KActionMenu>

#include <QMenu>

using Tellico::Controller;

Controller* Controller::s_self = nullptr;

Controller::Controller(Tellico::MainWindow* parent_)
    : QObject(parent_), m_mainWindow(parent_), m_working(false) {
}

Controller::~Controller() {
}

void Controller::addObserver(Tellico::Observer* obs) {
  m_observers.append(obs);
}

void Controller::removeObserver(Tellico::Observer* obs) {
  m_observers.removeAll(obs);
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
    foreach(Data::FieldPtr field, fields) {
      g << field->name();
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
  MARK;
  // at start-up, this might get called too early, so check and bail
  if(!coll_ || !m_mainWindow->m_groupView) {
    return;
  }

  // do this first because the group view will need it later
  m_mainWindow->readCollectionOptions(coll_);
  m_mainWindow->slotUpdateToolbarIcons();
  m_mainWindow->updateEntrySources(); // has to be called before all the addCollection()
  // calls in the widgets since they may want menu updates

  m_mainWindow->m_detailedView->addCollection(coll_);
  m_mainWindow->m_groupView->addCollection(coll_);
  m_mainWindow->m_editDialog->resetLayout(coll_);
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

  m_mainWindow->slotStatusMsg(i18n("Ready."));

  m_selectedEntries.clear();
  m_mainWindow->slotEntryCount();

  // there really should be a lot of signals to connect to, but right now, the only one
  // is used when a field is added on a merge
  connect(&*coll_, &Data::Collection::mergeAddedField,
          this, &Controller::slotFieldAdded);

  Q_EMIT collectionAdded(coll_->type());

  updateActions();

  connect(&*coll_, &Data::Collection::signalGroupsModified,
          m_mainWindow->m_groupView, &GroupView::slotModifyGroups);
  connect(&*coll_, &Data::Collection::signalRefreshField,
          this, &Controller::slotRefreshField);
}

void Controller::slotCollectionModified(Tellico::Data::CollPtr coll_, bool structuralChange_) {
  Data::EntryList prevSelection = m_selectedEntries;
  blockAllSignals(true);
  m_mainWindow->m_groupView->removeCollection(coll_);
  if(m_mainWindow->m_filterView) {
    m_mainWindow->m_filterView->slotReset();
  }
  if(m_mainWindow->m_loanView) {
    m_mainWindow->m_loanView->slotReset();
  }
  // TODO: removing and adding the collection in the detailed view is overkill
  // find a more elegant way to refresh the view
  m_mainWindow->m_detailedView->removeCollection(coll_);
  blockAllSignals(false);

  if(structuralChange_) {
    m_mainWindow->m_editDialog->resetLayout(coll_);
  }
  // the selected entries list gets cleared when the detailed list view removes the collection
  m_selectedEntries = prevSelection;
  m_mainWindow->m_editDialog->setContents(m_selectedEntries);
  m_mainWindow->m_detailedView->addCollection(coll_);
  m_mainWindow->m_groupView->addCollection(coll_);
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

  m_mainWindow->slotStatusMsg(i18n("Ready."));
  m_mainWindow->slotEntryCount();

  // https://bugs.kde.org/show_bug.cgi?id=386549
  // at some point, I need to revisit the ::setImagesAreAvailable() methodology
  // there are too many workarounds in the code for that
  m_mainWindow->m_detailedView->slotRefreshImages();
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
  m_mainWindow->m_entryView->clear();
  blockAllSignals(false);

  // disconnect all signals from the collection
  // this is needed because the Collection::appendCollection() and mergeCollection()
  // functions signal collection deleted then added for the same collection
  coll_->disconnect();
}

void Controller::slotFieldAdded(Tellico::Data::CollPtr coll_, Tellico::Data::FieldPtr field_) {
  addedField(coll_, field_);
}

// TODO: should be adding entries to models rather than to widget observers
void Controller::addedEntries(Tellico::Data::EntryList entries_) {
  blockAllSignals(true);
  foreach(Observer* obs, m_observers) {
    obs->addEntries(entries_);
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
  foreach(Observer* obs, m_observers) {
    obs->modifyEntries(entries_);
  }
  m_mainWindow->m_entryView->slotRefresh(); // special case
  blockAllSignals(false);
}

void Controller::removedEntries(Tellico::Data::EntryList entries_) {
  blockAllSignals(true);
  foreach(Observer* obs, m_observers) {
    obs->removeEntries(entries_);
  }
  foreach(Data::EntryPtr entry, entries_) {
    m_selectedEntries.removeAll(entry);
  }
  m_mainWindow->slotEntryCount();
  m_mainWindow->slotQueueFilter();
  blockAllSignals(false);
}

void Controller::addedField(Tellico::Data::CollPtr coll_, Tellico::Data::FieldPtr field_) {
  foreach(Observer* obs, m_observers) {
    obs->addField(coll_, field_);
  }
  m_mainWindow->m_entryView->slotRefresh();
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
  m_mainWindow->slotQueueFilter();
}

void Controller::removedField(Tellico::Data::CollPtr coll_, Tellico::Data::FieldPtr field_) {
  foreach(Observer* obs, m_observers) {
    obs->removeField(coll_, field_);
  }
  m_mainWindow->m_entryView->slotRefresh();
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
  m_mainWindow->slotQueueFilter();
}

void Controller::modifiedField(Tellico::Data::CollPtr coll_, Tellico::Data::FieldPtr oldField_, Tellico::Data::FieldPtr newField_) {
  foreach(Observer* obs, m_observers) {
    obs->modifyField(coll_, oldField_, newField_);
  }
  m_mainWindow->m_entryView->slotRefresh();
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
  m_mainWindow->slotQueueFilter();
}

void Controller::reorderedFields(Tellico::Data::CollPtr coll_) {
  m_mainWindow->m_editDialog->resetLayout(coll_);
  m_mainWindow->m_detailedView->reorderFields(coll_->fields());
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
  m_mainWindow->m_entryView->slotRefresh();
}

void Controller::slotClearSelection() {
  if(m_working) {
    return;
  }

  m_working = true;
  blockAllSignals(true);

  m_mainWindow->m_detailedView->clearSelection();
  m_mainWindow->m_iconView->clearSelection();
  m_mainWindow->m_groupView->clearSelection();
  if(m_mainWindow->m_filterView) {
    m_mainWindow->m_filterView->clearSelection();
  }
  if(m_mainWindow->m_loanView) {
    m_mainWindow->m_loanView->clearSelection();
  }

  blockAllSignals(false);

  m_selectedEntries.clear();
  updateActions();
  m_mainWindow->slotEntryCount();
  m_working = false;
}

void Controller::slotUpdateSelection(const Tellico::Data::EntryList& entries_) {
  if(m_working) {
    return;
  }
  m_working = true;

  m_selectedEntries = entries_;
  updateActions();
  m_mainWindow->slotEntryCount();
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
    QString dontAsk = QStringLiteral("DeleteEntry");
    int ret = KMessageBox::warningContinueCancel(Kernel::self()->widget(), str, i18n("Delete Entry"),
                                                 KStandardGuiItem::del(),
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
    QString dontAsk = QStringLiteral("DeleteMultipleBooks");
    int ret = KMessageBox::warningContinueCancelList(Kernel::self()->widget(), str, names,
                                                     i18n("Delete Multiple Entries"),
                                                     KStandardGuiItem::del(),
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
  m_mainWindow->m_entryView->slotRefresh();
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
  slotUpdateSelection(old);
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
  m_mainWindow->m_iconView->blockSignals(block_);
}

void Controller::slotUpdateFilter(Tellico::FilterPtr filter_) {
  blockAllSignals(true);

  // the view takes over ownership of the filter
  if(filter_ && !filter_->isEmpty()) {
    // clear the icon view selection only
    // the detailed view takes care of itself
    m_mainWindow->m_iconView->clearSelection();
    m_selectedEntries.clear();
  }
  updateActions();

  m_mainWindow->m_detailedView->setFilter(filter_); // takes ownership
  if(!filter_ && m_mainWindow->m_filterView && !m_mainWindow->m_dontQueueFilter) {
    // for example, when quick filter clears the selection
    // the check against m_dontQueueFilter is to prevent the situation when the FilterView has an Entry selected
    // which sends an empty filter selection, which would then clear the whole FilterView selection
    m_mainWindow->m_filterView->clearSelection();
  }

  blockAllSignals(false);

  m_mainWindow->slotEntryCount();
}

void Controller::clearFilter() {
  blockAllSignals(true);
  m_mainWindow->m_quickFilter->clear();
  m_mainWindow->m_detailedView->setFilter(Tellico::FilterPtr());
  blockAllSignals(false);
}

void Controller::editEntry(Tellico::Data::EntryPtr entry_) const {
  m_mainWindow->slotShowEntryEditor();
  m_mainWindow->m_editDialog->setContents(Data::EntryList() << entry_);
}

void Controller::plugCollectionActions(QMenu* popup_) {
  if(!popup_) {
    return;
  }

  popup_->addAction(m_mainWindow->action(QLatin1String("coll_rename_collection")));
  popup_->addAction(m_mainWindow->action(QLatin1String("coll_fields")));
  popup_->addAction(m_mainWindow->action(QLatin1String("change_entry_grouping")));
}

void Controller::plugEntryActions(QMenu* popup_) {
  if(!popup_) {
    return;
  }

//  m_mainWindow->m_newEntry->plug(popup_);
  popup_->addAction(m_mainWindow->m_editEntry);
  popup_->addAction(m_mainWindow->m_copyEntry);
  popup_->addAction(m_mainWindow->m_deleteEntry);
  popup_->addAction(m_mainWindow->m_mergeEntry);
  popup_->addMenu(m_mainWindow->m_updateEntryMenu->menu());
  // there's a bug in KActionMenu with KXMLGUIFactory::plugActionList
  // plugging the menu isn't enough to have the popup get populated
  plugUpdateMenu(popup_);
  popup_->addSeparator();
  popup_->addAction(m_mainWindow->m_checkOutEntry);
}

QMenu* Controller::plugSortActions(QMenu* popup_) {
  if(!popup_) {
    return nullptr;
  }

  QMenu* sortMenu = popup_->addMenu(i18n("&Sort By"));
  sortMenu->setIcon(QIcon::fromTheme(QStringLiteral("view-sort"),
                                     QIcon::fromTheme(QStringLiteral("view-sort-ascending"))));
  foreach(Data::FieldPtr field, Data::Document::self()->collection()->fields()) {
    // not allowed to sort by Image, Table, Para, or URL
    if(field->type() == Data::Field::Image ||
       field->type() == Data::Field::Table ||
       field->type() == Data::Field::URL ||
       field->type() == Data::Field::Para) {
      continue;
    }
    sortMenu->addAction(field->title())->setData(QVariant::fromValue(field));
  }
  return sortMenu;
}

void Controller::plugUpdateMenu(QMenu* popup_) {
  QMenu* updatePopup = nullptr;
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
  const bool emptySelection = m_selectedEntries.isEmpty();
  m_mainWindow->stateChanged(QStringLiteral("empty_selection"),
                             emptySelection ? KXMLGUIClient::StateNoReverse : KXMLGUIClient::StateReverse);
  foreach(QAction* action, m_mainWindow->m_fetchActions) {
    action->setEnabled(!emptySelection);
  }
  //only enable citation items when it's a bibliography
  const bool isBibtex = Kernel::self()->collectionType() == Data::Collection::Bibtex;
  if(isBibtex) {
    m_mainWindow->action(QLatin1String("cite_clipboard"))->setEnabled(!emptySelection);
    m_mainWindow->action(QLatin1String("cite_lyxpipe"))->setEnabled(!emptySelection);
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
  foreach(Observer* obs, m_observers) {
    obs->addBorrower(borrower_);
  }
  m_mainWindow->m_viewTabs->setTabBarHidden(false);
}

void Controller::modifiedBorrower(Tellico::Data::BorrowerPtr borrower_) {
  foreach(Observer* obs, m_observers) {
    if(borrower_->isEmpty()) {
      obs->removeBorrower(borrower_);
    } else {
      obs->modifyBorrower(borrower_);
    }
  }
  hideTabs();
}

void Controller::addedFilter(Tellico::FilterPtr filter_) {
  m_mainWindow->addFilterView(); // just in case
  foreach(Observer* obs, m_observers) {
    obs->addFilter(filter_);
  }
  m_mainWindow->m_viewTabs->setTabBarHidden(false);
}

void Controller::removedFilter(Tellico::FilterPtr filter_) {
  foreach(Observer* obs, m_observers) {
    obs->removeFilter(filter_);
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
    if(entry->field(QStringLiteral("loaned")) == QLatin1String("true")) {
      return true;
    }
  }
  return false;
}

void Controller::updatedFetchers() {
  m_mainWindow->updateEntrySources();
}
