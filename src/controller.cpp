/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "mainwindow.h"
#include "controller.h"
#include "groupview.h"
#include "detailedlistview.h"
#include "entryeditdialog.h"
#include "viewstack.h"
#include "entryview.h"
#include "entryiconview.h"
#include "imagefactory.h"
#include "filter.h"
#include "filterdialog.h"
#include "tellico_kernel.h"
#include "latin1literal.h"
#include "collection.h"
#include "document.h"

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kaction.h>

using Tellico::Controller;

Controller* Controller::s_self = 0;

Controller::Controller(MainWindow* parent_, const char* name_)
    : QObject(parent_, name_), m_mainWindow(parent_), m_working (false) {
}

void Controller::initWidgets() {
  m_groupView = m_mainWindow->m_groupView;
  m_detailedView = m_mainWindow->m_detailedView;
  m_editDialog = m_mainWindow->m_editDialog;
  m_viewStack = m_mainWindow->m_viewStack;
}

void Controller::slotCollectionAdded(Data::Collection* coll_) {
//  kdDebug() << "Controller::slotCollectionAdded()" << endl;

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
  m_detailedView->addCollection(coll_);
  m_groupView->addCollection(coll_);
  m_editDialog->setLayout(coll_);
//  blockAllSignals(false);

  // The importer doesn't ever finish the importer, better to do it here
  m_mainWindow->slotUpdateFractionDone(1.0);
  m_mainWindow->m_currentStep = 1;
  m_mainWindow->slotStatusMsg(i18n("Ready."));

  m_selectedEntries.clear();
  m_mainWindow->slotEntryCount();
  updateActions();

  connect(coll_, SIGNAL(signalGroupModified(Tellico::Data::Collection*, const Tellico::Data::EntryGroup*)),
          m_groupView, SLOT(slotModifyGroup(Tellico::Data::Collection*, const Tellico::Data::EntryGroup*)));

  connect(coll_, SIGNAL(signalFieldAdded(Tellico::Data::Collection*, Tellico::Data::Field*)),
          this, SLOT(slotFieldAdded(Tellico::Data::Collection*, Tellico::Data::Field*)));
  connect(coll_, SIGNAL(signalFieldDeleted(Tellico::Data::Collection*, Tellico::Data::Field*)),
          this, SLOT(slotFieldDeleted(Tellico::Data::Collection*, Tellico::Data::Field*)));
  connect(coll_, SIGNAL(signalFieldModified(Tellico::Data::Collection*, Tellico::Data::Field*, Tellico::Data::Field*)),
          this, SLOT(slotFieldModified(Tellico::Data::Collection*, Tellico::Data::Field*, Tellico::Data::Field*)));
  connect(coll_, SIGNAL(signalFieldsReordered(Tellico::Data::Collection*)),
          this, SLOT(slotFieldsReordered(Tellico::Data::Collection*)));
  connect(coll_, SIGNAL(signalRefreshField(Tellico::Data::Field*)),
          this, SLOT(slotRefreshField(Tellico::Data::Field*)));
}

void Controller::slotCollectionDeleted(Data::Collection* coll_) {
//  kdDebug() << "Controller::slotCollectionDeleted()" << endl;

  blockAllSignals(true);
  m_mainWindow->saveCollectionOptions(coll_);
  m_groupView->removeCollection(coll_);
  m_detailedView->removeCollection(coll_);
  m_viewStack->clear();
  blockAllSignals(false);

  // disconnect all signals from the collection to the controller
  // this is needed because the Collection::appendCollection() and mergeCollection()
  // functions signal collection deleted then added for the same collection
  disconnect(coll_, 0, this, 0);

//  ImageFactory::clean();
}

void Controller::slotCollectionRenamed(const QString& name_) {
  m_groupView->renameCollection(name_);
}

void Controller::slotEntryAdded(Data::Entry* entry_) {
// the group view gets called from the groupModified signal
  m_detailedView->addEntry(entry_);
  m_editDialog->slotUpdateCompletions(entry_);
  m_viewStack->iconView()->addEntry(entry_);
}

void Controller::slotEntryModified(Data::Entry* entry_) {
// the group view gets called from the groupModified signal
  m_detailedView->modifyEntry(entry_);
  m_editDialog->slotUpdateCompletions(entry_);
  m_viewStack->refresh();
}

