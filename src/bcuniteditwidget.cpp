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
#include "bcunit.h"
#include "bccollection.h"
#include "bcattribute.h"
#include "bcdetailedlistview.h"
#include "bctabcontrol.h"
#include "isbnvalidator.h"
#include "bookcase.h"
#include "bcutils.h"

#include <kcompletion.h>
#include <kbuttonbox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>

#include <qlayout.h>
#include <qstringlist.h>
#include <qlabel.h>
#include <qlistview.h>
#include <qptrlist.h>
#include <qgrid.h>
#include <qpushbutton.h>
#include <qwhatsthis.h>
#include <qvbox.h>

//#define SHOW_COPY_BTN

// must be an even number
static const int NCOLS = 4;

BCUnitEditWidget::BCUnitEditWidget(QWidget* parent_, const char* name_/*=0*/)
 : QWidget(parent_, name_), m_currColl(0), m_currUnit(0),
   m_tabs(new BCTabControl(this)), m_modified(false) {
//  kdDebug() << "BCUnitEditWidget()" << endl;
  QVBoxLayout* topLayout = new QVBoxLayout(this);

  connect(m_tabs, SIGNAL(tabSelected(int)), SLOT(slotSwitchFocus(int)));

  topLayout->setSpacing(5);
  topLayout->setMargin(5);
  // stretch = 1 so that the tabs expand vertically
  topLayout->addWidget(m_tabs, 1);

  KButtonBox* bb = new KButtonBox(this);
  bb->addStretch();
  m_new = bb->addButton(i18n("New Book"), this, SLOT(slotHandleNew()));
#ifdef SHOW_COPY_BTN
  m_copy = bb->addButton(i18n("Duplicate Book"), this, SLOT(slotHandleCopy()));
#endif
  m_save = bb->addButton(i18n("Enter Book"), this, SLOT(slotHandleSave()));
  m_delete = bb->addButton(i18n("Delete Book"), this, SLOT(slotHandleDelete()));
//  m_clear = bb->addButton(i18n("Clear Data"), this, SLOT(slotHandleClear()));
  bb->addStretch();
  // stretch = 0, so the height of the buttonbox is constant
  topLayout->addWidget(bb, 0, Qt::AlignBottom | Qt::AlignHCenter);

  m_save->setEnabled(false);
  // no currUnit exists, so disable the Copy and Delete button
#ifdef SHOW_COPY_BTN
  m_copy->setEnabled(false);
#endif
  m_delete->setEnabled(false);
}

void BCUnitEditWidget::slotReset() {
//  kdDebug() << "BCUnitEditWidget::slotReset()" << endl;

  m_currUnit = 0;
  m_save->setEnabled(false);
  m_delete->setEnabled(false);
  m_save->setText(i18n("Enter Book"));
  m_currColl = 0;

  //TODO might need this when support multiple collection types
// clear all the dicts
//  m_editDict.clear();
//  m_multiDict.clear();
//  m_comboDict.clear();
//  m_checkDict.clear();
}

void BCUnitEditWidget::slotSetCollection(BCCollection* coll_) {
  if(!coll_) {
    return;
  }
//  kdDebug() << "BCUnitEditWidget::slotSetCollection() - " << coll_->title() << endl;

// for now, reset, but if multiple collections are supported, this has to change
  slotHandleClear();
  m_currColl = coll_;
  m_currUnit = new BCUnit(m_currColl);

// don't do this, ,it would cause infinite looping
//  if(m_tabs->count() == 0) {
//    slotSetLayout(coll_);
//  }
  
  // go back to first tab, with title, etc...
  m_tabs->showTab(0);
}

