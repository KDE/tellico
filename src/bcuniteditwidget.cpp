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
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "bcuniteditwidget.h"
#include "bcunit.h"
#include "bccollection.h"
#include "bcattribute.h"
#include "bcdetailedlistview.h"
#include "bctabcontrol.h"
#include "isbnvalidator.h"

#include <klineedit.h>
#include <kcombobox.h>
#include <kcompletion.h>
#include <kbuttonbox.h>
#include <klocale.h>
#include <kdebug.h>

#include <qlayout.h>
#include <qcheckbox.h>
#include <qstringlist.h>
#include <qlabel.h>
#include <qmultilineedit.h>
#include <qlistview.h>
#include <qlist.h>
#include <qdict.h>
#include <qgrid.h>
#include <qpushbutton.h>
#include <qwhatsthis.h>

// must be an even number
static const int NCOLS = 4;

BCUnitEditWidget::BCUnitEditWidget(QWidget* parent_, const char* name_/*=0*/)
 : QWidget(parent_, name_) {
//  kdDebug() << "BCUnitEditWidget()" << endl;
  QVBoxLayout* topLayout = new QVBoxLayout(this);
  m_tabs = new BCTabControl(this);
  connect(m_tabs, SIGNAL(tabSelected(int)), SLOT(slotSwitchFocus(int)));

  topLayout->setSpacing(5);
  topLayout->setMargin(5);
  // stretch = 1 so that the tabs expand vertically
  topLayout->addWidget(m_tabs, 1);

  KButtonBox* bb = new KButtonBox(this);
  m_new = bb->addButton(i18n("New Book"), this, SLOT(slotHandleNew()));
//  m_copy = bb->addButton(i18n("Copy"), this, SLOT(slotHandleCopy()));
  m_save = bb->addButton(i18n("Enter Book"), this, SLOT(slotHandleSave()));
  m_delete = bb->addButton(i18n("Delete Book"), this, SLOT(slotHandleDelete()));
  m_clear = bb->addButton(i18n("Clear Data"), this, SLOT(slotHandleClear()));
  // stretch = 0, so the height of the buttonbox is constant
  topLayout->addWidget(bb, 0, Qt::AlignBottom | Qt::AlignHCenter);

  // no currUnit exists, so disable the Copy and Delete button
//  m_copy->setEnabled(false);
  m_delete->setEnabled(false);

  m_currUnit = 0;
  m_currColl = 0;
}

BCUnitEditWidget::~BCUnitEditWidget() {
//  if(m_currUnit && m_currUnit->collection() && m_currUnit->collection()->unitList().containsRef(m_currUnit) == 0) {
//    // this means we created a pointer but haven't added it to the collection yet
//    // TODO: ask user if he wants to save it
//    delete m_currUnit;
//  }
//  kdDebug() << "~BCUnitEditWidget()" << endl;
}

void BCUnitEditWidget::slotReset() {
  kdDebug() << "BCUnitEditWidget::slotReset()" << endl;

//  if(m_currUnit && m_currUnit->collection() && m_currUnit->collection()->unitList().containsRef(m_currUnit) == 0) {
//    // this means we created a pointer but haven't added it to the collection yet
//    // TODO: ask user if he wants to save it
//    delete m_currUnit;
//  }

  m_currUnit = 0;
  m_delete->setEnabled(false);
  m_save->setText(i18n("Enter Book"));
  m_currColl = 0;

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

  if(m_tabs->count() == 0) {
    slotSetLayout(coll_);
  }
}

