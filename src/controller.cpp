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
#include "entryview.h"
#include "imagefactory.h"

#include "collection.h"
#include "document.h"

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kaction.h>

using Bookcase::Controller;

Controller::Controller(MainWindow* parent_, const char* name_)
    : QObject(parent_, name_), m_mainWindow(parent_) {
}

void Controller::setWidgets(GroupView* groupView_, DetailedListView* detailedView_,
                            EntryEditDialog* editDialog_, EntryView* entryView_) {
  m_groupView = groupView_;
  m_detailedView = detailedView_;
  m_editDialog = editDialog_;
  m_entryView = entryView_;
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

  m_mainWindow->slotEntryCount(0);
  m_selectedEntries.clear();

  connect(coll_, SIGNAL(signalGroupModified(Bookcase::Data::Collection*, const Bookcase::Data::EntryGroup*)),
          m_groupView, SLOT(slotModifyGroup(Bookcase::Data::Collection*, const Bookcase::Data::EntryGroup*)));

  connect(coll_, SIGNAL(signalFieldAdded(Bookcase::Data::Collection*, Bookcase::Data::Field*)),
          this, SLOT(slotFieldAdded(Bookcase::Data::Collection*, Bookcase::Data::Field*)));
  connect(coll_, SIGNAL(signalFieldDeleted(Bookcase::Data::Collection*, Bookcase::Data::Field*)),
          this, SLOT(slotFieldDeleted(Bookcase::Data::Collection*, Bookcase::Data::Field*)));
  connect(coll_, SIGNAL(signalFieldModified(Bookcase::Data::Collection*, Bookcase::Data::Field*, Bookcase::Data::Field*)),
          this, SLOT(slotFieldModified(Bookcase::Data::Collection*, Bookcase::Data::Field*, Bookcase::Data::Field*)));
  connect(coll_, SIGNAL(signalFieldsReordered(Bookcase::Data::Collection*)),
          this, SLOT(slotFieldsReordered(Bookcase::Data::Collection*)));
  connect(coll_, SIGNAL(signalRefreshAttribute(Bookcase::Data::Field*)),
          this, SLOT(slotRefreshField(Bookcase::Data::Field*)));
}

void Controller::slotCollectionDeleted(Data::Collection* coll_) {
//  kdDebug() << "Controller::slotCollectionDeleted()" << endl;

  blockAllSignals(true);
  m_mainWindow->saveCollectionOptions(coll_);
  m_groupView->removeCollection(coll_);
  m_detailedView->removeCollection(coll_);
  m_entryView->clear();
  blockAllSignals(false);

//  ImageFactory::clean();
}

void Controller::slotCollectionRenamed(const QString& name_) {
  m_groupView->renameCollection(name_);
}

void Controller::slotEntryAdded(Data::Entry* entry_) {
// the group view gets called from the groupModified signal
  m_detailedView->addEntry(entry_);
  m_editDialog->slotUpdateCompletions(entry_);
  m_entryView->refresh();
}

void Controller::slotEntryModified(Data::Entry* entry_) {
// the group view gets called from the groupModified signal
  m_detailedView->modifyEntry(entry_);
  m_editDialog->slotUpdateCompletions(entry_);
  m_entryView->refresh();
}

void Controller::slotEntryDeleted(Data::Entry* entry_) {
// the group view gets called from the groupModified signal
  m_detailedView->removeEntry(entry_);
  m_editDialog->clear();
  m_entryView->clear();
}

void Controller::slotFieldAdded(Data::Collection* coll_, Data::Field* field_) {
  m_editDialog->setLayout(coll_);
  m_detailedView->addField(field_, 0); // hide by default
  m_entryView->refresh();
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
}

void Controller::slotFieldDeleted(Data::Collection* coll_, Data::Field* field_) {
  m_editDialog->removeField(field_);
  m_detailedView->removeField(field_);
  m_entryView->refresh();
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
}

