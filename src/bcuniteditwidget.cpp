/***************************************************************************
                            bcuniteditwidget.cpp
                             -------------------
    begin                : Wed Sep 26 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bcuniteditwidget.h"
#include "bccollection.h"
#include "bctabcontrol.h"

#include <kbuttonbox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kdeversion.h>
#if KDE_VERSION > 305
#include <kaccelmanager.h>
#endif

#include <qlayout.h>
#include <qstringlist.h>
#include <qpushbutton.h>
#include <qvaluevector.h>
#include <qvbox.h>
#include <qdatetime.h>

// must be an even number
static const int NCOLS = 2;

BCUnitEditWidget::BCUnitEditWidget(QWidget* parent_, const char* name_/*=0*/)
    : QWidget(parent_, name_), m_currColl(0),
      m_tabs(new BCTabControl(this)), m_modified(false), m_isSaving(false) {
//  kdDebug() << "BCUnitEditWidget()" << endl;
  m_widgetDict.setAutoDelete(true);
  QVBoxLayout* topLayout = new QVBoxLayout(this);

  topLayout->setSpacing(5);
  topLayout->setMargin(5);
  // stretch = 1 so that the tabs expand vertically
  topLayout->addWidget(m_tabs, 1);

  KButtonBox* bb = new KButtonBox(this);
  bb->addStretch();
  m_new = bb->addButton(i18n("New Book"), this, SLOT(slotHandleNew()));
  m_save = bb->addButton(i18n("Enter Book"), this, SLOT(slotHandleSave()));
  m_delete = bb->addButton(i18n("Delete Book"), this, SLOT(slotHandleDelete()));
//  m_clear = bb->addButton(i18n("Clear Data"), this, SLOT(slotHandleClear()));
  bb->addStretch();
  // stretch = 0, so the height of the buttonbox is constant
  topLayout->addWidget(bb, 0, Qt::AlignBottom | Qt::AlignHCenter);

  m_save->setEnabled(false);
  // no currUnit exists, so disable the Copy and Delete button
#ifdef SHOW_COPY_BTN
  m_copy = bb->addButton(i18n("Duplicate Book"), this, SLOT(slotHandleCopy()));
  m_copy->setEnabled(false);
#endif
  m_delete->setEnabled(false);

  m_isOrphan = false;
}

void BCUnitEditWidget::slotReset() {
//  kdDebug() << "BCUnitEditWidget::slotReset()" << endl;

  m_save->setEnabled(false);
  m_delete->setEnabled(false);
  m_save->setText(i18n("Enter Book"));
  m_currColl = 0;
  m_currUnits.clear();;

  m_modified = false;
  
  m_tabs->clear();
  m_widgetDict.setAutoDelete(false);
  m_widgetDict.clear();
  m_widgetDict.setAutoDelete(true);
}

void BCUnitEditWidget::slotSetLayout(BCCollection* coll_) {
  if(!coll_) {
    return;
  }
  
//  kdDebug() << "BCUnitEditWidget::slotSetLayout()" << endl;

  setUpdatesEnabled(false);
  if(m_tabs->count() > 0) {
//    kdDebug() << "BCUnitEditWidget::slotSetLayout() - resetting contents." << endl;
    slotReset();
  }

  m_currColl = coll_;
  
  int maxHeight = 0;

  QStringList catList = m_currColl->attributeCategories();
  QStringList::ConstIterator catIt;
  for(catIt = catList.begin(); catIt != catList.end(); ++catIt) {
    BCAttributeList list = m_currColl->attributesByCategory(*catIt);

    QWidget* page = new QWidget(m_tabs);
    // (parent, margin, spacing)
    QVBoxLayout* boxLayout = new QVBoxLayout(page, 0, 0);

    int nrows = (list.count()+1)/NCOLS;

    QWidget* grid = new QWidget(page);
    // (parent, nrows, ncols, margin, spacing)
    QGridLayout* layout = new QGridLayout(grid, nrows, NCOLS, 8, 0);
    boxLayout->addWidget(grid, 0);
    boxLayout->addStretch(1);

    // if a column has a line edit or text edit, then it should have a stretch factor
    QValueVector<bool> hasEdit(NCOLS, false);
    QValueVector<int> maxWidth(NCOLS, 0);
    
    BCAttributeWidget* widget;
    BCAttributeListIterator it(list);
    for(int count = 0; it.current(); ++it) {
      if(it.current()->type() == BCAttribute::ReadOnly) {
        continue; // ReadOnly attributes don't get widgets
      }
      widget = new BCAttributeWidget(it.current(), grid);
      connect(widget, SIGNAL(modified()), SLOT(slotSetModified()));

      layout->addWidget(widget, count/NCOLS, count%NCOLS);
      layout->setRowStretch(count/NCOLS, 1);

      m_widgetDict.insert(QString::number(m_currColl->id()) + it.current()->name(), widget);

      maxWidth[count%NCOLS] = QMAX(maxWidth[count%NCOLS], widget->labelWidth());
      if(widget->isTextEdit()) {
        hasEdit[count%NCOLS] = true;
      }
      widget->updateGeometry();
      if(it.current()->type() != BCAttribute::Para) {
        maxHeight = QMAX(maxHeight, widget->sizeHint().height());
      }
      ++count;
    }
    
    // now, the labels in a column should all be the same width
    it.toFirst();
    for(int count = 0; it.current(); ++it) {
      widget = m_widgetDict[QString::number(m_currColl->id()) + it.current()->name()];
      if(widget) {
        widget->setLabelWidth(maxWidth[count%NCOLS]);
        layout->addRowSpacing(count/NCOLS, maxHeight);
        ++count;
      }
    }

    // update stretch factors for columns with a line edit
    for(int col = 0; col < NCOLS; ++col) {
      if(hasEdit[col]) {
        layout->setColStretch(col, 1);
      }
    }
    
    // I don't want anything to be hidden
    grid->setMinimumHeight(grid->sizeHint().height());
    m_tabs->addTab(page, *catIt);
  }

    // update keyboard accels
#if KDE_VERSION > 305
    KAcceleratorManager::manage(m_tabs);
#endif

  setUpdatesEnabled(true);
// this doesn't seem to work
//  setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));
// so do this instead
  layout()->invalidate(); // needed so the sizeHint() gets recalculated
  setFixedHeight(sizeHint().height());
  updateGeometry();

  m_tabs->setCurrentPage(0);
  
  slotHandleNew();
  m_modified = false; // because the year is inserted
}

