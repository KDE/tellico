/***************************************************************************
    copyright            : (C) 2001-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "entryeditdialog.h"
#include "tabcontrol.h"
#include "collection.h"

#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kdeversion.h>
#if KDE_VERSION > 309
#include <kaccelmanager.h>
#include <qtabbar.h>
#endif

#include <qlayout.h>
#include <qstringlist.h>
#include <qpushbutton.h>
#include <qvaluevector.h>
#include <qvbox.h>
#include <qobjectlist.h>

// must be an even number
static const int NCOLS = 2; // number of columns of FieldWidgets

using Bookcase::EntryEditDialog;

EntryEditDialog::EntryEditDialog(QWidget* parent_, const char* name_)
    : KDialogBase(parent_, name_, false, i18n("Edit Entry"), User1|User2|Close, User1, false,
                  KStdGuiItem::save(), KGuiItem(i18n("&New Entry"))),
      m_currColl(0),
      m_tabs(new TabControl(this)),
      m_modified(false),
      m_isSaving(false) {
  setMainWidget(m_tabs);

  enableButton(User1, false);

  connect(this, SIGNAL(user1Clicked()), SLOT(slotHandleSave()));
  connect(this, SIGNAL(user2Clicked()), SLOT(slotHandleNew()));

  m_isOrphan = false;

  resize(configDialogSize(QString::fromLatin1("Edit Dialog Options")));
}

void EntryEditDialog::slotClose() {
  hide();
}

void EntryEditDialog::slotReset() {
//  kdDebug() << "EntryEditDialog::slotReset()" << endl;

  enableButton(User1, false);
  setButtonText(User1, i18n("Sa&ve Entry"));
  m_currColl = 0;
  m_currEntries.clear();;

  m_modified = false;

  m_tabs->clear(); // FieldWidgets get deleted here
  m_widgetDict.clear();
}

void EntryEditDialog::setLayout(Data::Collection* coll_) {
  if(!coll_) {
    return;
  }

//  kdDebug() << "EntryEditDialog::setLayout()" << endl;

  actionButton(User2)->setIconSet(KGlobal::iconLoader()->loadIcon(coll_->entryName(), KIcon::User));

  setUpdatesEnabled(false);
  if(m_tabs->count() > 0) {
//    kdDebug() << "EntryEditDialog::setLayout() - resetting contents." << endl;
    slotReset();
  }

  m_currColl = coll_;
  FieldWidget::s_coll = coll_;

  int maxHeight = 0;
  QPtrList<QWidget> gridList;
  
  QStringList catList = m_currColl->fieldCategories();
  QStringList::ConstIterator catIt;
  for(catIt = catList.begin(); catIt != catList.end(); ++catIt) {
    Data::FieldList list = m_currColl->fieldsByCategory(*catIt);

    // if this layout model is changed, be sure to check slotUpdateField()
    QWidget* page = new QWidget(m_tabs);
    // (parent, margin, spacing)
    QVBoxLayout* boxLayout = new QVBoxLayout(page, 0, 0);

    QWidget* grid = new QWidget(page);
    gridList.append(grid);
    // (parent, nrows, ncols, margin, spacing)
    QGridLayout* layout = new QGridLayout(grid, 0, NCOLS, 8, 0);

    boxLayout->addWidget(grid, 0);
    // those with multiple, get a stretch
    if(list.count() > 1 || !list.getFirst()->isSingleCategory()) {
      boxLayout->addStretch(1);
    }

    // keep track of which should expand
    QValueVector<bool> expands(NCOLS, false);
    QValueVector<int> maxWidth(NCOLS, 0);

    FieldWidget* widget;
    Data::FieldListIterator it(list);
    for(int count = 0; it.current(); ++it) {
      // ReadOnly and Dependent fields don't get widgets
      if(it.current()->type() == Data::Field::ReadOnly
         || it.current()->type() == Data::Field::Dependent) {
        continue;
      }
      
      widget = new FieldWidget(it.current(), grid);
      connect(widget, SIGNAL(modified()), SLOT(slotSetModified()));

      layout->addWidget(widget, count/NCOLS, count%NCOLS);
      layout->setRowStretch(count/NCOLS, 1);

      m_widgetDict.insert(QString::number(m_currColl->id()) + it.current()->name(), widget);

      maxWidth[count%NCOLS] = QMAX(maxWidth[count%NCOLS], widget->labelWidth());
      if(widget->expands()) {
        expands[count%NCOLS] = true;
      }
      widget->updateGeometry();
      if(!it.current()->isSingleCategory()) {
        maxHeight = QMAX(maxHeight, widget->minimumSizeHint().height());
      }
      ++count;
    }

    // now, the labels in a column should all be the same width
    it.toFirst();
    for(int count = 0; it.current(); ++it) {
      widget = m_widgetDict[QString::number(m_currColl->id()) + it.current()->name()];
      if(widget) {
        widget->setLabelWidth(maxWidth[count%NCOLS]);
        ++count;
      }
    }

    // update stretch factors for columns with a line edit
    for(int col = 0; col < NCOLS; ++col) {
      if(expands[col]) {
        layout->setColStretch(col, 1);
      }
    }

    // I don't want anything to be hidden
    //grid->setMinimumHeight(grid->sizeHint().height());

    m_tabs->addTab(page, *catIt);
  }

  // Now, go through and set all the field widgets to the same height
  for(QPtrListIterator<QWidget> it(gridList); it.current(); ++it) {
    QGridLayout* l = static_cast<QGridLayout*>(it.current()->layout());
    for(int row = 0; row < l->numRows() && it.current()->children()->count() > 1; ++row) {
      l->addRowSpacing(row, maxHeight);
    }
  }

  setUpdatesEnabled(true);
// this doesn't seem to work
//  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
// so do this instead
  layout()->invalidate(); // needed so the sizeHint() gets recalculated
  m_tabs->setMinimumHeight(m_tabs->minimumSizeHint().height());
  m_tabs->setMinimumWidth(m_tabs->sizeHint().width());

  // update keyboard accels
#if KDE_VERSION > 309
  // only want to manage tabBar(), but KDE bug 71769 means the parent widget must be used
  KAcceleratorManager::manage(m_tabs->tabBar()->parentWidget());
#endif

  m_tabs->setCurrentPage(0);
  
  slotHandleNew();
  m_modified = false; // because the year is inserted
}

void EntryEditDialog::slotHandleNew() {
  if(!m_currColl || !queryModified()) {
    return;
  }
//  kdDebug() << "EntryEditDialog::slotHandleNew()" << endl;

  m_tabs->setResetFocus(true);
  m_tabs->setCurrentPage(0);
  clear();
  emit signalClearSelection(this);

  Data::Entry* unit = new Data::Entry(m_currColl);
  m_currEntries.append(unit);
  m_isOrphan = true;

// special case for purchase date, insert current year
//  QString key = QString::number(m_currColl->id()) + QString::fromLatin1("pur_date");
//  FieldWidget* widget = m_widgetDict.find(key);
//  if(widget) {
//    QDate now = QDate::currentDate();
//    widget->setText(QString::number(now.year()));
//    m_modified = false;
//  }
}

void EntryEditDialog::slotHandleSave() {
//  kdDebug() << "EntryEditDialog::slotHandleSave()" << endl;
  if(!m_currColl) {
    // big problem
    kdDebug() << "EntryEditDialog::slotHandleSave() - no valid collection pointer" << endl;
    return;
  }

  m_isSaving = true;

  if(m_currEntries.isEmpty()) {
    m_currEntries.append(new Data::Entry(m_currColl));
    m_isOrphan = true;
  }

  // add a message box if multiple items are selected
  if(m_currEntries.count() > 1) {
    QStringList names;
    for(Data::EntryListIterator uIt(m_currEntries); uIt.current(); ++uIt) {
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

  Data::FieldListIterator fIt(m_currColl->fieldList());
  for(Data::EntryListIterator uIt(m_currEntries); uIt.current(); ++uIt) {
    // boolean to keep track if every possible field is empty
    bool empty = true;

    for(fIt.toFirst(); fIt.current(); ++fIt) {
      QString key = QString::number(m_currColl->id()) + fIt.current()->name();
      FieldWidget* widget = m_widgetDict.find(key);
      if(widget && widget->isEnabled()) {
        QString temp = widget->text();
        // ok to set field empty string, just not all of them
        uIt.current()->setField(fIt.current()->name(), temp);
        if(!temp.isEmpty()) {
          empty = false;
        }
      }
    }

    // if something was not empty, signal a save
    if(!empty) {
      m_isOrphan = false;
      emit signalSaveEntry(uIt.current());
    }
  }
  m_modified = false;
  m_isSaving = false;
  enableButton(User1, false);
//  slotHandleNew();
}

void EntryEditDialog::clear() {
//  kdDebug() << "EntryEditDialog::clear()" << endl;

  // clear the widgets
  for(QDictIterator<FieldWidget> it(m_widgetDict); it.current(); ++it) {
    it.current()->setEnabled(true);
    it.current()->clear();
  }

  if(m_isOrphan) {
    if(m_currEntries.count() > 1) {
      kdWarning() << "EntryEditDialog::clear() - is an orphan, but more than one" << endl;
    }
    delete m_currEntries.getFirst();
    m_isOrphan = false;
  }
  m_currEntries.clear();

  setButtonText(User1, i18n("Sa&ve Entry"));
  enableButton(User1, false);

  m_modified = false;
}

void EntryEditDialog::setContents(const Data::EntryList& list_) {
  // this slot might get called if we try to save multiple items, so just return
  if(m_isSaving) {
    return;
  }

  if(list_.isEmpty()) {
//    kdDebug() << "EntryEditDialog::setContents() - empty list" << endl;
    if(queryModified()) {
      blockSignals(true);
      slotHandleNew();
      blockSignals(false);
    }
    return;
  }

//  kdDebug() << "EntryEditDialog::setContents() - " << list_.count() << " entries" << endl;

  // first set contents to first item
  setContents(list_.getFirst());
  // something weird...if list count can actually be 1 before the setContents call
  // and 0 after it. Why is that? It's const!
  if(list_.count() < 2) {
    return;
  }

  // keep track of the units
  m_currEntries = list_;

  blockSignals(true);

  Data::EntryListIterator uIt(list_);
  for(Data::FieldListIterator fIt(m_currColl->fieldList()); fIt.current(); ++fIt) {
    QString key = QString::number(m_currColl->id()) + fIt.current()->name();
    FieldWidget* widget = m_widgetDict.find(key);
    if(!widget) { // probably read-only
      continue;
    }
    widget->editMultiple(true);

    QString value = list_.getFirst()->field(fIt.current()->name());
    for(++uIt; uIt.current(); ++uIt) { // skip checking the first one
      if(uIt.current()->field(fIt.current()->name()) != value) {
        widget->setEnabled(false);
        break;
      }
    }
    uIt.toFirst();
  } // end field loop

  blockSignals(false);

  setButtonText(User1, i18n("Sa&ve Entries"));
}

void EntryEditDialog::setContents(Data::Entry* entry_, const QString& highlight_/*empty*/) {
  bool ok = queryModified();
  if(!ok) {
    return;
  }

  if(!entry_) {
    kdDebug() << "EntryEditDialog::setContents() - null entry pointer" << endl;
    slotHandleNew();
    return;
  }

  m_tabs->setResetFocus(false);

 // kdDebug() << "EntryEditDialog::setContents() - " << entry_->title() << endl;
  blockSignals(true);
  clear();
  blockSignals(false);
  m_currEntries.append(entry_);

  // I'll end up nulling the highlight string, so make a copy
  QString highlight = highlight_;

  if(m_currColl != entry_->collection()) {
    kdDebug() << "EntryEditDialog::setContents() - collections don't match" << endl;
    m_currColl = entry_->collection();
  }

//  m_tabs->showTab(0);

  slotSetModified(false);

  int highlightPage = -1;
  for(Data::FieldListIterator it(m_currColl->fieldList()); it.current(); ++it) {
    QString key = QString::number(m_currColl->id()) + it.current()->name();
    FieldWidget* widget = m_widgetDict.find(key);
    if(!widget) { // is probably read-only
      continue;
    }

    QString value = entry_->field(it.current()->name());
    widget->setText(value);
    if(!highlight.isEmpty() && value.lower().contains(highlight.lower())) {
      widget->setHighlighted(highlight);
      // now I only want to worry about first string found, so empty it
      highlight = QString::null;
      highlightPage = m_tabs->indexOf(static_cast<QWidget*>(widget->parent()));
    }
    widget->setEnabled(true);
    widget->editMultiple(false);
  } // end field loop

  if(entry_->isOwned()) {
    setButtonText(User1, i18n("Sa&ve Entry"));
    enableButton(User1, m_modified);
  } else {
    slotSetModified(true);
  }
  if(highlightPage > -1) {
    m_tabs->setCurrentPage(highlightPage);
  }
}

