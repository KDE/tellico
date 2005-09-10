/***************************************************************************
    copyright            : (C) 2001-2005 by Robby Stephenson
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
#include "gui/tabcontrol.h"
#include "collection.h"
#include "controller.h"
#include "field.h"
#include "entry.h"
#include "tellico_utils.h"
#include "tellico_kernel.h"
#include "tellico_debug.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kaccelmanager.h>
#include <kiconloader.h>
#include <kdeversion.h>

#include <qlayout.h>
#include <qstringlist.h>
#include <qpushbutton.h>
#include <qvaluevector.h>
#include <qvbox.h>
#include <qobjectlist.h>
#include <qtabbar.h>
#include <qstyle.h>

namespace {
  // must be an even number
  static const int NCOLS = 2; // number of columns of GUI::FieldWidgets
}

using Tellico::EntryEditDialog;

EntryEditDialog::EntryEditDialog(QWidget* parent_, const char* name_)
    : KDialogBase(parent_, name_, false, i18n("Edit Entry"), Help|User1|User2|Close, User1, false,
                  KStdGuiItem::save(), KGuiItem(i18n("&New Entry"))),
      m_currColl(0),
      m_tabs(new GUI::TabControl(this)),
      m_modified(false),
      m_isOrphan(false),
      m_isWorking(false) {
  setMainWidget(m_tabs);

  enableButton(User1, false);

  connect(this, SIGNAL(user1Clicked()), SLOT(slotHandleSave()));
  connect(this, SIGNAL(user2Clicked()), SLOT(slotHandleNew()));

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
  if(m_isWorking) {
    return;
  }

  enableButton(User1, false);
  setButtonText(User1, i18n("Sa&ve Entry"));
  m_currColl = 0;
  m_currEntries.clear();

  m_modified = false;

  m_tabs->clear(); // GUI::FieldWidgets get deleted here
  m_widgetDict.clear();
}

void EntryEditDialog::setLayout(Data::Collection* coll_) {
  if(!coll_ || m_isWorking) {
    return;
  }
//  kdDebug() << "EntryEditDialog::setLayout()" << endl;

  actionButton(User2)->setIconSet(UserIconSet(coll_->entryName()));

  setUpdatesEnabled(false);
  if(m_tabs->count() > 0) {
//    kdDebug() << "EntryEditDialog::setLayout() - resetting contents." << endl;
    slotReset();
  }
  m_isWorking = true;

  m_currColl = coll_;

  int maxHeight = 0;
  QPtrList<QWidget> gridList;
  bool noChoices = true;

  QStringList catList = m_currColl->fieldCategories();
  for(QStringList::ConstIterator catIt = catList.begin(); catIt != catList.end(); ++catIt) {
    Data::FieldVec fields = m_currColl->fieldsByCategory(*catIt);
    if(fields.isEmpty()) { // sanity check
      continue;
    }

    // if this layout model is changed, be sure to check slotUpdateField()
    QWidget* page = new QWidget(m_tabs);
    // (parent, margin, spacing)
    QVBoxLayout* boxLayout = new QVBoxLayout(page, 0, 0);

    QWidget* grid = new QWidget(page);
    gridList.append(grid);
    // (parent, nrows, ncols, margin, spacing)
    // spacing gets a bit weird, if there are absolutely no Choice fields,
    // then spacing should be 5, which is set later
    QGridLayout* layout = new QGridLayout(grid, 0, NCOLS, 8, 2);
    // keramik styles make big widget, cut down the spacing a bit
    if(QCString(style().name()).lower().find("keramik", 0, false) > -1) {
      layout->setSpacing(0);
    }

    boxLayout->addWidget(grid, 0);
    // those with multiple, get a stretch
    if(fields.count() > 1 || !fields[0]->isSingleCategory()) {
      boxLayout->addStretch(1);
    }

    // keep track of which should expand
    QValueVector<bool> expands(NCOLS, false);
    QValueVector<int> maxWidth(NCOLS, 0);

    Data::FieldVec::Iterator it = fields.begin(); // needed later
    for(int count = 0; it != fields.end(); ++it) {
      Data::Field* field = it;
      // ReadOnly and Dependent fields don't get widgets
      if(field->type() == Data::Field::ReadOnly || field->type() == Data::Field::Dependent) {
        continue;
      }
      if(field->type() == Data::Field::Choice) {
        noChoices = false;
      }

      GUI::FieldWidget* widget = GUI::FieldWidget::create(field, grid);
      if(!widget) {
        continue;
      }
      connect(widget, SIGNAL(modified()), SLOT(slotSetModified()));

      int r = count/NCOLS;
      int c = count%NCOLS;
      layout->addWidget(widget, r, c);
      layout->setRowStretch(r, 1);

      m_widgetDict.insert(QString::number(m_currColl->id()) + field->name(), widget);

      maxWidth[count%NCOLS] = KMAX(maxWidth[count%NCOLS], widget->labelWidth());
      if(widget->expands()) {
        expands[count%NCOLS] = true;
      }
      widget->updateGeometry();
      if(!field->isSingleCategory()) {
        maxHeight = KMAX(maxHeight, widget->minimumSizeHint().height());
      }
      ++count;
    }

    // now, the labels in a column should all be the same width
    it = fields.begin();
    for(int count = 0; it != fields.end(); ++it) {
      GUI::FieldWidget* widget = m_widgetDict.find(QString::number(m_currColl->id()) + it->name());
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
  // for some reason, RedHat 9 with KDE 3.1 barfs on KAcceleratorManager
  // so disable it for all KDE 3.1
#if KDE_IS_VERSION(3,2,90)
  KAcceleratorManager::manage(m_tabs->tabBar());
#elif KDE_IS_VERSION(3,1,90)
  KAcceleratorManager::manage(m_tabs->tabBar()->parentWidget());
#endif

  m_tabs->setCurrentPage(0);

  m_isWorking = false;
  slotHandleNew();
  m_modified = false; // because the year is inserted
}

void EntryEditDialog::slotHandleNew() {
  if(!m_currColl || !queryModified()) {
    return;
  }

  m_tabs->setResetFocus(true);
  m_tabs->setCurrentPage(0);
  clear();
  m_isWorking = true; // clear() will get called again
  Controller::self()->slotClearSelection();
  m_isWorking = false;

  Data::Entry* entry = new Data::Entry(m_currColl);
  m_currEntries.append(entry);
  m_isOrphan = true;
}

void EntryEditDialog::slotHandleSave() {
  if(!m_currColl || m_isWorking) {
    return;
  }

  m_isWorking = true;

  if(m_currEntries.isEmpty()) {
    m_currEntries.append(new Data::Entry(m_currColl));
    m_isOrphan = true;
  }

  // add a message box if multiple items are selected
  if(m_currEntries.count() > 1) {
    QStringList names;
    for(Data::EntryVec::ConstIterator entry = m_currEntries.constBegin(); entry != m_currEntries.constEnd(); ++entry) {
      names += entry->title();
    }
    QString str(i18n("Do you really want to modify these entries?"));
    QString dontAsk = QString::fromLatin1("SaveMultipleBooks"); // don't change 'books', invisible anyway
    int ret = KMessageBox::questionYesNoList(this, str, names, i18n("Modify Multiple Entries"),
                                             KStdGuiItem::yes(), KStdGuiItem::no(), dontAsk);
    if(ret != KMessageBox::Yes) {
      m_isWorking = false;
      return;
    }
  }

  GUI::CursorSaver cs(Qt::waitCursor);

  Data::EntryVec oldEntries;

  Data::FieldVec fields = m_currColl->fields();
  // boolean to keep track if every possible field is empty
  bool empty = true;
  for(Data::EntryVecIt entry = m_currEntries.begin(); entry != m_currEntries.end(); ++entry) {
    // if the entry is owned, then we're modifying an existing entry, keep a copy of the old one
    if(entry->isOwned()) {
      oldEntries.append(new Data::Entry(*entry));
    }
    for(Data::FieldVec::Iterator fIt = fields.begin(); fIt != fields.end(); ++fIt) {
      QString key = QString::number(m_currColl->id()) + fIt->name();
      GUI::FieldWidget* widget = m_widgetDict.find(key);
      if(widget && widget->isEnabled()) {
        QString temp = widget->text();
        // ok to set field empty string, just not all of them
        entry->setField(fIt->name(), temp);
        if(!temp.isEmpty()) {
          empty = false;
        }
      }
    }
  }

  // if something was not empty, signal a save
  if(!empty) {
    m_isOrphan = false;
    Kernel::self()->saveEntries(oldEntries, m_currEntries);
    if(!m_currEntries.isEmpty() && !m_currEntries[0].data()->title().isEmpty()) {
      setCaption(i18n("Edit Entry") + QString::fromLatin1(" - ") + m_currEntries[0].data()->title());
    }
  }

  m_modified = false;
  m_isWorking = false;
  enableButton(User1, false);
//  slotHandleNew();
}

void EntryEditDialog::clear() {
//  kdDebug() << "EntryEditDialog::clear()" << endl;
  if(m_isWorking) {
    return;
  }

  m_isWorking = true;
  // clear the widgets
  for(QDictIterator<GUI::FieldWidget> it(m_widgetDict); it.current(); ++it) {
    it.current()->setEnabled(true);
    it.current()->clear();
  }

  setCaption(i18n("Edit Entry"));

  if(m_isOrphan) {
    if(m_currEntries.count() > 1) {
      kdWarning() << "EntryEditDialog::clear() - is an orphan, but more than one" << endl;
    }
    m_isOrphan = false;
  }
  m_currEntries.clear();

  setButtonText(User1, i18n("Sa&ve Entry"));
  enableButton(User1, false);

  m_modified = false;
  m_isWorking = false;
}

void EntryEditDialog::setContents(Data::EntryVec entries_) {
  // this slot might get called if we try to save multiple items, so just return
  if(m_isWorking) {
    return;
  }

  if(entries_.isEmpty()) {
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
  setContents(entries_[0]);
  // something weird...if list count can actually be 1 before the setContents call
  // and 0 after it. Why is that? It's const!
  if(entries_.count() < 2) {
    return;
  }

  // multiple entries, so don't set caption
  setCaption(i18n("Edit Entries"));

  m_currEntries = entries_;
  m_isWorking = true;
  blockSignals(true);

  Data::EntryVec::ConstIterator entry;
  Data::FieldVec fields = m_currColl->fields();
  for(Data::FieldVec::Iterator fIt = fields.begin(); fIt != fields.end(); ++fIt) {
    QString key = QString::number(m_currColl->id()) + fIt->name();
    GUI::FieldWidget* widget = m_widgetDict.find(key);
    if(!widget) { // probably read-only
      continue;
    }
    widget->editMultiple(true);

    QString value = entries_[0].data()->field(fIt->name());
    entry = entries_.constBegin();
    for(++entry; entry != entries_.constEnd(); ++entry) { // skip checking the first one
      if(entry->field(fIt->name()) != value) {
        widget->setEnabled(false);
        break;
      }
    }
  } // end field loop

  blockSignals(false);
  m_isWorking = false;

  setButtonText(User1, i18n("Sa&ve Entries"));
}

void EntryEditDialog::setContents(Data::Entry* entry_) {
  bool ok = queryModified();
  if(!ok || m_isWorking) {
    return;
  }

  if(!entry_) {
    kdDebug() << "EntryEditDialog::setContents() - null entry pointer" << endl;
    slotHandleNew();
    return;
  }

  m_tabs->setResetFocus(false);

//  kdDebug() << "EntryEditDialog::setContents() - " << entry_->title() << endl;
  blockSignals(true);
  clear();
  blockSignals(false);
  m_isWorking = true;
  m_currEntries.append(entry_);

  if(!entry_->title().isEmpty()) {
    setCaption(i18n("Edit Entry") + QString::fromLatin1(" - ") + entry_->title());
  }

  if(m_currColl.data() != entry_->collection()) {
    kdDebug() << "EntryEditDialog::setContents() - collections don't match" << endl;
    m_currColl = entry_->collection();
  }

//  m_tabs->showTab(0);

  slotSetModified(false);

  Data::FieldVec fields = m_currColl->fields();
  for(Data::FieldVec::Iterator it = fields.begin(); it != fields.end(); ++it) {
    QString key = QString::number(m_currColl->id()) + it->name();
    GUI::FieldWidget* widget = m_widgetDict.find(key);
    if(!widget) { // is probably read-only
      continue;
    }

    QString value = entry_->field(it->name());
    widget->setText(value);
    widget->setEnabled(true);
    widget->editMultiple(false);
  } // end field loop

  if(entry_->isOwned()) {
    setButtonText(User1, i18n("Sa&ve Entry"));
    enableButton(User1, m_modified);
  } else {
    slotSetModified(true);
  }
  m_isWorking = false;
}

void EntryEditDialog::removeField(Data::Collection*, Data::Field* field_) {
  if(!field_) {
    return;
  }

//  kdDebug() << "EntryEditDialog::removeField - name = " << field_->name() << endl;
  QString key = QString::number(m_currColl->id()) + field_->name();
  GUI::FieldWidget* widget = m_widgetDict.find(key);
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

      Data::FieldVec vec = m_currColl->fieldsByCategory(field_->category());
      Data::FieldVec::Iterator it = vec.begin();
      for(int count = 0; it != vec.end(); ++it) {
        GUI::FieldWidget* widget = m_widgetDict.find(QString::number(m_currColl->id()) + it->name());
        if(widget) {
          layout->remove(widget);
          layout->addWidget(widget, count/NCOLS, count%NCOLS);

          maxWidth[count%NCOLS] = KMAX(maxWidth[count%NCOLS], widget->labelWidth());
          if(widget->expands()) {
            expands[count%NCOLS] = true;
          }
          widget->updateGeometry();
          ++count;
        }
      }

      // now, the labels in a column should all be the same width
      it = vec.begin();
      for(int count = 0; it != vec.end(); ++it) {
        GUI::FieldWidget* widget = m_widgetDict.find(QString::number(m_currColl->id()) + it->name());
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

void EntryEditDialog::updateCompletions(Data::Entry* entry_) {
#ifndef NDEBUG
  if(m_currColl.data() != entry_->collection()) {
    kdDebug() << "EntryEditDialog::updateCompletions - inconsistent collection pointers!" << endl;
    m_currColl = entry_->collection();
  }
#endif

  Data::FieldVec fields = m_currColl->fields();
  for(Data::FieldVec::Iterator it = fields.begin(); it != fields.end(); ++it) {
    if(it->type() != Data::Field::Line
       || !(it->flags() & Data::Field::AllowCompletion)) {
      continue;
    }

    QString key = QString::number(m_currColl->id()) + it->name();
    GUI::FieldWidget* widget = m_widgetDict.find(key);
    if(!widget) {
      continue;
    }
    if(it->flags() & Data::Field::AllowMultiple) {
      QStringList items = entry_->fields(it->name(), false);
      for(QStringList::ConstIterator it = items.begin(); it != items.end(); ++it) {
        widget->addCompletionObjectItem(*it);
      }
    } else {
      widget->addCompletionObjectItem(entry_->field(it->name()));
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
    KGuiItem item = KStdGuiItem::save();
    item.setText(i18n("Save Entry"));
    int want_save = KMessageBox::warningYesNoCancel(this, str, i18n("Unsaved Changes"),
                                                    item, KStdGuiItem::discard());
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
void EntryEditDialog::modifyField(Data::Collection* coll_, Data::Field* oldField_, Data::Field* newField_) {
//  kdDebug() << "EntryEditDialog::slotUpdateField() - " << newField_->name() << endl;

  if(coll_ != m_currColl) {
    kdDebug() << "EntryEditDialog::slotUpdateField() - wrong collection pointer!" << endl;
    m_currColl = coll_;
  }

  // if the field type changed, go ahead and redo the whole layout
  // also if the category changed for a non-single field, since a new tab must be created
  if(oldField_->type() != newField_->type()
     || (oldField_->category() != newField_->category() && !newField_->isSingleCategory())) {
    bool modified = m_modified;
    setLayout(coll_);
    setContents(m_currEntries);
    m_modified = modified;
    return;
  }

  QString key = QString::number(coll_->id()) + oldField_->name();
  GUI::FieldWidget* widget = m_widgetDict[key];
  if(widget) {
    widget->updateField(oldField_, newField_);
    // need to update label widths
    if(newField_->title() != oldField_->title()) {
      int maxWidth = 0;
      QObjectList* childList = widget->parentWidget()->queryList("Tellico::GUI::FieldWidget", 0, false, false);
      QObjectListIt it(*childList);
      for(it.toFirst(); it.current(); ++it) {
        maxWidth = KMAX(maxWidth, static_cast<GUI::FieldWidget*>(it.current())->labelWidth());
      }
      for(it.toFirst(); it.current(); ++it) {
        static_cast<GUI::FieldWidget*>(it.current())->setLabelWidth(maxWidth);
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