//void BCUnitEditWidget::keyReleaseEvent(QKeyEvent* ev_) {
//  if(!m_completionActivated
//      && (ev_->key() == Qt::Key_Return
//      || ev_->key() == Qt::Key_Enter)) {
//    slotHandleReturn();
//    ev_->accept();
//  } else {
//    ev_->ignore();
//  }
//}

void BCUnitEditWidget::slotHandleReturn() {
  kdDebug() << "BCUnitEditWidget::slotHandleReturn()" << endl;
  slotHandleSave();
}

void BCUnitEditWidget::slotHandleNew() {
  if(!m_currColl || !queryModified()) {
    return;
  }
//  kdDebug() << "BCUnitEditWidget::slotHandleNew()" << endl;
  slotHandleClear();

  BCUnit* unit = new BCUnit(m_currColl);
  m_currUnits.append(unit);
  m_isOrphan = true;

// special case for purchase date, insert current year
//  QString key = QString::number(m_currColl->id()) + QString::fromLatin1("pur_date");
//  BCAttributeWidget* widget = m_widgetDict.find(key);
//  if(widget) {
//    QDate now = QDate::currentDate();
//    widget->setText(QString::number(now.year()));
//    m_modified = false;
//  }
}

void BCUnitEditWidget::slotHandleCopy() {
  BCUnit* unit = new BCUnit(*m_currUnits.getFirst());
  slotHandleSave();
  slotHandleClear();
  m_isOrphan = true;
  slotSetContents(unit);
}

void BCUnitEditWidget::slotHandleSave() {
//  kdDebug() << "BCUnitEditWidget::slotHandleSave()" << endl;
  if(!m_currColl) {
    // big problem
    kdDebug() << "BCUnitEditWidget::slotHandleSave() - no valid collection pointer" << endl;
    return;
  }

  m_isSaving = true;
  
  if(m_currUnits.isEmpty()) {
    m_currUnits.append(new BCUnit(m_currColl));
    m_isOrphan = true;
  }

  // add a message box if multiple items are selected
  if(m_currUnits.count() > 1) {
    QStringList names;
    BCUnitListIterator uIt(m_currUnits);
    for( ; uIt.current(); ++uIt) {
      names += uIt.current()->title();
    }
    QString str(i18n("Do you really want to modify these books?"));
    QString dontAsk = QString::fromLatin1("SaveMultipleBooks");
    int ret = KMessageBox::questionYesNoList(this, str, names, i18n("Modify Multiple Books?"),
                                             KStdGuiItem::yes(), KStdGuiItem::no(), dontAsk);
    if(ret != KMessageBox::Yes) {
      return;
    }
  }

  BCAttributeListIterator aIt(m_currColl->attributeList());
  BCAttributeWidget* widget;
  QString temp, key;

  BCUnitListIterator uIt(m_currUnits);
  for( ; uIt.current(); ++uIt) {
    // boolean to keep track if every possible attribute is empty
    bool empty = true;
  
    for(aIt.toFirst(); aIt.current(); ++aIt) {
      key = QString::number(m_currColl->id()) + aIt.current()->name();
      widget = m_widgetDict.find(key);
      if(widget && widget->isEnabled()) {
        temp = widget->text();
        // ok to set attribute empty string, just not all of them
        uIt.current()->setAttribute(aIt.current()->name(), temp);
        if(!temp.isEmpty()) {
          empty = false;
        }
      }
    }

    // if something was not empty, signal a save
    if(!empty) {
      m_isOrphan = false;
      emit signalSaveUnit(uIt.current());
    }
  }
  m_modified = false;
  m_isSaving = false;
  slotHandleNew();
}