void EntryEditDialog::removeField(Data::Field* field_) {
  if(!field_) {
    return;
  }

//  kdDebug() << "EntryEditDialog::removeField - name = " << field_->name() << endl;
  QString key = QString::number(m_currColl->id()) + field_->name();
  FieldWidget* widget = m_widgetDict.find(key);
  if(widget) {
    m_widgetDict.remove(key); // auto deleted
    // if this is the last field in the category, need to remove the tab page
    // this function is called after the field has been removed from the collection,
    // so the category should be gone from the category list
    if(m_currColl->fieldCategories().findIndex(field_->category()) == -1) {
      kdDebug() << "last field in the category" << endl;
      // fragile, widget's parent is the grid, whose parent is the tab page
      QWidget* w = widget->parentWidget()->parentWidget();
      m_tabs->removePage(w);
      delete w; // automatically deletes child widget
    }
    delete widget;
  }
}

void EntryEditDialog::slotUpdateCompletions(Data::Entry* entry_) {
#ifndef NDEBUG
  if(m_currColl != entry_->collection()) {
    kdDebug() << "EntryEditDialog::slotUpdateCompletions - inconsistent collection pointers!" << endl;
    m_currColl = entry_->collection();
  }
#endif

  for(Data::FieldListIterator it(m_currColl->fieldList()); it.current(); ++it) {
    if(it.current()->type() == Data::Field::Line
       && it.current()->flags() & Data::Field::AllowCompletion) {
      QString key = QString::number(m_currColl->id()) + it.current()->name();
      FieldWidget* widget = m_widgetDict.find(key);
      if(widget) {
        if(it.current()->flags() & Data::Field::AllowMultiple) {
          QStringList items = entry_->fields(it.current()->name(), false);
          for(QStringList::ConstIterator it = items.begin(); it != items.end(); ++it) {
            widget->addCompletionObjectItem(*it);
          }
        } else {
          widget->addCompletionObjectItem(entry_->field(it.current()->name()));
        }
      }
    }
  }
}