void BCUnitEditWidget::slotSetLayout(BCCollection* coll_) {
  if(!coll_) {
    return;
  }
//  kdDebug() << "BCUnitEditWidget::slotSetLayout() - " << coll_->title() << endl;
//  slotSetCollection(coll_);

  if(m_tabs->count() > 0) {
    kdDebug() << "BCUnitEditWidget::slotSetLayout() - tabs already exist." << endl;
    return;
  }

  KLineEdit* kl;
  QMultiLineEdit* mle;
  KComboBox* kc;
  QCheckBox* cb;

  QStringList groupList = m_currColl->attributeGroups();
  QStringList::ConstIterator groupIt = groupList.begin();
  for( ; groupIt != groupList.end(); ++groupIt) {
    QGrid* grid = new QGrid(NCOLS, m_tabs);
    grid->setMargin(10);
    grid->setSpacing(5);

    QList<BCAttribute> list = m_currColl->attributeListByGroup(static_cast<QString>(*groupIt));
    QListIterator<BCAttribute> it(list);
    for( ; it.current(); ++it) {
      QLabel* la = new QLabel(it.current()->title() + ":", grid);
      la->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      QWhatsThis::add(la, it.current()->description());

      switch(it.current()->type()) {
        case BCAttribute::Line:
          kl = new KLineEdit(QString::null, grid);
          QWhatsThis::add(kl, it.current()->description());
          if(! (it.current()->flags() & BCAttribute::DontComplete)) {
            kl->completionObject()->setItems(m_currColl->valuesByAttributeName(it.current()->name()));
            kl->setAutoDeleteCompletionObject(true);
          }
          if(it.current()->name() == "isbn") {
            ISBNValidator* val = new ISBNValidator(this);
            kl->setValidator(val);
          }
          m_editDict.insert(QString::number(m_currColl->id()) + it.current()->name(), kl);
          connect(kl, SIGNAL(returnPressed(const QString&)), SLOT(slotHandleReturn(const QString&)));
          break;

        case BCAttribute::Para:
          mle = new QMultiLineEdit(grid);
          mle->setWordWrap(QMultiLineEdit::WidgetWidth);
          QWhatsThis::add(mle, it.current()->description());
          m_multiDict.insert(QString::number(m_currColl->id()) + it.current()->name(), mle);
          break;

        case BCAttribute::Choice:
          kc = new KComboBox(grid);
          QWhatsThis::add(kc, it.current()->description());
          // always have empty choice
          kc->insertItem(QString::null);
          kc->insertStringList(it.current()->allowed());
          kc->setEditable(false);
          m_comboDict.insert(QString::number(m_currColl->id()) + it.current()->name(), kc);
          break;

        case BCAttribute::Bool:
          cb = new QCheckBox(grid);
          QWhatsThis::add(cb, it.current()->description());
          m_checkDict.insert(QString::number(m_currColl->id()) + it.current()->name(), cb);
          break;

        default:
          kdDebug() << "BCUnitEditWidget() - unknown attribute type  ("
            "" << it.current()->type() << ") named " << it.current()->name() << endl;
         break;
      } // end switch
    }
    m_tabs->addTab(grid, static_cast<QString>(*groupIt));
  }

  // maybe this should be sizePolicy() reimplemented instead
//  setMinimumHeight(minimumSizeHint().height());
//  setMaximumHeight(minimumSizeHint().height());
  setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));

  // no currUnit exists, so disable the Copy and Delete button
//  m_copy->setEnabled(false);
  m_delete->setEnabled(false);
}
void BCUnitEditWidget::slotHandleReturn(const QString&) {
  //slotHandleSave();
}

void BCUnitEditWidget::slotHandleNew() {
  slotHandleClear();
  m_currUnit = new BCUnit(m_currColl);
  m_save->setText(i18n("Enter Book"));
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
  QMultiLineEdit* mle;
  KComboBox* kc;
  QCheckBox* cb;
  QString temp;
  QListIterator<BCAttribute> it(m_currColl->attributeList());
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
        } else {
          kdDebug() << "BCUnitEditWidget::slotHandleSave() - no lineedit object!\n";
        }
        break;

      case BCAttribute::Para:
        mle = m_multiDict.find(QString::number(m_currColl->id()) + it.current()->name());
        if(mle) {
          temp = mle->text().simplifyWhiteSpace();
          m_currUnit->setAttribute(it.current()->name(), temp);
          if(!temp.isEmpty()) {
            empty = false;
          }
        } else {
          kdDebug() << "BCUnitEditWidget::slotHandleSave() - no multilineedit object!\n";
        }
        break;

      case BCAttribute::Choice:
        kc = m_comboDict.find(QString::number(m_currColl->id()) + it.current()->name());
        if(kc) {
          temp = kc->currentText().simplifyWhiteSpace();
          // ok to set attribute empty
          m_currUnit->setAttribute(it.current()->name(), temp);
          if(!temp.isEmpty()) {
            empty = false;
          }
        } else {
          kdDebug() << "BCUnitEditWidget::slotHandleSave() - no combobox object!\n";
        }
        break;

      case BCAttribute::Bool:
        cb = m_checkDict.find(QString::number(m_currColl->id()) + it.current()->name());
        if(cb) {
          if(cb->isChecked()) {
            m_currUnit->setAttribute(it.current()->name(), "1");
            empty = false;
          }
        } else {
          kdDebug() << "BCUnitEditWidget::slotHandleSave() - no checkbox object!\n";
        }
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
    emit signalSaveUnit(m_currUnit);
  }
  slotHandleClear();

  // go back to first tab, with title, etc...
  m_tabs->showTab(0);
}