void BCUnitEditWidget::slotSetLayout(BCCollection* coll_) {
  if(!coll_) {
    return;
  }
  
//  kdDebug() << "BCUnitEditWidget::slotSetLayout()" << endl;

  if(m_tabs->count() > 0) {
    kdDebug() << "BCUnitEditWidget::slotSetLayout() - tabs already exist." << endl;
    return;
  }

  slotSetCollection(coll_);

  KLineEdit* kl;
  QTextEdit* te;
  KComboBox* kc;
  QCheckBox* cb;

  QStringList catList = m_currColl->attributeCategories();
  QStringList::ConstIterator catIt = catList.begin();
  for( ; catIt != catList.end(); ++catIt) {
    QGrid* grid = new QGrid(NCOLS, m_tabs);
    grid->setMargin(10);
    grid->setSpacing(5);

    QString catName = static_cast<QString>(*catIt);
    BCAttributeList list = m_currColl->attributesByCategory(catName);
    BCAttributeListIterator it(list);
    for( ; it.current(); ++it) {
      QLabel* la = new QLabel(it.current()->title() + QString::fromLatin1(":"), grid);
      la->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      QWhatsThis::add(la, it.current()->description());

      switch(it.current()->type()) {
        case BCAttribute::Line:
          kl = new KLineEdit(QString::null, grid);
          connect(kl, SIGNAL(textChanged(const QString&)), this, SLOT(slotSetModified()));
          QWhatsThis::add(kl, it.current()->description());
          if(! (it.current()->flags() & BCAttribute::NoComplete)) {
            kl->completionObject()->setItems(m_currColl->valuesByAttributeName(it.current()->name()));
            kl->setAutoDeleteCompletionObject(true);
          }
          
          if(it.current()->name() == QString::fromLatin1("isbn")) {
            ISBNValidator* isbn = new ISBNValidator(this);
            kl->setValidator(isbn);
          }
          
          m_editDict.insert(QString::number(m_currColl->id()) + it.current()->name(), kl);
          break;

        case BCAttribute::Para:
          te = new QTextEdit(grid);
          te->setTextFormat(Qt::PlainText);
          connect(te, SIGNAL(textChanged()), this, SLOT(slotSetModified()));
          QWhatsThis::add(te, it.current()->description());
          m_multiDict.insert(QString::number(m_currColl->id()) + it.current()->name(), te);
          break;

        case BCAttribute::Choice:
          kc = new KComboBox(grid);
          connect(kc, SIGNAL(activated(int)), this, SLOT(slotSetModified()));
          QWhatsThis::add(kc, it.current()->description());
          // always have empty choice
          kc->insertItem(QString::null);
          kc->insertStringList(it.current()->allowed());
          kc->setEditable(false);
          m_comboDict.insert(QString::number(m_currColl->id()) + it.current()->name(), kc);
          break;

        case BCAttribute::Bool:
          cb = new QCheckBox(grid);
          connect(cb, SIGNAL(clicked()), this, SLOT(slotSetModified()));
          QWhatsThis::add(cb, it.current()->description());
          m_checkDict.insert(QString::number(m_currColl->id()) + it.current()->name(), cb);
          break;
          
        case BCAttribute::Year:
          kl = new KLineEdit(QString::null, grid);
          connect(kl, SIGNAL(textChanged(const QString&)), this, SLOT(slotSetModified()));
          QWhatsThis::add(kl, it.current()->description());

          kl->setMaxLength(4);
          kl->setValidator(new QIntValidator(1000, 9999, this));
            
          m_editDict.insert(QString::number(m_currColl->id()) + it.current()->name(), kl);
          break;

        case BCAttribute::ReadOnly:
          break;

        default:
          kdDebug() << "BCUnitEditWidget() - unknown attribute type  ("
                    << it.current()->type() << ") named " << it.current()->name() << endl;
         break;
      } // end switch
    }
    // I don't want anything to be hidden
    grid->setMinimumHeight(grid->sizeHint().height());
    m_tabs->addTab(grid, catName);
  }

// this doesn't seem to work
//  setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));
// so do this instead
  setMinimumHeight(sizeHint().height());
  setMaximumHeight(sizeHint().height());
}

void BCUnitEditWidget::slotHandleNew() {
  if(!queryModified()) {
    return;
  }
  slotHandleClear();
  m_currUnit = new BCUnit(m_currColl);
//  m_save->setText(i18n("Enter Book"));
}

void BCUnitEditWidget::slotHandleCopy() {
  // if the currUnit exists and has already been saved
  // TODO: if the attribute values have been changed without clicking save
  // need to ask user for confirmation
  if(m_currUnit && m_currColl
      && m_currColl->unitList().containsRef(m_currUnit) > 0) {
    m_currUnit = new BCUnit(*m_currUnit);
    // slotHandleSave() clears everthing, so need to keep a pointer
    BCUnit* unit = m_currUnit;
    slotHandleSave();
    // let's be nice and put everything back in there
    slotSetContents(unit);
  }
}