void EntryEditDialog::slotSetModified(bool mod_/*=true*/) {
  m_modified = mod_;
  enableButton(User1, mod_);
}

bool EntryEditDialog::queryModified() {
//  kdDebug() << "EntryEditDialog::queryModified() - modified is " << (m_modified?"true":"false") << endl;
  bool ok = true;
  if(m_modified) {
    QString str(i18n("The current entry has been modified.\n"
                      "Do you want to enter the changes?"));
    int want_save = KMessageBox::warningYesNoCancel(this, str, i18n("Warning!"),
                                                    i18n("Save Entry"), KStdGuiItem::discard());
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

// modified fields will always have the same name
void EntryEditDialog::slotUpdateField(Data::Collection* coll_, Data::Field* newField_, Data::Field* oldField_) {
  if(coll_ != m_currColl) {
    kdDebug() << "EntryEditDialog::slotUpdateField() - wrong collection pointer!" << endl;
    m_currColl = coll_;
  }
  QString key = QString::number(coll_->id()) + oldField_->name();
  FieldWidget* widget = m_widgetDict[key];
  if(widget) {
    widget->updateField(newField_, oldField_);
    // need to update label widths
    if(newField_->title() != oldField_->title()) {
      int maxWidth = 0;
      QObjectList* childList = widget->parentWidget()->queryList("Bookcase::FieldWidget", 0, false, false);
      QObjectListIt it(*childList);
      for(it.toFirst(); it.current(); ++it) {
        maxWidth = QMAX(maxWidth, static_cast<FieldWidget*>(it.current())->labelWidth());
      }
      for(it.toFirst(); it.current(); ++it) {
        static_cast<FieldWidget*>(it.current())->setLabelWidth(maxWidth);
      }
      delete childList;
    }
    // this is very fragile!
    // field widgets's parent is the grid, whose parent is the tab page
    if(newField_->category() != oldField_->category()) {
      m_tabs->setTabLabel(widget->parentWidget()->parentWidget(), newField_->category());
    }
  }
}
