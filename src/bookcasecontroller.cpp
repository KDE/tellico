/***************************************************************************
                            bookcasecontroller.h
                             -------------------
    begin                : Thu May 29 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bookcase.h"
#include "bookcasecontroller.h"
#include "bcgroupview.h"
#include "bcdetailedlistview.h"
#include "bcuniteditwidget.h"
#include "bccollection.h"
#include "bookcasedoc.h"

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

BookcaseController::BookcaseController(Bookcase* parent_, const char* name_)
    : QObject(parent_, name_), m_bookcase(parent_) {
}

void BookcaseController::setWidgets(BCGroupView* groupView_, BCDetailedListView* detailedView_,
                                    BCUnitEditWidget* editWidget_) {
  m_groupView = groupView_;
  m_detailedView = detailedView_;
  m_editWidget = editWidget_;
}

void BookcaseController::slotCollectionAdded(BCCollection* coll_) {
//  kdDebug() << "BookcaseController::slotAddCollection()" << endl;

  // do this first because the group view will need it later
  m_bookcase->readCollectionOptions(coll_);

  // these might take some time, change the status message
  // the detailed view also has half the progress bar
  // this slot gets called after the importer has loaded the collection
  // so first bump it to 100% of the step, inrcease the step, then load
  // the document view
  m_bookcase->slotUpdateFractionDone(1.0);
  m_bookcase->m_currentStep = m_bookcase->m_maxSteps;
  m_detailedView->addCollection(coll_);

  m_groupView->addCollection(coll_);

  m_editWidget->slotSetLayout(coll_);
  // The importer doesn't ever finish the importer, better to do it here
  m_bookcase->slotUpdateFractionDone(1.0);
  m_bookcase->m_currentStep = 1;
  m_bookcase->slotStatusMsg(i18n("Ready."));

  m_bookcase->slotUnitCount(0);
  m_selectedUnits.clear();

  connect(coll_, SIGNAL(signalGroupModified(BCCollection*, const BCUnitGroup*)),
          m_groupView, SLOT(slotModifyGroup(BCCollection*, const BCUnitGroup*)));

  connect(coll_, SIGNAL(signalAttributeAdded(BCCollection*, BCAttribute*)),
          this, SLOT(slotAttributeAdded(BCCollection*, BCAttribute*)));
  connect(coll_, SIGNAL(signalAttributeDeleted(BCCollection*, BCAttribute*)),
          this, SLOT(slotAttributeDeleted(BCCollection*, BCAttribute*)));
  connect(coll_, SIGNAL(signalAttributeModified(BCCollection*, BCAttribute*, BCAttribute*)),
          this, SLOT(slotAttributeModified(BCCollection*, BCAttribute*, BCAttribute*)));
  connect(coll_, SIGNAL(signalAttributesReordered(BCCollection*)),
          this, SLOT(slotAttributesReordered(BCCollection*)));
}

void BookcaseController::slotCollectionDeleted(BCCollection* coll_) {
//  kdDebug() << "BookcaseController::slotRemoveCollection()" << endl;

  m_bookcase->saveCollectionOptions(coll_);
  m_groupView->removeCollection(coll_);
  m_detailedView->removeCollection(coll_);
}

void BookcaseController::slotCollectionRenamed(const QString& name_) {
  m_groupView->renameCollection(name_);
}

void BookcaseController::slotUnitAdded(BCUnit* unit_) {
// the group view gets called from the groupModified signal
  m_detailedView->addItem(unit_);
  m_editWidget->slotUpdateCompletions(unit_);
}

void BookcaseController::slotUnitModified(BCUnit* unit_) {
// the group view gets called from the groupModified signal
  m_detailedView->modifyItem(unit_);
  m_editWidget->slotUpdateCompletions(unit_);
}

void BookcaseController::slotUnitDeleted(BCUnit* unit_) {
// the group view gets called from the groupModified signal
  m_detailedView->removeItem(unit_);
//  m_editWidget->slotUpdateCompletions(unit_);
  m_editWidget->slotHandleClear();
}

void BookcaseController::slotAttributeAdded(BCCollection* coll_, BCAttribute* att_) {
  m_editWidget->slotSetLayout(coll_);
  m_detailedView->addAttribute(att_);
  m_bookcase->slotUpdateCollectionToolBar(coll_);
}

void BookcaseController::slotAttributeDeleted(BCCollection* coll_, BCAttribute* att_) {
  m_editWidget->slotSetLayout(coll_);
  m_detailedView->removeAttribute(att_);
  m_bookcase->slotUpdateCollectionToolBar(coll_);
}

void BookcaseController::slotAttributeModified(BCCollection* coll_, BCAttribute* oldAtt_,
                                               BCAttribute* newAtt_) {
  m_editWidget->slotUpdateAttribute(coll_, oldAtt_, newAtt_);
  m_detailedView->modifyAttribute(oldAtt_, newAtt_);
  m_bookcase->slotUpdateCollectionToolBar(coll_);
}

void BookcaseController::slotAttributesReordered(BCCollection* coll_) {
  m_editWidget->slotSetLayout(coll_);
  m_detailedView->reorderAttributes(coll_->attributeList());
  m_bookcase->slotUpdateCollectionToolBar(coll_);
}

void BookcaseController::slotUpdateSelection(QWidget* widget_, const BCUnitList& list_) {
//  kdDebug() << "BookcaseController::slotUpdateSelection()" << endl;

// in the list view and group view, if units are selected in one, clear selection in other
  if(widget_ != m_detailedView) {
    m_detailedView->clearSelection();
  }
  if(widget_ != m_groupView) {
    m_groupView->clearSelection();
  }
  if(widget_ != m_editWidget) {
    m_editWidget->setContents(list_);
  }
  m_bookcase->slotUnitCount(list_.count());
  m_selectedUnits = list_;
}

void BookcaseController::slotUpdateSelection(BCUnit* unit_, const QString& highlight_) {
//  kdDebug() << "BookcaseController::slotUpdateSelection()" << endl;

  m_detailedView->setUnitSelected(unit_);
  m_groupView->setUnitSelected(unit_);
  m_editWidget->setContents(unit_, highlight_);
  m_selectedUnits.clear();
  m_selectedUnits.append(unit_);
}

void BookcaseController::slotDeleteSelectedUnits() {
  if(m_selectedUnits.isEmpty()) {
    return;
  }

  BCUnitListIterator it(m_selectedUnits);
  // add a message box if multiple items are to be deleted
  if(m_selectedUnits.count() > 1) {
    QStringList names;
    for(it.toFirst() ; it.current(); ++it) {
      names += it.current()->title();
    }
    QString str = i18n("Do you really want to delete these entries?");
    QString dontAsk = QString::fromLatin1("DeleteMultipleBooks");
    int ret = KMessageBox::questionYesNoList(m_bookcase, str, names, i18n("Delete Multiple Entries?"),
                                             KStdGuiItem::yes(), KStdGuiItem::no(), dontAsk);
    if(ret != KMessageBox::Yes) {
      return;
    }
  }

  for(it.toFirst() ; it.current(); ++it) {
    m_bookcase->doc()->slotDeleteUnit(it.current());
  }
  m_selectedUnits.clear();
  m_bookcase->slotUnitCount(m_selectedUnits.count());
}
