/***************************************************************************
    copyright            : (C) 2001-2008 by Robby Stephenson
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
#include "gui/tabwidget.h"
#include "collection.h"
#include "controller.h"
#include "field.h"
#include "entry.h"
#include "tellico_utils.h"
#include "tellico_kernel.h"
#include "tellico_debug.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kacceleratormanager.h>
#include <kpushbutton.h>
#include <kaction.h>
#include <KVBox>

#include <QStringList>
#include <QObject>
#include <QStyle>
#include <QApplication>
#include <QGridLayout>

namespace {
  // must be an even number
  static const int NCOLS = 2; // number of columns of GUI::FieldWidgets
}

using Tellico::EntryEditDialog;

EntryEditDialog::EntryEditDialog(QWidget* parent_)
    : KDialog(parent_),
      m_tabs(new GUI::TabWidget(this)),
      m_modified(false),
      m_isOrphan(false),
      m_isWorking(false),
      m_needReset(false) {
  setCaption(i18n("Edit Entry"));
  setButtons(Help|User1|User2|User3|Apply|Close);
  setDefaultButton(User1);
  setButtonGuiItem(User1, KGuiItem(i18n("&New Entry")));

  setMainWidget(m_tabs);

  m_prevBtn = User3;
  m_nextBtn = User2;
  m_newBtn  = User1;
  m_saveBtn = Apply;
  KGuiItem save = KStandardGuiItem::save();
  save.setText(i18n("Sa&ve Entry"));
  setButtonGuiItem(m_saveBtn, save);
  enableButton(m_saveBtn, false);

  connect(this, SIGNAL(closeClicked()), SLOT(slotClose()));
  connect(this, SIGNAL(applyClicked()), SLOT(slotHandleSave()));
  connect(this, SIGNAL(user1Clicked()), SLOT(slotHandleNew()));
  connect(this, SIGNAL(user2Clicked()), SLOT(slotGoNextEntry()));
  connect(this, SIGNAL(user3Clicked()), SLOT(slotGoPrevEntry()));

  KGuiItem prev;
  prev.setIconName(QLatin1String(QApplication::isLeftToRight() ? "go-previous" : "go-next"));
  prev.setToolTip(i18n("Go to the previous entry in the collection"));
  prev.setWhatsThis(prev.toolTip());

  KGuiItem next;
  next.setIconName(QLatin1String(QApplication::isLeftToRight() ? "go-next" : "go-previous"));
  next.setToolTip(i18n("Go to the next entry in the collection"));
  next.setWhatsThis(next.toolTip());

  setButtonGuiItem(m_nextBtn, next);
  setButtonGuiItem(m_prevBtn, prev);

  // these are just fror the key shortcuts
  KAction* newAct = new KAction(QLatin1String("Go Prev"), this);
  newAct->setShortcut(Qt::Key_PageUp);
  connect(newAct, SIGNAL(triggered()), Controller::self(), SLOT(slotGoPrevEntry()));
  newAct = new KAction(QLatin1String("Go Next"), this);
  newAct->setShortcut(Qt::Key_PageDown);
  connect(newAct, SIGNAL(triggered()), Controller::self(), SLOT(slotGoNextEntry()));

  setHelp(QLatin1String("entry-editor"));

  KConfigGroup config(KGlobal::config(), QLatin1String("Edit Dialog Options"));
  restoreDialogSize(config);
}

void EntryEditDialog::slotClose() {
  // check to see if an entry should be saved before hiding
  // block signals so the entry view and selection isn't cleared
  if(queryModified()) {
    hide();
//    blockSignals(true);
//    slotHandleNew();
//    blockSignals(false);
    KConfigGroup config(KGlobal::config(), QLatin1String("Edit Dialog Options"));
    saveDialogSize(config);
  }
}

void EntryEditDialog::slotReset() {
  if(m_isWorking) {
    return;
  }
//  myDebug();

  slotSetModified(false);
  enableButton(m_saveBtn, false);
  setButtonText(m_saveBtn, i18n("Sa&ve Entry"));
  m_currColl = 0;
  m_currEntries.clear();

  while(m_tabs->count() > 0) {
    QWidget* widget = m_tabs->widget(0);
    m_tabs->removeTab(0);
    delete widget;
  }
  m_widgetDict.clear();
}

void EntryEditDialog::setLayout(Tellico::Data::CollPtr coll_) {
  if(!coll_ || m_isWorking) {
    return;
  }
//  myDebug();

  button(m_newBtn)->setIcon(KIcon(coll_->typeName()));

  setUpdatesEnabled(false);
  if(m_tabs->count() > 0) {
//    myDebug() << "resetting contents.";
    slotReset();
  }
  m_isWorking = true;

  m_currColl = coll_;

  int maxHeight = 0;
  QList<QWidget*> gridList;
  bool noChoices = true;

  bool focusedFirst = false;
  QStringList catList = m_currColl->fieldCategories();
  for(QStringList::ConstIterator catIt = catList.constBegin(); catIt != catList.constEnd(); ++catIt) {
    Data::FieldList fields = m_currColl->fieldsByCategory(*catIt);
    if(fields.isEmpty()) { // sanity check
      continue;
    }

    // if this layout model is changed, be sure to check slotUpdateField()
    QWidget* page = new QWidget(m_tabs);
    // (parent, margin, spacing)
    QBoxLayout* boxLayout = new QVBoxLayout(page);

    QWidget* grid = new QWidget(page);
    gridList.append(grid);
    // (parent, nrows, ncols, margin, spacing)
    // spacing gets a bit weird, if there are absolutely no Choice fields,
    // then spacing should be 5, which is set later
    QGridLayout* layout = new QGridLayout(grid);

    boxLayout->addWidget(grid, 0);
    // those with multiple, get a stretch
    if(fields.count() > 1 || !fields[0]->isSingleCategory()) {
      boxLayout->addStretch(1);
    }

    // keep track of which should expand
    QVector<bool> expands(NCOLS, false);
    QVector<int> maxWidth(NCOLS, 0);

    int count = 0;
    foreach(Data::FieldPtr field, fields) {
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
      connect(widget, SIGNAL(valueChanged()), SLOT(slotSetModified()));
      if(!focusedFirst && widget->focusPolicy() != Qt::NoFocus) {
        widget->setFocus();
        focusedFirst = true;
      }

      int r = count/NCOLS;
      int c = count%NCOLS;
      layout->addWidget(widget, r, c);
      layout->setRowStretch(r, 1);

      m_widgetDict.insert(QString::number(m_currColl->id()) + field->name(), widget);

      maxWidth[count%NCOLS] = qMax(maxWidth[count%NCOLS], widget->labelWidth());
      if(widget->expands()) {
        expands[count%NCOLS] = true;
      }
      widget->updateGeometry();
      if(!field->isSingleCategory()) {
        maxHeight = qMax(maxHeight, widget->minimumSizeHint().height());
      }
      ++count;
    }

    // now, the labels in a column should all be the same width
    count = 0;
    foreach(Data::FieldPtr field, fields) {
      GUI::FieldWidget* widget = m_widgetDict.value(QString::number(m_currColl->id()) + field->name());
      if(widget) {
        widget->setLabelWidth(maxWidth[count%NCOLS]);
        ++count;
      }
    }

    // update stretch factors for columns with a line edit
    for(int col = 0; col < NCOLS; ++col) {
      if(expands[col]) {
        layout->setColumnStretch(col, 1);
      }
    }

    m_tabs->addTab(page, *catIt);
  }

  // Now, go through and set all the field widgets to the same height
  foreach(QWidget* grid, gridList) {
    QGridLayout* l = static_cast<QGridLayout*>(grid->layout());
    if(noChoices) {
      l->setSpacing(5);
    }
    for(int row = 0; row < l->rowCount() && grid->children().count() > 1; ++row) {
      l->setRowMinimumHeight(row, maxHeight);
    }
    // I don't want anything to be hidden, Keramik has a bug if I don't do this
    grid->setMinimumHeight(grid->sizeHint().height());
    // the parent of the grid is the page that got added to the tabs
    grid->parentWidget()->layout()->invalidate();
    grid->parentWidget()->setMinimumHeight(grid->parentWidget()->sizeHint().height());

    // also, no accels for the field widgets
    KAcceleratorManager::setNoAccel(grid);
  }

  setUpdatesEnabled(true);
// this doesn't seem to work
//  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
// so do this instead
  layout()->invalidate(); // needed so the sizeHint() gets recalculated
  m_tabs->setMinimumHeight(m_tabs->minimumSizeHint().height());
  m_tabs->setMinimumWidth(m_tabs->sizeHint().width());

  m_tabs->setCurrentIndex(0);

  m_isWorking = false;
  slotHandleNew();
}

void EntryEditDialog::slotHandleNew() {
  if(!m_currColl || !queryModified()) {
    return;
  }
//  myDebug();

  m_tabs->setCurrentIndex(0);
  m_tabs->setFocusToFirstChild();
  clear();
  m_isWorking = true; // clear() will get called again
  Controller::self()->slotClearSelection();
  m_isWorking = false;

  enableButton(m_prevBtn, false);
  enableButton(m_nextBtn, false);

  Data::EntryPtr entry(new Data::Entry(m_currColl));
  m_currEntries.append(entry);
  m_isOrphan = true;
}

void EntryEditDialog::slotHandleSave() {
  if(!m_currColl || m_isWorking) {
    return;
  }
//  myDebug();

  m_isWorking = true;

  if(m_currEntries.isEmpty()) {
    myDebug() << "creating new entry";
    m_currEntries.append(Data::EntryPtr(new Data::Entry(m_currColl)));
    m_isOrphan = true;
  }

  // add a message box if multiple items are selected
  if(m_currEntries.count() > 1) {
    QStringList names;
    foreach(Data::EntryPtr entry, m_currEntries) {
      names += entry->title();
    }
    QString str(i18n("Do you really want to modify these entries?"));
    QString dontAsk = QLatin1String("SaveMultipleBooks"); // don't change 'books', invisible anyway
    int ret = KMessageBox::questionYesNoList(this, str, names, i18n("Modify Multiple Entries"),
                                             KStandardGuiItem::yes(), KStandardGuiItem::no(), dontAsk);
    if(ret != KMessageBox::Yes) {
      m_isWorking = false;
      return;
    }
  }

  GUI::CursorSaver cs;

  Data::EntryList oldEntries;
  Data::FieldList fields = m_currColl->fields();
  Data::FieldList fieldsRequiringValues;
  // boolean to keep track if any field gets changed
  bool modified = false;
  foreach(Data::EntryPtr entry, m_currEntries) {
    // if the entry is owned, then we're modifying an existing entry, keep a copy of the old one
    if(entry->isOwned()) {
      oldEntries.append(Data::EntryPtr(new Data::Entry(*entry)));
    }
    foreach(Data::FieldPtr fIt, fields) {
      QString key = QString::number(m_currColl->id()) + fIt->name();
      GUI::FieldWidget* widget = m_widgetDict.value(key);
      if(widget && widget->isEnabled()) {
        QString temp = widget->text();
        // ok to set field empty string, just not all of them
        if(modified == false && entry->field(fIt) != temp) {
          modified = true;
        }
        entry->setField(fIt, temp);
        if(temp.isEmpty()) {
          QString prop = fIt->property(QLatin1String("required")).toLower();
          if(prop == QLatin1String("1") || prop == QLatin1String("true")) {
            fieldsRequiringValues.append(fIt);
          }
        }
      }
    }
  }

  if(!fieldsRequiringValues.isEmpty()) {
    GUI::CursorSaver cs2(Qt::ArrowCursor);
    QString str = i18n("A value is required for the following fields. Do you want to continue?");
    QStringList titles;
    foreach(Data::FieldPtr it, fieldsRequiringValues) {
      titles << it->title();
    }
    QString dontAsk = QLatin1String("SaveWithoutRequired");
    int ret = KMessageBox::questionYesNoList(this, str, titles, i18n("Modify Entries"),
                                             KStandardGuiItem::yes(), KStandardGuiItem::no(), dontAsk);
    if(ret != KMessageBox::Yes) {
      m_isWorking = false;
      return;
    }
  }

  // if something was not empty, signal a save
  if(modified) {
    m_isOrphan = false;
    if(oldEntries.isEmpty()) {
      Kernel::self()->addEntries(m_currEntries, false);
    } else {
      Kernel::self()->modifyEntries(oldEntries, m_currEntries);
    }
    if(!m_currEntries.isEmpty() && !m_currEntries[0]->title().isEmpty()) {
      setCaption(i18n("Edit Entry") + QLatin1String(" - ") + m_currEntries[0]->title());
    }
  }

  m_isWorking = false;
  slotSetModified(false);
//  slotHandleNew();
}

void EntryEditDialog::clear() {
  if(m_isWorking) {
    return;
  }
//  myDebug();

  m_isWorking = true;
  // clear the widgets
  foreach(GUI::FieldWidget* widget, m_widgetDict) {
    widget->setEnabled(true);
    widget->clear();
    widget->insertDefault();
  }

  setCaption(i18n("Edit Entry"));

  if(m_isOrphan) {
    if(m_currEntries.count() > 1) {
      myWarning() << "EntryEditDialog::clear() - is an orphan, but more than one";
    }
    m_isOrphan = false;
  }
  m_currEntries.clear();

  setButtonText(m_saveBtn, i18n("Sa&ve Entry"));

  m_isWorking = false;
  slotSetModified(false);
}

void EntryEditDialog::setContents(Tellico::Data::EntryList entries_) {
  // this slot might get called if we try to save multiple items, so just return
  if(m_isWorking) {
    return;
  }

  if(entries_.isEmpty()) {
//    myDebug() << "empty list";
    if(queryModified()) {
      blockSignals(true);
      slotHandleNew();
      blockSignals(false);
    }
    return;
  }

//  myDebug() << entries_.count() << " entries";

  // if some entries get selected in one view, then in another, don't reset
  if(!m_needReset && entries_ == m_currEntries) {
    return;
  }
  m_needReset = false;

  // first set contents to first item
  setContents(entries_.front());
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

  foreach(Data::FieldPtr fIt, m_currColl->fields()) {
    QString key = QString::number(m_currColl->id()) + fIt->name();
    GUI::FieldWidget* widget = m_widgetDict.value(key);
    if(!widget) { // probably read-only
      continue;
    }
    widget->editMultiple(true);

    QString value = entries_[0]->field(fIt);
    for(int i = 1; i < entries_.count(); ++i) {  // skip checking the first one
      if(entries_[i]->field(fIt) != value) {
        widget->setEnabled(false);
        break;
      }
    }
  } // end field loop

  blockSignals(false);
  m_isWorking = false;

  // can't go to next entry if multiple are selected
  enableButton(m_prevBtn, false);
  enableButton(m_nextBtn, false);
  setButtonText(m_saveBtn, i18n("Sa&ve Entries"));
}

void EntryEditDialog::setContents(Tellico::Data::EntryPtr entry_) {
  if(m_isWorking || !queryModified()) {
    return;
  }
//  myDebug();

  if(!entry_) {
    myDebug() << "null entry pointer";
    slotHandleNew();
    return;
  }

//  myDebug() << entry_->title();
  blockSignals(true);
  clear();
  blockSignals(false);

  m_isWorking = true;
  m_currEntries.append(entry_);

  if(!entry_->title().isEmpty()) {
    setCaption(i18n("Edit Entry") + QLatin1String(" - ") + entry_->title());
  }

  if(m_currColl != entry_->collection()) {
    myDebug() << "collections don't match";
    m_currColl = entry_->collection();
  }

  foreach(Data::FieldPtr field, m_currColl->fields()) {
    QString key = QString::number(m_currColl->id()) + field->name();
    GUI::FieldWidget* widget = m_widgetDict.value(key);
    if(!widget) { // is probably read-only
      continue;
    }

    widget->setText(entry_->field(field));
    widget->setEnabled(true);
    widget->editMultiple(false);
  } // end field loop

  enableButton(m_prevBtn, true);
  enableButton(m_nextBtn, true);
  if(entry_->isOwned()) {
    setButtonText(m_saveBtn, i18n("Sa&ve Entry"));
    slotSetModified(false);
  } else {
    // saving is necessary for unoqnwed entries
    slotSetModified(true);
  }
  m_isWorking = false;
}

void EntryEditDialog::removeField(Tellico::Data::CollPtr, Tellico::Data::FieldPtr field_) {
  if(!field_) {
    return;
  }

//  myDebug() << "name = " << field_->name();
  QString key = QString::number(m_currColl->id()) + field_->name();
  GUI::FieldWidget* widget = m_widgetDict.value(key);
  if(widget) {
    m_widgetDict.remove(key);
    // if this is the last field in the category, need to remove the tab page
    // this function is called after the field has been removed from the collection,
    // so the category should be gone from the category list
    if(m_currColl->fieldCategories().indexOf(field_->category()) == -1) {
//      myDebug() << "last field in the category";
      // fragile, widget's parent is the grid, whose parent is the tab page
      QWidget* w = widget->parentWidget()->parentWidget();
      m_tabs->removeTab(m_tabs->indexOf(w));
      delete w; // automatically deletes child widget
    } else {
      // much of this replicates code in setLayout()
      QGridLayout* layout = static_cast<QGridLayout*>(widget->parentWidget()->layout());
      delete widget; // automatically removes from layout

      QVector<bool> expands(NCOLS, false);
      QVector<int> maxWidth(NCOLS, 0);

      Data::FieldList vec = m_currColl->fieldsByCategory(field_->category());
      Data::FieldList::Iterator it = vec.begin();
      int count = 0;
      foreach(Data::FieldPtr field, vec) {
        GUI::FieldWidget* widget = m_widgetDict.value(QString::number(m_currColl->id()) + field->name());
        if(widget) {
          layout->removeWidget(widget);
          layout->addWidget(widget, count/NCOLS, count%NCOLS);

          maxWidth[count%NCOLS] = qMax(maxWidth[count%NCOLS], widget->labelWidth());
          if(widget->expands()) {
            expands[count%NCOLS] = true;
          }
          widget->updateGeometry();
          ++count;
        }
      }

      // now, the labels in a column should all be the same width
      count = 0;
      foreach(Data::FieldPtr field, vec) {
        GUI::FieldWidget* widget = m_widgetDict.value(QString::number(m_currColl->id()) + field->name());
        if(widget) {
          widget->setLabelWidth(maxWidth[count%NCOLS]);
          ++count;
        }
      }

      // update stretch factors for columns with a line edit
      for(int col = 0; col < NCOLS; ++col) {
        if(expands[col]) {
          layout->setColumnStretch(col, 1);
        }
      }
    }
  }
}

void EntryEditDialog::updateCompletions(Tellico::Data::EntryPtr entry_) {
#ifndef NDEBUG
  if(m_currColl != entry_->collection()) {
    myDebug() << "inconsistent collection pointers!";
//    m_currColl = entry_->collection();
  }
#endif

  foreach(Data::FieldPtr it, m_currColl->fields()) {
    if(it->type() != Data::Field::Line
       || !(it->flags() & Data::Field::AllowCompletion)) {
      continue;
    }

    QString key = QString::number(m_currColl->id()) + it->name();
    GUI::FieldWidget* widget = m_widgetDict.value(key);
    if(!widget) {
      continue;
    }
    if(it->flags() & Data::Field::AllowMultiple) {
      QStringList items = entry_->fields(it, false);
      for(QStringList::ConstIterator it = items.constBegin(); it != items.constEnd(); ++it) {
        widget->addCompletionObjectItem(*it);
      }
    } else {
      widget->addCompletionObjectItem(entry_->field(it->name()));
    }
  }
}

void EntryEditDialog::slotSetModified(bool mod_/*=true*/) {
  m_modified = mod_;
  enableButton(m_saveBtn, mod_);
}

