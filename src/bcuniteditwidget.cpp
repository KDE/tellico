/***************************************************************************
                            bcuniteditwidget.cpp
                             -------------------
    begin                : Wed Sep 26 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@radiojodi.com
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
#include "bcattribute.h"
#include "bccolumnview.h"
#include "bctabcontrol.h"

#include <klineedit.h>
#include <kcombobox.h>
#include <kcompletion.h>
#include <kbuttonbox.h>
//#include <ktabctl.h>
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
#include <qwidgetstack.h>
#include <qobjectlist.h>

// must be an even number
const int NCOLS = 4;

BCUnitEditWidget::BCUnitEditWidget(QWidget* parent_, const char* name_/*=0*/)
 : QWidget(parent_, name_) {
//  kdDebug() << "BCUnitEditWidget()" << endl;

  m_stack = new QWidgetStack(this);
  QVBoxLayout* l = new QVBoxLayout(this);
  l->addWidget(m_stack);
}

BCUnitEditWidget::~BCUnitEditWidget() {
//  kdDebug() << "~BCUnitEditWidget()" << endl;
  clearWidgets();
}

void BCUnitEditWidget::clearWidgets() {
  kdDebug() << "BCUnitEditWidget::clearWidgets()" << endl;
  m_currUnit = NULL;
  m_currColl = NULL;

  delete m_stack;
  m_stack = NULL;

  // need to nullify button pointers
  m_new = NULL;
  m_copy = NULL;
  m_save = NULL;
  m_delete = NULL;
  m_clear = NULL;
  
  // clear all the dicts
  m_editDict.clear();
  m_multiDict.clear();
  m_comboDict.clear();
  m_checkDict.clear();
}

void BCUnitEditWidget::slotReset() {
  kdDebug() << "BCUnitEditWidget::slotReset()" << endl;
  clearWidgets();

  m_stack = new QWidgetStack(this);
  static_cast<QVBoxLayout*>(layout())->addWidget(m_stack);
  m_stack->show();
}

void BCUnitEditWidget::slotAddPage(BCCollection* coll_) {
  if(!coll_) {
    return;
  }
//  kdDebug() << "BCUnitEditWidget::slotAddPage() - " << coll_->title() << endl;
  m_currColl = coll_;

  QWidget* w = new QWidget(m_stack);

  QVBoxLayout* topLayout = new QVBoxLayout(w);
  // need to include a name since QObject::child(name) is called later
  BCTabControl* tabs = new BCTabControl(w, "tabs");
  connect(tabs, SIGNAL(tabSelected(int)), SLOT(slotSwitchFocus(int)));

  KLineEdit* kl;
  QMultiLineEdit* mle;
  KComboBox* kc;
  QCheckBox* cb;

  QStringList groupList = coll_->attributeGroups();
  QStringList::ConstIterator groupIt = groupList.begin();
  for( ; groupIt != groupList.end(); ++groupIt) {
    QGrid* grid = new QGrid(NCOLS, tabs);
    grid->setMargin(10);
    grid->setSpacing(5);

    QList<BCAttribute> list = coll_->attributeListByGroup(static_cast<QString>(*groupIt));
    QListIterator<BCAttribute> it(list);
    for( ; it.current(); ++it) {
      QLabel* la = new QLabel(it.current()->title() + ":", grid);
      la->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

      switch(it.current()->type()) {
        case BCAttribute::Line:
          kl = new KLineEdit(QString::null, grid);
          if(! (it.current()->flags() & BCAttribute::DontComplete)) {
            kl->completionObject()->setItems(coll_->valuesByAttributeName(it.current()->name()));
            kl->setAutoDeleteCompletionObject(true);
          }
          m_editDict.insert(QString::number(coll_->id()) + it.current()->name(), kl);
          connect(kl, SIGNAL(returnPressed(const QString&)), SLOT(slotHandleReturn(const QString&)));
          break;

        case BCAttribute::Para:
          mle = new QMultiLineEdit(grid);
          mle->setWordWrap(QMultiLineEdit::WidgetWidth);
          m_multiDict.insert(QString::number(coll_->id()) + it.current()->name(), mle);
          break;

        case BCAttribute::Choice:
          kc = new KComboBox(grid);
          kc->insertItem(QString::null);
          kc->insertStringList(it.current()->allowed());
          kc->setEditable(false);
          m_comboDict.insert(QString::number(coll_->id()) + it.current()->name(), kc);
          break;

        case BCAttribute::Bool:
          cb = new QCheckBox(grid);
          m_checkDict.insert(QString::number(coll_->id()) + it.current()->name(), cb);
          break;

        default:
         kdDebug() << "BCUnitEditWidget() - unknown attribute type\n";
         break;
      } // end switch
    }
    tabs->addTab(grid, static_cast<QString>(*groupIt));
  }

  topLayout->setSpacing(5);
  topLayout->setMargin(5);
  // stretch = 1 so that the tabs expand vertically
  topLayout->addWidget(tabs, 1);

  KButtonBox* bb = new KButtonBox(w);
  m_new = bb->addButton(i18n("New"), this, SLOT(slotHandleNew()));
  m_copy = bb->addButton(i18n("Copy"), this, SLOT(slotHandleCopy()));
  m_save = bb->addButton(i18n("Save"), this, SLOT(slotHandleSave()));
  m_delete = bb->addButton(i18n("Delete"), this, SLOT(slotHandleDelete()));
  m_clear = bb->addButton(i18n("Clear"), this, SLOT(slotHandleClear()));
  // stretch = 0, so the height of the buttonbox is constant
  topLayout->addWidget(bb, 0, Qt::AlignBottom | Qt::AlignHCenter);

  m_stack->addWidget(w, coll_->id());
  m_stack->raiseWidget(w);
  // the tabs gets squashed unless the minSize is increased
  m_stack->setMinimumSize(m_stack->sizeHint());

  // no currUnit exists, so disable the Copy and Delete button
  m_copy->setEnabled(false);
  m_delete->setEnabled(false);
}