void BCUnitEditWidget::slotHandleSave() {
  if(!m_currColl) {
    // big problem
    kdDebug() << "BCUnitEditWidget::slotHandleSave() - no valid collection pointer" << endl;
  }

  // make sure we have a good BCUnit pointer
  if(!m_currUnit) {
    m_currUnit = new BCUnit(m_currColl);
    kdDebug() << "BCUnitEditWidget::slotHandleSave() - new BCUnit pointer created"
      " in collection " << m_currColl->title() << endl;
  }

  // boolean to keep track if every possible attribute is empty
  bool empty = true;
  KLineEdit* kl;
  QTextEdit* te;
  KComboBox* kc;
  QCheckBox* cb;
  
  QString temp;
  BCAttributeListIterator it(m_currColl->attributeList());
  for( ; it.current(); ++it) {
    switch(it.current()->type()) {
      case BCAttribute::Line:
        kl = m_editDict.find(QString::number(m_currColl->id()) + it.current()->name());
        if(kl) {
          temp = kl->text().simplifyWhiteSpace();
          // ok to set attribute empty string
          m_currUnit->setAttribute(it.current()->name(), temp);
          if(!temp.isEmpty()) {
            empty = false;
          }
          // the completion object is updated when slotUpdateCompletions is called
        }
        break;

      case BCAttribute::Para:
        te = m_multiDict.find(QString::number(m_currColl->id()) + it.current()->name());
        if(te) {
          temp = te->text().simplifyWhiteSpace();
          m_currUnit->setAttribute(it.current()->name(), temp);
          if(!temp.isEmpty()) {
            empty = false;
          }
        }
        break;

      case BCAttribute::Choice:
        kc = m_comboDict.find(QString::number(m_currColl->id()) + it.current()->name());
        if(kc) {
          temp = kc->currentText().simplifyWhiteSpace();
          // ok to set attribute empty
          m_currUnit->setAttribute(it.current()->name(), temp);
          if(!temp.isEmpty()) {
            //empty = false;
          }
        }
        break;

      case BCAttribute::Bool:
        cb = m_checkDict.find(QString::number(m_currColl->id()) + it.current()->name());
        if(cb) {
          if(cb->isChecked()) {
            // "1" means checked
            m_currUnit->setAttribute(it.current()->name(), QString::fromLatin1("1"));
            //empty = false;
          } else {
            m_currUnit->setAttribute(it.current()->name(), QString());
          }
        }
        break;

      case BCAttribute::Year:
        kl = m_editDict.find(QString::number(m_currColl->id()) + it.current()->name());
        if(kl) {
          temp = kl->text();
          // ok to set attribute empty string
          m_currUnit->setAttribute(it.current()->name(), temp);
        }
        break;
              
      case BCAttribute::ReadOnly:
        break;

      default:
        kdWarning() << "BCUnitEditWidget::slotHandleSave() - unknown attribute type ("
          "" << it.current()->type() << ") named " << it.current()->name() << endl;
        return;
        break;
    } // end switch
  }

  // if something was not empty, signal a save, and then clear everything
  if(!empty) {
    // this has to be before signal, since slotSetContents() gets called
    m_modified = false;
    emit signalSaveUnit(m_currUnit);
  } else {
    // go back to first tab, with title, etc...
    m_tabs->showTab(0);
  }

  slotHandleNew();
}

void BCUnitEditWidget::slotHandleDelete() {
  if(m_currUnit && m_currColl->unitList().containsRef(m_currUnit) > 0) {
//    kdDebug() << "BCUnitEditWidget::slotHandleDelete() - item " << m_currUnit->title() << endl;
    // this widget does not actually delete the unit
    emit signalDeleteUnit(m_currUnit);
  }

  // clear the widget whether or not anything was deleted
  slotHandleClear();
}

void BCUnitEditWidget::slotHandleClear() {
//  kdDebug() << "BCUnitEditWidget::slotHandleClear()" << endl;

  // clear the linedits
  QDictIterator<KLineEdit> it1(m_editDict);
  for( ; it1.current(); ++it1) {
    it1.current()->clear();
  }

  // clear the linedits
  QDictIterator<QTextEdit> it2(m_multiDict);
  for( ; it2.current(); ++it2) {
    it2.current()->clear();
  }

  // set all the comboboxes to the first item, which should be the null string
  QDictIterator<KComboBox> it3(m_comboDict);
  for( ; it3.current(); ++it3) {
    it3.current()->setCurrentItem(0);
  }

  // clear the checkboxes, too
  QDictIterator<QCheckBox> it4(m_checkDict);
  for( ; it4.current(); ++it4) {
    it4.current()->setChecked(false);
  }

  // this crashes the app, find out why...
  // nullify the pointer...
//  if(m_currUnit
//      && m_currUnit->collection()->unitList().containsRef(m_currUnit) == 0) {
//    // this means we created a pointer but haven't added it to the collection yet
//    // TODO: ask user if he wants to save it
//    delete m_currUnit;
//  }

  m_currUnit = 0;

  // disable the copy and delete buttons
#ifdef SHOW_COPY_BTN
  m_copy->setEnabled(false);
#endif
  m_delete->setEnabled(false);
  m_save->setText(i18n("Enter Book"));
  m_save->setEnabled(false);

  m_modified = false;
}