void BCUnitEditWidget::slotHandleDelete() {
  // add a message box if multiple items are to be deleted
  if(m_currUnits.count() > 1) {
    QStringList names;
    BCUnitListIterator uIt(m_currUnits);
    for( ; uIt.current(); ++uIt) {
      names += uIt.current()->title();
    }
    QString str(i18n("Do you really want to delete these books?"));
    QString dontAsk = QString::fromLatin1("DeleteMultipleBooks");
    int ret = KMessageBox::questionYesNoList(this, str, names, i18n("Delete Multiple Books?"),
                                             KStdGuiItem::yes(), KStdGuiItem::no(), dontAsk);
    if(ret != KMessageBox::Yes) {
      return;
    }
  }

  BCUnit* unit;
  BCUnitListIterator it(m_currUnits);
  while(it.current()) {
    unit = it.current();
    ++it;
    // only emit signal if the parent collection contains the unit in its list
    if(unit->isOwned()) {
//      kdDebug() << "BCUnitEditWidget::slotHandleDelete() - deleting " << unit->title() << endl;
      m_currUnits.removeRef(unit);
      emit signalDeleteUnit(unit);
    } else {
//      kdDebug() << "BCUnitEditWidget::slotHandleDelete() - " << unit->title() << " is not owned" << endl;
    }
  }

  // clear the widget whether or not anything was deleted
  slotHandleNew();
}

void BCUnitEditWidget::slotHandleClear() {
//  kdDebug() << "BCUnitEditWidget::slotHandleClear()" << endl;

  // clear the widgets
  QDictIterator<BCAttributeWidget> it(m_widgetDict);
  for( ; it.current(); ++it) {
    it.current()->setEnabled(true);
    it.current()->clear();
  }

  if(m_isOrphan) {
    if(m_currUnits.count() > 1) {
      kdWarning() << "BCUnitEditWidget::slotHandleClear() - is an orphan, but more than one" << endl;
    }
    delete m_currUnits.getFirst();
    m_isOrphan = false;
  }
  m_currUnits.clear();

  // disable the copy and delete buttons
#ifdef SHOW_COPY_BTN
  m_copy->setEnabled(false);
#endif
  m_delete->setEnabled(false);
  m_save->setText(i18n("Enter Book"));
  m_save->setEnabled(false);

  m_modified = false;
}

void BCUnitEditWidget::slotSetContents(const BCUnitList& list_) {
  // this slot might get called if we try to save multiple items, so just return
  if(m_isSaving) {
    return;
  }

  if(list_.isEmpty()) {
    if(!m_isOrphan && m_modified) {
      slotHandleNew();
    }
    return;
  }

//  kdDebug() << "BCUnitEditWidget::slotSetContents() - " << list_.count() << " units" << endl;
  
  // first set contents to first item
  slotSetContents(list_.getFirst());
  // something weird...if list count can actually be 1 before the slotSetContents call
  // and 0 after it. Why is that? It's const!
  if(list_.count() < 2) {
    return;
  }

  // keep track of the units
  m_currUnits = list_;

  blockSignals(true);

  QString key;
  BCAttributeWidget* widget;
  // TODO: fix for multiple collections
  BCUnitListIterator uIt(list_);
  BCAttributeListIterator aIt(m_currColl->attributeList());
  for( ; aIt.current(); ++aIt) {
    key = QString::number(m_currColl->id()) + aIt.current()->name();
    widget = m_widgetDict.find(key);
    if(!widget) { // probably read-only
      continue;
    }
    widget->editMultiple(true);
    
    QString value = list_.getFirst()->attribute(aIt.current()->name());
    for(++uIt; uIt.current(); ++uIt) { // skip checking the first one
      if(uIt.current()->attribute(aIt.current()->name()) != value) {
        widget->setEnabled(false);
        break;
      }
    }
    uIt.toFirst();
  } // end attribute loop

  blockSignals(false);
  
  m_save->setText(i18n("Modify Books"));
  m_delete->setText(i18n("Delete Books"));
}