void BCUnitEditWidget::slotRemovePage(BCCollection* coll_) {
  if(!coll_) {
    return;
  }
  kdDebug() << "BCUnitEditWidget::slotRemovePage() - " << coll_->title() << endl;

  QWidget* w = m_stack->widget(coll_->id());

  // need to remove all entry widgets from Dicts
  QListIterator<BCAttribute> it(coll_->attributeList());
  for( ; it.current(); ++it) {
    switch(it.current()->type()) {
      case BCAttribute::Line:
        m_editDict.remove(QString::number(coll_->id()) + it.current()->name());
        break;

      case BCAttribute::Para:
        m_multiDict.remove(QString::number(coll_->id()) + it.current()->name());
        break;

      case BCAttribute::Choice:
        m_comboDict.remove(QString::number(coll_->id()) + it.current()->name());
        break;

      case BCAttribute::Bool:
        m_checkDict.remove(QString::number(coll_->id()) + it.current()->name());
        break;

      default:
        kdWarning() << "BCUnitEditWidget::slotRemovePage() - unknown attribute type\n";
        break;
    } //end switch
  } // end attribute loop

  m_stack->removeWidget(w);
  delete w;
}

void BCUnitEditWidget::slotHandleReturn(const QString&) {
  //slotHandleSave();
}

void BCUnitEditWidget::slotHandleNew() {
  // if we've made a new one and not added it, we need to delete it
  if(m_currUnit && m_currColl->unitList().containsRef(m_currUnit) == 0) {
    delete m_currUnit;
  }

  slotHandleClear();
  m_currUnit = new BCUnit(m_currColl);
}

void BCUnitEditWidget::slotHandleCopy() {
  // if the currUnit exists and has already been saved
  // TODO: if the attribute values have been changed without clicking save
  // need to ask use for confirmation
  if(m_currUnit && m_currColl->unitList().containsRef(m_currUnit) > 0) {
    m_currUnit = new BCUnit(*m_currUnit);
    // slotHandleSave() clears everthing, so need to keep a pointer
    BCUnit* unit = m_currUnit;
    slotHandleSave();
    // let's be nice and put everything back in there
    slotSetContents(unit);
  }
}