bool EntryEditDialog::queryModified() {
//  myDebug() << "modified is " << (m_modified?"true":"false");
  bool ok = true;
  // assume that if the dialog is hidden, we shouldn't ask the user to modify changes
  if(!isVisible()) {
    m_modified = false;
  }
  if(m_modified) {
    QString str(i18n("The current entry has been modified.\n"
                      "Do you want to enter the changes?"));
    KGuiItem item = KStandardGuiItem::save();
    item.setText(i18n("Save Entry"));
    int want_save = KMessageBox::warningYesNoCancel(this, str, i18n("Unsaved Changes"),
                                                    item, KStandardGuiItem::discard());
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
void EntryEditDialog::modifyField(Tellico::Data::CollPtr coll_, Tellico::Data::FieldPtr oldField_, Tellico::Data::FieldPtr newField_) {
//  myDebug() << newField_->name();

  if(coll_ != m_currColl) {
    myDebug() << "wrong collection pointer!";
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
  GUI::FieldWidget* widget = m_widgetDict.value(key);
  if(widget) {
    widget->updateField(oldField_, newField_);
    // need to update label widths
    if(newField_->title() != oldField_->title()) {
      int maxWidth = 0;
      QList<GUI::FieldWidget*> childList = widget->parentWidget()->findChildren<GUI::FieldWidget*>();
      foreach(GUI::FieldWidget* obj, childList) {
        maxWidth = qMax(maxWidth, obj->labelWidth());
      }
      foreach(GUI::FieldWidget* obj, childList) {
        obj->setLabelWidth(maxWidth);
      }
    }
    // this is very fragile!
    // field widgets's parent is the grid, whose parent is the tab page
    // this is for singleCategory fields
    if(newField_->category() != oldField_->category()) {
      int idx = m_tabs->indexOf(widget->parentWidget()->parentWidget());
      if(idx > -1) {
        m_tabs->setTabText(idx, newField_->category());
      }
    }
  }
}

void EntryEditDialog::addEntries(Tellico::Data::EntryList entries_) {
  foreach(Data::EntryPtr entry, entries_) {
    updateCompletions(entry);
  }
}

void EntryEditDialog::modifyEntries(Tellico::Data::EntryList entries_) {
  bool updateContents = false;
  foreach(Data::EntryPtr entry, entries_) {
    updateCompletions(entry);
    if(!updateContents && m_currEntries.contains(entry)) {
      updateContents = true;
    }
  }
  if(updateContents) {
    m_needReset = true;
    setContents(m_currEntries);
  }
}

void EntryEditDialog::slotGoPrevEntry() {
  queryModified();
  Controller::self()->slotGoPrevEntry();
}

void EntryEditDialog::slotGoNextEntry() {
  queryModified();
  Controller::self()->slotGoNextEntry();
}

#include "entryeditdialog.moc"