void BCUnitEditWidget::slotSetContents(BCUnit* unit_, const QString& highlight_/*empty*/) {
  bool ok = queryModified();
  if(!ok) {
    return;
  }
  
  if(!unit_) {
    kdDebug() << "BCUnitEditWidget::slotSetContents() - null unit pointer" << endl;
    slotHandleNew();
    return;
  }

 // kdDebug() << "BCUnitEditWidget::slotSetContents() - " << unit_->title() << endl;
  slotHandleClear();
  m_currUnits.append(unit_);

  // I'll end up nulling the highlight string, so make a copy
  QString highlight = highlight_;
  
  if(m_currColl != unit_->collection()) {
    kdDebug() << "BCUnitEditWidget::slotSetContents() - collections don't match" << endl;
    m_currColl = unit_->collection();
  }
  
  //disable save button
//  m_save->setEnabled(false);
//  // enable copy and delete buttons
//#ifdef SHOW_COPY_BTN
//  m_copy->setEnabled(true);
//#endif
  m_delete->setEnabled(true);

//  m_tabs->showTab(0);

  slotSetModified(false);

  int highlightPage = -1;
  QString key;
  BCAttributeWidget* widget;    
  BCAttributeListIterator it(m_currColl->attributeList());
  for( ; it.current(); ++it) {
    key = QString::number(m_currColl->id()) + it.current()->name();
    widget = m_widgetDict.find(key);
    if(!widget) { // is probably read-only
      continue;
    }

    QString value = unit_->attribute(it.current()->name());
    widget->setText(value);
    if(!highlight.isEmpty() && value.lower().contains(highlight.lower())) {
      widget->setHighlighted(highlight);
      // now I only want to worry about first string found, so empty it
      highlight = QString::null;
      highlightPage = m_tabs->indexOf(static_cast<QWidget*>(widget->parent()));
    }
    widget->setEnabled(true);
    widget->editMultiple(false);
  } // end attribute loop
  
  if(unit_->isOwned()) {
    m_save->setText(i18n("Modify Book"));
    m_save->setEnabled(m_modified);
  } else {
    slotSetModified(true);
  }
  m_delete->setText(i18n("Delete Book"));
  if(highlightPage > -1) {
    m_tabs->setCurrentPage(highlightPage);
  }
}

void BCUnitEditWidget::slotUpdateCompletions(BCUnit* unit_) {
  if(m_currColl != unit_->collection()) {
    kdDebug() << "BCUnitEditWidget::slotUpdateCompletions - inconsistent collection pointers!" << endl;
    m_currColl = unit_->collection();
  }
  
  BCAttributeWidget* widget;
  QString key, value;
  BCAttributeListIterator it(m_currColl->attributeList());
  for( ; it.current(); ++it) {
    if(it.current()->type() == BCAttribute::Line
       && it.current()->flags() & BCAttribute::AllowCompletion) {
      key = QString::number(m_currColl->id()) + it.current()->name();
      widget = m_widgetDict.find(key);
      if(widget) {
        value = unit_->attribute(it.current()->name());
        widget->addCompletionObjectItem(value);
      }
    }
  }
}

void BCUnitEditWidget::slotSetModified(bool mod_/*=true*/) {
  m_modified = mod_;
  m_save->setEnabled(mod_);
}

bool BCUnitEditWidget::queryModified() {
//  kdDebug() << "BCUnitEditWidget::queryModified() - modified is " << (m_modified?"true":"false") << endl;
  bool ok = true;
  if(m_modified) {
    QString str(i18n("The current book has been modified.\n"
                      "Do you want to enter the changes?"));
    int want_save = KMessageBox::warningYesNoCancel(this, str, i18n("Warning!"),
                                                    i18n("Enter Book"),
                                                    KStdGuiItem::discard());
    switch(want_save) {
      case KMessageBox::Yes:
        slotHandleSave();
        ok = true;
        break;

      case KMessageBox::No:
        m_modified = false;
        ok = true;
        break;

      case KMessageBox::Cancel:
        ok = false;
        break;
    }
  }
  return ok;
}

void BCUnitEditWidget::slotUpdateAttribute(BCCollection* coll_, BCAttribute* att_) {
  if(coll_ != m_currColl) {
    kdDebug() << "BCUnitEditWidget::slotUpdateAttribute() - wrong collection pointer!" << endl;
    m_currColl = coll_;
  }
  QString key = QString::number(coll_->id()) + att_->name();
  BCAttributeWidget* widget = m_widgetDict[key];
  if(widget) {
    widget->updateAttribute(att_);
  }
}