void BCUnitEditWidget::slotHandleDelete() {
  if(m_currUnit && m_currColl->unitList().containsRef(m_currUnit) > 0) {
    kdDebug() << "BCUnitEditWidget::slotHandleDelete() - item " << m_currUnit->attribute("title") << endl;
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
  QDictIterator<QMultiLineEdit> it2(m_multiDict);
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
//  m_copy->setEnabled(false);
  m_delete->setEnabled(false);
  m_save->setText(i18n("Enter Book"));
}

void BCUnitEditWidget::slotSetContents(BCUnit* unit_) {
  if(!unit_) {
    slotHandleClear();
    return;
  }
//  kdDebug() << "BCUnitEditWidget::slotSetContents() - " << unit_->attribute("title") << endl;
  m_currUnit = unit_;
//  m_currColl = unit_->collection();
  if(m_currColl != unit_->collection()) {
    kdDebug() << "BCUnitEditWidget::slotSetContents() - collections don't match" << endl;
    m_currColl = unit_->collection();
  }
  
  // enable copy and delete buttons
//  m_copy->setEnabled(true);
  m_delete->setEnabled(true);

  m_tabs->showTab(0);

  KLineEdit* kl;
  QMultiLineEdit* mle;
  KComboBox* kc;
  QCheckBox* cb;
  QListIterator<BCAttribute> it(m_currColl->attributeList());
  for( ; it.current(); ++it) {
    switch(it.current()->type()) {
      case BCAttribute::Line:
        kl = m_editDict.find(QString::number(m_currColl->id()) + it.current()->name());
        if(kl) {
          kl->setText(unit_->attribute(it.current()->name()));
        } else {
          kdDebug() << "BCUnitEditWidget:slotSetContents() - no line edit found for " << it.current()->name() << endl;
        }
        break;

      case BCAttribute::Para:
        mle = m_multiDict.find(QString::number(m_currColl->id()) + it.current()->name());
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
        if(cb && !m_currUnit->attribute(it.current()->name()).isEmpty()) {
          cb->setChecked(true);
        }
        break;

      default:
        kdWarning() << "BCUnitEditWidget::slotSetContents() - unknown attribute type ("
          "" << it.current()->type() << ") named " << it.current()->name() << endl;
        break;
    } //end switch
  } // end attribute loop
  if(m_currUnit && m_currColl->unitList().containsRef(m_currUnit) > 0) {
    m_save->setText(i18n("Modify Book"));
  }
}

void BCUnitEditWidget::slotUpdateCompletions(BCUnit* unit_) {
  QListIterator<BCAttribute> it(unit_->collection()->attributeList());
  for( ; it.current(); ++it) {
    if(it.current()->type() == BCAttribute::Line
                  && !(it.current()->flags() & BCAttribute::DontComplete)) {
      QString key = QString::number(unit_->collection()->id()) + it.current()->name();
      KLineEdit* kl = m_editDict.find(key);
      if(kl) {
        QString value = unit_->attribute(it.current()->name());
        kl->completionObject()->addItem(value);
      }
    }
  }
}

void BCUnitEditWidget::slotSwitchFocus(int tabNum_) {
  m_tabs->setFocusToLineEdit(tabNum_);
}
