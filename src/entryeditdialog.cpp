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
#include "controller.h"

#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kaccelmanager.h>
#include <kdeversion.h>
#include <kapplication.h> // for cursor

#include <qlayout.h>
#include <qstringlist.h>
#include <qpushbutton.h>
#include <qvaluevector.h>
#include <qvbox.h>
#include <qobjectlist.h>
#include <qtabbar.h>

// must be an even number
static const int NCOLS = 2; // number of columns of FieldWidgets

using Bookcase::EntryEditDialog;

EntryEditDialog::EntryEditDialog(QWidget* parent_, const char* name_)
    : KDialogBase(parent_, name_, false, i18n("Edit Entry"), Help|User1|User2|Close, User1, false,
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

  setHelp(QString::fromLatin1("entry-editor"));

  resize(configDialogSize(QString::fromLatin1("Edit Dialog Options")));
}

void EntryEditDialog::slotClose() {
  // check to see if an entry should be saved before hiding
  // block signals so the entry view and selection isn't cleared
  if(queryModified()) {
    hide();
//    blockSignals(true);
//    slotHandleNew();
//    blockSignals(false);
  }
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
  bool noChoices = true;

  QStringList catList = m_currColl->fieldCategories();
  for(QStringList::ConstIterator catIt = catList.begin(); catIt != catList.end(); ++catIt) {
    Data::FieldList list = m_currColl->fieldsByCategory(*catIt);

    // if this layout model is changed, be sure to check slotUpdateField()
    QWidget* page = new QWidget(m_tabs);
    // (parent, margin, spacing)
    QVBoxLayout* boxLayout = new QVBoxLayout(page, 0, 0);

    QWidget* grid = new QWidget(page);
    gridList.append(grid);
    // (parent, nrows, ncols, margin, spacing)
    // spacing gets a bit weird, if there are absolute no Choice fields, then spacing should be 4
    QGridLayout* layout = new QGridLayout(grid, 0, NCOLS, 8, 0);

    boxLayout->addWidget(grid, 0);
    // those with multiple, get a stretch
    if(list.count() > 1 || !list.getFirst()->isSingleCategory()) {
      boxLayout->addStretch(1);
    }

    // keep track of which should expand
    QValueVector<bool> expands(NCOLS, false);
    QValueVector<int> maxWidth(NCOLS, 0);

    Data::FieldListIterator it(list); // needed later
    for(int count = 0; it.current(); ++it) {
      Data::Field* f = it.current();
      // ReadOnly and Dependent fields don't get widgets
      if(f->type() == Data::Field::ReadOnly || f->type() == Data::Field::Dependent) {
        continue;
      }
      if(f->type() == Data::Field::Choice) {
        noChoices = false;
      }

      FieldWidget* widget = new FieldWidget(f, grid);
      connect(widget, SIGNAL(modified()), SLOT(slotSetModified()));

      layout->addWidget(widget, count/NCOLS, count%NCOLS);
      layout->setRowStretch(count/NCOLS, 1);

      m_widgetDict.insert(QString::number(m_currColl->id()) + f->name(), widget);

      maxWidth[count%NCOLS] = QMAX(maxWidth[count%NCOLS], widget->labelWidth());
      if(widget->expands()) {
        expands[count%NCOLS] = true;
      }
      widget->updateGeometry();
      if(!f->isSingleCategory()) {
        maxHeight = QMAX(maxHeight, widget->minimumSizeHint().height());
      }
      ++count;
    }

    // now, the labels in a column should all be the same width
    it.toFirst();
    for(int count = 0; it.current(); ++it) {
      FieldWidget* widget = m_widgetDict.find(QString::number(m_currColl->id()) + it.current()->name());
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

    m_tabs->addTab(page, *catIt);
  }

  // Now, go through and set all the field widgets to the same height
  for(QPtrListIterator<QWidget> it(gridList); it.current(); ++it) {
    QGridLayout* l = static_cast<QGridLayout*>(it.current()->layout());
    if(noChoices) {
      l->setSpacing(5);
    }
    for(int row = 0; row < l->numRows() && it.current()->children()->count() > 1; ++row) {
      l->addRowSpacing(row, maxHeight);
    }
    // I don't want anything to be hidden, Keramik has a bug if I don't do this
    it.current()->setMinimumHeight(it.current()->sizeHint().height());
    // the parent of the grid is the page that got added to the tabs
    it.current()->parentWidget()->layout()->invalidate();
    it.current()->parentWidget()->setMinimumHeight(it.current()->parentWidget()->sizeHint().height());
  }

  setUpdatesEnabled(true);
// this doesn't seem to work
//  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
// so do this instead
  layout()->invalidate(); // needed so the sizeHint() gets recalculated
  m_tabs->setMinimumHeight(m_tabs->minimumSizeHint().height());
  m_tabs->setMinimumWidth(m_tabs->sizeHint().width());

  // update keyboard accels
  // only want to manage tabBar(), but KDE bug 71769 means the parent widget must be used
#if KDE_IS_VERSION(3,2,90)
  KAcceleratorManager::manage(m_tabs->tabBar());
#else
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
  Controller::self()->slotUpdateSelection(this); // FIXME:should have a clearSelection() function

  Data::Entry* entry = new Data::Entry(m_currColl);
  m_currEntries.append(entry);
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
    for(Data::EntryListIterator entryIt(m_currEntries); entryIt.current(); ++entryIt) {
      names += entryIt.current()->title();
    }
    QString str(i18n("Do you really want to modify these books?"));
    QString dontAsk = QString::fromLatin1("SaveMultipleBooks");
    int ret = KMessageBox::questionYesNoList(this, str, names, i18n("Modify Multiple Books?"),
                                             KStdGuiItem::yes(), KStdGuiItem::no(), dontAsk);
    if(ret != KMessageBox::Yes) {
      return;
    }
  }

  kapp->setOverrideCursor(Qt::waitCursor);
  Data::FieldListIterator fIt(m_currColl->fieldList());
  // boolean to keep track if every possible field is empty
  bool empty = true;
  for(Data::EntryListIterator entryIt(m_currEntries); entryIt.current(); ++entryIt) {
    for(fIt.toFirst(); fIt.current(); ++fIt) {
      QString key = QString::number(m_currColl->id()) + fIt.current()->name();
      FieldWidget* widget = m_widgetDict.find(key);
      if(widget && widget->isEnabled()) {
        QString temp = widget->text();
        // ok to set field empty string, just not all of them
        entryIt.current()->setField(fIt.current()->name(), temp);
        if(!temp.isEmpty()) {
          empty = false;
        }
      }
    }
  }

  // if something was not empty, signal a save
  if(!empty) {
    m_isOrphan = false;
    emit signalSaveEntries(m_currEntries);
    if(!m_currEntries.getFirst()->title().isEmpty()) {
      setCaption(i18n("Edit Entry") + QString::fromLatin1(" - ") + m_currEntries.getFirst()->title());
    }
  }


  kapp->restoreOverrideCursor();
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

  setCaption(i18n("Edit Entry"));

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

  // multiple entries, so don't set caption
  setCaption(i18n("Edit Entry"));

  // keep track of the entries
  m_currEntries = list_;

  blockSignals(true);

  Data::EntryListIterator entryIt(list_);
  for(Data::FieldListIterator fIt(m_currColl->fieldList()); fIt.current(); ++fIt) {
    QString key = QString::number(m_currColl->id()) + fIt.current()->name();
    FieldWidget* widget = m_widgetDict.find(key);
    if(!widget) { // probably read-only
      continue;
    }
    widget->editMultiple(true);

    QString value = list_.getFirst()->field(fIt.current()->name());
    for(++entryIt; entryIt.current(); ++entryIt) { // skip checking the first one
      if(entryIt.current()->field(fIt.current()->name()) != value) {
        widget->setEnabled(false);
        break;
      }
    }
    entryIt.toFirst();
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

  if(!entry_->title().isEmpty()) {
    setCaption(i18n("Edit Entry") + QString::fromLatin1(" - ") + entry_->title());
  }

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
    m_widgetDict.remove(key);
    // if this is the last field in the category, need to remove the tab page
    // this function is called after the field has been removed from the collection,
    // so the category should be gone from the category list
    if(m_currColl->fieldCategories().findIndex(field_->category()) == -1) {
//      kdDebug() << "last field in the category" << endl;
      // fragile, widget's parent is the grid, whose parent is the tab page
      QWidget* w = widget->parentWidget()->parentWidget();
      m_tabs->removePage(w);
      delete w; // automatically deletes child widget
    } else {
      // much of this replicates code in setLayout()
      QGridLayout* layout = static_cast<QGridLayout*>(widget->parentWidget()->layout());
      delete widget; // automatically removes from layout

      QValueVector<bool> expands(NCOLS, false);
      QValueVector<int> maxWidth(NCOLS, 0);

      Data::FieldList list = m_currColl->fieldsByCategory(field_->category());
      Data::FieldListIterator it(list);
      for(int count = 0; it.current(); ++it) {
        FieldWidget* widget = m_widgetDict.find(QString::number(m_currColl->id()) + it.current()->name());
        if(widget) {
          layout->remove(widget);
          layout->addWidget(widget, count/NCOLS, count%NCOLS);

          maxWidth[count%NCOLS] = QMAX(maxWidth[count%NCOLS], widget->labelWidth());
          if(widget->expands()) {
            expands[count%NCOLS] = true;
          }
          widget->updateGeometry();
          ++count;
        }
      }

      // now, the labels in a column should all be the same width
      it.toFirst();
      for(int count = 0; it.current(); ++it) {
        FieldWidget* widget = m_widgetDict.find(QString::number(m_currColl->id()) + it.current()->name());
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
    }
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
//  kdDebug() << "EntryEditDialog::slotUpdateField() - " << newField_->name() << endl;

  if(coll_ != m_currColl) {
    kdDebug() << "EntryEditDialog::slotUpdateField() - wrong collection pointer!" << endl;
    m_currColl = coll_;
  }

  // if the field type changed, go ahead and redo the whole layout
  // also if the category changed for a non-single field, since a new tab must be created
  if(oldField_->type() != newField_->type()
     || (oldField_->category() != newField_->category() && !newField_->isSingleCategory())) {
    Data::EntryList entries = m_currEntries;
    bool modified = m_modified;
    setLayout(coll_);
    setContents(entries);
    m_modified = modified;
    return;
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
    // this is for singleCategory fields
    if(newField_->category() != oldField_->category()) {
      m_tabs->setTabLabel(widget->parentWidget()->parentWidget(), newField_->category());
    }
  }
}

#include "entryeditdialog.moc"