void Controller::slotEntryDeleted(Data::Entry* entry_) {
// the group view gets called from the groupModified signal
  m_detailedView->removeEntry(entry_);
  m_editDialog->clear();
  if(entry_ == m_selectedEntries.getFirst()) {
    m_viewStack->entryView()->clear();
  }
  m_viewStack->iconView()->removeEntry(entry_);
}

void Controller::slotFieldAdded(Data::Collection* coll_, Data::Field* field_) {
  m_editDialog->setLayout(coll_);
  m_detailedView->addField(field_, 0); // hide by default
  m_viewStack->refresh();
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
}

void Controller::slotFieldDeleted(Data::Collection* coll_, Data::Field* field_) {
//  kdDebug() << "Controller::slotFieldDeleted() - " << field_->name() << endl;
  m_editDialog->removeField(field_);
  m_detailedView->removeField(field_);
  m_viewStack->refresh();
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
}

void Controller::slotFieldModified(Data::Collection* coll_, Data::Field* oldField_, Data::Field* newField_) {
  m_editDialog->slotUpdateField(coll_, oldField_, newField_);
  m_detailedView->modifyField(oldField_, newField_);
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
  m_viewStack->refresh();
}

void Controller::slotFieldsReordered(Data::Collection* coll_) {
  m_editDialog->setLayout(coll_);
  m_detailedView->reorderFields(coll_->fieldList());
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
  m_viewStack->refresh();
}

void Controller::slotUpdateSelection(QWidget*, const Data::Collection* coll_) {
  if(m_working) {
    return;
  }
  m_working = true;

  blockAllSignals(true);
  m_detailedView->clearSelection();
  m_editDialog->setContents(Data::EntryList());
  m_viewStack->showCollection(coll_);
  blockAllSignals(false);

  m_mainWindow->m_copyEntry->setEnabled(false);
  m_mainWindow->m_deleteEntry->setEnabled(false);
  m_working = false;
}

void Controller::slotUpdateSelection(QWidget*, const Data::EntryGroup* group_) {
  if(m_working) {
    return;
  }
  m_working = true;

  blockAllSignals(true);
  m_detailedView->clearSelection();
  m_editDialog->setContents(*group_);
  m_viewStack->showEntryGroup(group_);
  blockAllSignals(false);

  m_selectedEntries = *group_;
  updateActions();
  m_mainWindow->slotEntryCount();
  m_working = false;
}

void Controller::slotUpdateSelection(QWidget* widget_, const Data::EntryList& list_) {
  if(m_working) {
    return;
  }
  m_working = true;
//  kdDebug() << "Controller::slotUpdateSelection() entryList - " << list_.count() << endl;

  blockAllSignals(true);
// in the list view and group view, if entries are selected in one, clear selection in other
  if(widget_ != m_detailedView) {
    m_detailedView->clearSelection();
  }
  if(widget_ != m_groupView) {
    m_groupView->clearSelection();
  }
  if(widget_ != m_editDialog) {
    m_editDialog->setContents(list_);
  }
  // entry view only shows one
  if(widget_ != m_viewStack->iconView()) {
    m_viewStack->iconView()->clearSelection();
    if(widget_) {
      m_viewStack->showEntry(list_.getFirst());
    }
  }
  blockAllSignals(false);

  m_selectedEntries = list_;
  updateActions();
  m_mainWindow->slotEntryCount();
  m_working = false;
}

void Controller::slotUpdateSelection(Data::Entry* entry_, const QString& highlight_) {
  if(m_working) {
    return;
  }
//  kdDebug() << "Controller::slotUpdateSelection()" << endl;

  // first clear selection
//  slotUpdateSelection(0, Data::EntryList());

  blockAllSignals(true);
  m_working = true;
  m_detailedView->setEntrySelected(entry_);
  m_groupView->setEntrySelected(entry_);
  m_editDialog->setContents(entry_, highlight_);
  m_viewStack->showEntry(entry_);
  m_selectedEntries.clear();
  m_selectedEntries.append(entry_);
  updateActions();
  m_working = false;
  blockAllSignals(false);
}