void Controller::slotFieldModified(Data::Collection* coll_, Data::Field* oldField_, Data::Field* newField_) {
  m_editDialog->slotUpdateField(coll_, oldField_, newField_);
  m_detailedView->modifyField(oldField_, newField_);
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
  m_entryView->refresh();
}

void Controller::slotFieldsReordered(Data::Collection* coll_) {
  m_editDialog->setLayout(coll_);
  m_detailedView->reorderFields(coll_->fieldList());
  m_mainWindow->slotUpdateCollectionToolBar(coll_);
  m_entryView->refresh();
}

void Controller::slotUpdateSelection(QWidget* widget_, const Data::EntryList& list_) {
//  kdDebug() << "Controller::slotUpdateSelection() - " << list_.count() << endl;

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
  blockAllSignals(false);
  // entry view only shows one
  m_entryView->showEntry(list_.getFirst());

  if(list_.isEmpty()) {
    m_mainWindow->m_editCopyEntry->setEnabled(false);
    m_mainWindow->m_editDeleteEntry->setEnabled(false);
  } else {
    m_mainWindow->m_editCopyEntry->setEnabled(true);
    m_mainWindow->m_editDeleteEntry->setEnabled(true);
  }
  m_mainWindow->slotEntryCount(list_.count());
  m_selectedEntries = list_;
}

void Controller::slotUpdateSelection(Data::Entry* entry_, const QString& highlight_) {
//  kdDebug() << "Controller::slotUpdateSelection()" << endl;

  slotUpdateSelection(0, Data::EntryList());
  m_detailedView->setEntrySelected(entry_);
  m_groupView->setEntrySelected(entry_);
  m_editDialog->setContents(entry_, highlight_);
  m_entryView->showEntry(entry_);
  m_selectedEntries.clear();
  m_selectedEntries.append(entry_);
}

void Controller::slotDeleteSelectedEntries() {
  if(m_selectedEntries.isEmpty()) {
    return;
  }

  Data::EntryListIterator it(m_selectedEntries);
  // add a message box if multiple items are to be deleted
  if(m_selectedEntries.count() > 1) {
    QStringList names;
    for(it.toFirst(); it.current(); ++it) {
      names += it.current()->title();
    }
    QString str = i18n("Do you really want to delete these entries?");
    QString dontAsk = QString::fromLatin1("DeleteMultipleBooks");
    int ret = KMessageBox::questionYesNoList(m_mainWindow, str, names, i18n("Delete Multiple Entries?"),
                                             KStdGuiItem::yes(), KStdGuiItem::no(), dontAsk);
    if(ret != KMessageBox::Yes) {
      return;
    }
  }

  for(it.toFirst(); it.current(); ++it) {
    m_mainWindow->doc()->slotDeleteEntry(it.current());
  }
  m_selectedEntries.clear();
  slotUpdateSelection(0, m_selectedEntries);
}

void Controller::slotRefreshField(Data::Field* field_) {
  // group view only needs to refresh if it's the title
  if(field_->name() == QString::fromLatin1("title")) {
    m_groupView->populateCollection(m_mainWindow->doc()->collection());
  }
  m_detailedView->slotRefresh();
  m_entryView->refresh();
}

void Controller::slotCopySelectedEntries() {
  if(m_selectedEntries.isEmpty()) {
    return;
  }

  // need a copy because the selected list changes
  Data::EntryList list = m_selectedEntries;
  for(Data::EntryListIterator it(list); it.current(); ++it) {
    m_mainWindow->doc()->slotSaveEntry(new Data::Entry(*it.current()));
  }
  m_selectedEntries = list;
  slotUpdateSelection(0, Data::EntryList());
}

void Controller::blockAllSignals(bool block_) {
  m_detailedView->blockSignals(block_);
  m_groupView->blockSignals(block_);
  m_editDialog->blockSignals(block_);
}