void BCUnitEditWidget::slotHandleSave() {
  // check to see if we hold a pointer already, if not allocate new object
  if(!m_currUnit) {
    m_currUnit = new BCUnit(m_currColl);
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
        kdWarning() << "BCUnitEditWidget::slotHandleSave() - unknown attribute type\n";
        break;
    } // end switch
  }

  // if something was not empty, signal a save, and then clear everything
  if(!empty) {
    emit signalDoUnitSave(m_currUnit);
    slotHandleClear();
  }

  // find the tabs widget...it's a child, named "tabs"
  // "BCTabControl" probably not necessary, but good check
  QObject* tabs = m_stack->visibleWidget()->child("tabs", "BCTabControl");
  if(tabs) {
    //flip to first tab
    static_cast<BCTabControl*>(tabs)->showTab(0);
  }
}

void BCUnitEditWidget::slotHandleDelete() {
  if(m_currUnit && m_currUnit->collection()->unitList().containsRef(m_currUnit) > 0) {
    kdDebug() << "BCUnitEditWidget::slotHandleDelete() - item " << m_currUnit->attribute("title") << endl;
    // this widget does not actually delete the unit
    emit signalDoUnitDelete(m_currUnit);
  }

  // clear the widget whether or not anything was deleted
  slotHandleClear();
}

void BCUnitEditWidget::slotHandleClear() {
  kdDebug() << "BCUnitEditWidget::slotHandleClear()" << endl;
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

  // nullify the pointer...
  m_currUnit = NULL;
  // disable the copy and delete buttons
  m_copy->setEnabled(false);
  m_delete->setEnabled(false);
}

void BCUnitEditWidget::slotSetContents(BCUnit* unit_) {
  if(!unit_) {
    return;
  }
  kdDebug() << "BCUnitEditWidget::slotSetContents() - " << unit_->attribute("title") << endl;
  m_currUnit = unit_;
  m_currColl = unit_->collection();
  // enable copy and delete buttons
  if(m_copy) { // if m_copy exists, so does m_delete
    m_copy->setEnabled(true);
    m_delete->setEnabled(true);
  }

  QWidget* w = m_stack->widget(m_currColl->id());
  // should never happen, but JIC
  if(!w) {
    slotAddPage(unit_->collection());
    w = m_stack->widget(m_currColl->id());
  }
  if(w != m_stack->visibleWidget()) {
    m_stack->raiseWidget(w);
  }

  // find the tabs widget...it's a child, named "tabs"
  // "BCTabControl" probably not necessary, but good check
  QObject* tabs = w->child("tabs", "BCTabControl");
  if(tabs) {
    // flip to first tab
    static_cast<BCTabControl*>(tabs)->showTab(0);
  }
  
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
          kl->setText(m_currUnit->attribute(it.current()->name()));
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
        kdWarning() << "BCUnitEditWidget::slotSetContents() - unknown attribute type" << endl;
        break;
    } //end switch
  } // end attribute loop
}

void BCUnitEditWidget::slotUpdateCompletions(BCUnit* unit_) {
  QListIterator<BCAttribute> it(unit_->collection()->attributeList());
  for( ; it.current(); ++it) {
    if(it.current()->type() == BCAttribute::Line
                  && !(it.current()->flags() & BCAttribute::DontComplete)) {
      QString key = QString::number(unit_->collection()->id()) + it.current()->name();
      KLineEdit* kl = m_editDict.find(key);
      if(kl) {
        QString temp = unit_->attribute(it.current()->name());
        kl->completionObject()->addItem(temp);
      }
    }
  }
}

void BCUnitEditWidget::slotSwitchFocus(int tabNum_) {
  // tabNum_ isn't really needed, the visible one is assumed to be the one called
  // first get the tab widget
  BCTabControl* tabs = static_cast<BCTabControl*>(m_stack->visibleWidget());
  // the line edits are all children of the QGrid on the page
//  QWidget* w = tabs->currentPage();
//  if(!w) {
//    return;
//  }
//  QObjectList* l = w->queryList("KLineEdit");
//  kdDebug() << "BCU::slotSwitchFocus() - " << l->count() << " line edits on the page." << endl;
}