void BCUnitEditWidget::slotSetContents(BCUnit* unit_) {
  bool ok = queryModified();
  if(!ok) {
    return;
  }
  
  if(!unit_) {
    slotHandleClear();
    return;
  }

//  kdDebug() << "BCUnitEditWidget::slotSetContents() - " << unit_->title() << endl;
  m_currUnit = unit_;
//  m_currColl = unit_->collection();
  if(m_currColl != unit_->collection()) {
    kdDebug() << "BCUnitEditWidget::slotSetContents() - collections don't match" << endl;
    m_currColl = unit_->collection();
  }
  
  //disable save button
  m_save->setEnabled(false);
  // enable copy and delete buttons
#ifdef SHOW_COPY_BTN
  m_copy->setEnabled(true);
#endif
  m_delete->setEnabled(true);

//  m_tabs->showTab(0);

  KLineEdit* kl;
  QTextEdit* te;
  KComboBox* kc;
  QCheckBox* cb;
  
  BCAttributeListIterator it(m_currColl->attributeList());
  for( ; it.current(); ++it) {
    switch(it.current()->type()) {
      case BCAttribute::Line:
        kl = m_editDict.find(QString::number(m_currColl->id()) + it.current()->name());
        if(kl) {
          kl->setText(unit_->attribute(it.current()->name()));
        }
        break;

      case BCAttribute::Para:
        te = m_multiDict.find(QString::number(m_currColl->id()) + it.current()->name());
        break;

      case BCAttribute::Choice:
        kc = m_comboDict.find(QString::number(m_currColl->id()) + it.current()->name());
        for(int i = 0; kc && i < kc->count(); ++i) {
          if(kc->text(i) == m_currUnit->attribute(it.current()->name())) {
            kc->setCurrentItem(i);
            break;
          }
        }
        break;

      case BCAttribute::Bool:
        cb = m_checkDict.find(QString::number(m_currColl->id()) + it.current()->name());
        if(cb) {
          if(m_currUnit->attribute(it.current()->name()).isEmpty()) {
            cb->setChecked(false);
          } else {
            cb->setChecked(true);
          }
        }
        break;

      case BCAttribute::Year:
        kl = m_editDict.find(QString::number(m_currColl->id()) + it.current()->name());
        if(kl) {
          kl->setText(unit_->attribute(it.current()->name()));
        }
        break;

      case BCAttribute::ReadOnly:
        break;

      default:
        kdWarning() << "BCUnitEditWidget::slotSetContents() - unknown attribute type ("
          "" << it.current()->type() << ") named " << it.current()->name() << endl;
        break;
    } //end switch
  } // end attribute loop
  
  if(m_currUnit && m_currColl->unitList().containsRef(m_currUnit) > 0) {
    m_save->setText(i18n("Modify Book"));
    m_save->setEnabled(false);
  }
  m_modified = false;
}

void BCUnitEditWidget::slotUpdateCompletions(BCUnit* unit_) {
  if(m_currColl != unit_->collection()) {
    kdDebug() << "BCUnitEditWidget::slotUpdateCompletions - inconsistent colleection pointers!" << endl;
    m_currColl = unit_->collection();
  }
  
  BCAttributeListIterator it(m_currColl->attributeList());
  for( ; it.current(); ++it) {
    if(it.current()->type() == BCAttribute::Line
                  && !(it.current()->flags() & BCAttribute::NoComplete)) {
      QString key = QString::number(m_currColl->id()) + it.current()->name();
      KLineEdit* kl = m_editDict.find(key);
      if(kl) {
        QString value = unit_->attribute(it.current()->name());
        kl->completionObject()->addItem(value);
      }
    }
  }
}

void BCUnitEditWidget::slotSwitchFocus(int tabNum_) {
  m_tabs->setFocusToChild(tabNum_);
}

void BCUnitEditWidget::slotSetModified() {
  m_modified = true;
  m_save->setEnabled(true);
}

bool BCUnitEditWidget::queryModified() {
  bool ok = true;
  if(m_modified) {
    Bookcase* app = bookcaseParent(parent());
    QString str(i18n("The current book has been modified.\n"
                      "Do you want to enter the changes?"));
    int want_save = KMessageBox::warningYesNoCancel(app, str, i18n("Warning!"),
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