void Controller::slotDeleteSelectedEntries() {
  if(m_selectedEntries.isEmpty()) {
    return;
  }

  m_working = true;
  // must iterate over a copy of the list
  Data::EntryList entriesToDelete = m_selectedEntries;
  Data::EntryListIterator it(entriesToDelete);
  // confirm delete
  if(m_selectedEntries.count() == 1) {
    QString str = i18n("Do you really want to delete this entry?");
    QString dontAsk = QString::fromLatin1("DeleteEntry");
    int ret = KMessageBox::questionYesNo(Kernel::self()->widget(), str, i18n("Delete Entry?"),
                                         KStdGuiItem::yes(), KStdGuiItem::no(), dontAsk);
    if(ret != KMessageBox::Yes) {
      return;
    }
  } else {
    QStringList names;
    for(it.toFirst(); it.current(); ++it) {
      names += it.current()->title();
    }
    QString str = i18n("Do you really want to delete these entries?");
    // historically called DeleteMultipleBooks, don't change
    QString dontAsk = QString::fromLatin1("DeleteMultipleBooks");
    int ret = KMessageBox::questionYesNoList(Kernel::self()->widget(), str, names,
                                             i18n("Delete Multiple Entries?"),
                                             KStdGuiItem::yes(), KStdGuiItem::no(), dontAsk);
    if(ret != KMessageBox::Yes) {
      return;
    }
  }

  for(it.toFirst(); it.current(); ++it) {
    Kernel::self()->doc()->slotDeleteEntry(it.current());
  }
  updateActions();

  m_working = false;

  // special case, the detailed list view selects the next item, so handle that
  Data::EntryList newList;
  for(QPtrListIterator<MultiSelectionListViewItem> it(m_detailedView->selectedItems()); it.current(); ++it) {
    newList.append(static_cast<EntryItem*>(it.current())->entry());
  }
  slotUpdateSelection(m_detailedView, newList);
}

void Controller::slotRefreshField(Data::Field* field_) {
  // group view only needs to refresh if it's the title
  if(field_->name() == Latin1Literal("title")) {
    m_groupView->populateCollection(Kernel::self()->collection());
  }
  m_detailedView->slotRefresh();
  m_viewStack->refresh();
}

void Controller::slotCopySelectedEntries() {
  if(m_selectedEntries.isEmpty()) {
    return;
  }

  // keep copy of selected entries
  Data::EntryList old = m_selectedEntries;

  // need to create copies
  Data::EntryList list;
  for(Data::EntryListIterator it(m_selectedEntries); it.current(); ++it) {
    list.append(new Data::Entry(*it.current()));
  }
  Kernel::self()->doc()->slotSaveEntries(list);
  slotUpdateSelection(0, old);
}

void Controller::blockAllSignals(bool block_) const {
  m_detailedView->blockSignals(block_);
  m_groupView->blockSignals(block_);
  m_editDialog->blockSignals(block_);
  m_viewStack->iconView()->blockSignals(block_);
}

void Controller::slotUpdateFilter(Filter* filter_) {
  // the view takes over ownership of the filter
  m_detailedView->clearSelection();
  m_selectedEntries.clear();
  updateActions();
  m_detailedView->setFilter(filter_);
  m_viewStack->iconView()->clearSelection();

  // since filter dialog isn't modal
  if(m_mainWindow->m_filterDlg) {
    if(filter_) {
      m_mainWindow->m_filterDlg->setFilter(filter_);
    } else {
      m_mainWindow->m_filterDlg->slotClear();
    }
  }
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
  m_mainWindow->m_newEntry->plug(widget_);
  m_mainWindow->m_editEntry->plug(widget_);
  m_mainWindow->m_copyEntry->plug(widget_);
  m_mainWindow->m_deleteEntry->plug(widget_);
}

void Controller::updateActions() const {
  bool emptySelection = m_selectedEntries.isEmpty();
  m_mainWindow->m_editEntry->setEnabled(!emptySelection);
  m_mainWindow->m_copyEntry->setEnabled(!emptySelection);
  m_mainWindow->m_deleteEntry->setEnabled(!emptySelection);

  if(m_selectedEntries.count() < 2) {
    m_mainWindow->m_editEntry->setText(i18n("&Edit Entry"));
    m_mainWindow->m_copyEntry->setText(i18n("&Copy Entry"));
    m_mainWindow->m_deleteEntry->setText(i18n("&Delete Entry"));
  } else {
    m_mainWindow->m_editEntry->setText(i18n("&Edit Entries"));
    m_mainWindow->m_copyEntry->setText(i18n("&Copy Entries"));
    m_mainWindow->m_deleteEntry->setText(i18n("&Delete Entries"));
  }
}

#include "controller.moc"
