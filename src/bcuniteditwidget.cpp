/***************************************************************************
                          bcuniteditwidget.cpp  -  description
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
#include "bclistview.h"
#include <klineedit.h>
#include <kcombobox.h>
#include <kcompletion.h>
#include <kbuttonbox.h>
#include <ktabctl.h>
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
#include <qwidgetstack.h>

// must be an even number
const int NCOLS = 4;

BCUnitEditWidget::BCUnitEditWidget(QWidget* parent_, const char* name_/*=0*/)
 : QWidget(parent_, name_) {
//  kdDebug() << "BCUnitEditWidget()" << endl;
  m_pages = new QWidgetStack(this);

  QVBoxLayout* l = new QVBoxLayout(this);
  l->addWidget(m_pages);

  m_editDict = new QDict<KLineEdit>();
  m_editDict->setAutoDelete(true);

  m_multiDict = new QDict<QMultiLineEdit>();
  m_multiDict->setAutoDelete(true);

  m_comboDict = new QDict<KComboBox>();
  m_comboDict->setAutoDelete(true);

  m_checkDict = new QDict<QCheckBox>();
  m_checkDict->setAutoDelete(true);
}

BCUnitEditWidget::~BCUnitEditWidget() {
//  kdDebug() << "~BCUnitEditWidget()" << endl;
  clearWidgets();
}

void BCUnitEditWidget::clearWidgets() {
//  kdDebug() << "BCUnitEditWidget::clearWidgets()" << endl;
  m_currUnit = NULL;
  m_currColl = NULL;

  delete m_checkDict;
  m_checkDict = NULL;
  delete m_comboDict;
  m_comboDict = NULL;
  delete m_editDict;
  m_editDict = NULL;
  delete m_multiDict;
  m_multiDict = NULL;
  delete m_pages;
  m_pages = NULL;
}

void BCUnitEditWidget::slotReset() {
//  kdDebug() << "BCUnitEditWidget::slotReset()" << endl;
  // clear all the contents not connected to the collection
  slotHandleClear();
  clearWidgets();
  kdDebug() << "BCUnitEditWidget::slotReset() - 2" << endl;
  m_pages = new QWidgetStack(this);
  static_cast<QVBoxLayout*>(layout())->addWidget(m_pages);
  m_pages->show();
//  static_cast<QVBoxLayout*>(layout())->invalidate();

//  // reset all the completion objects
//  QDictIterator<KLineEdit> it1(*m_editDict);
//  for( ; it1.current(); ++it1) {
//    if(! (m_currColl->attributeByName(it1.currentKey())->flags() & BCAttribute::DontComplete)) {
//      KCompletion* comp = it1.current()->completionObject();
//      comp->setItems(m_currColl->valuesByAttributeName(it1.currentKey()));
//    }
//  }

//  // need to also clear all comboboxes
//  QDictIterator<KComboBox> it2(*m_comboDict);
//  for( ; it2.current(); ++it2) {
//    QStringList strlist = m_currColl->attributeByName(it2.currentKey())->allowed();
//    it2.current()->clear();
//    it2.current()->insertItem(QString::null);
//    it2.current()->insertStringList(strlist);
//  }
}

void BCUnitEditWidget::slotAddPage(BCCollection* coll_) {
  if(!coll_) {
    return;
  }
//  kdDebug() << "BCUnitEditWidget::slotAddPage() - " << coll_->title() << endl;
  m_currColl = coll_;

  QWidget* w = new QWidget(m_pages);

  QVBoxLayout* topLayout = new QVBoxLayout(w);
  KTabCtl* tabs = new KTabCtl(w);

  KLineEdit* kl;
  QMultiLineEdit* mle;
  KComboBox* kc;
  QCheckBox* cb;

  QStringList groupList = coll_->attributeGroups();
  QStringList::ConstIterator groupIt = groupList.begin();
  for( ; groupIt != groupList.end(); ++groupIt) {
    QGrid* grid = new QGrid(NCOLS, w);
    grid->setMargin(10);
    grid->setSpacing(5);

    QList<BCAttribute> list = coll_->attributeListByGroup(static_cast<QString>(*groupIt));
    QListIterator<BCAttribute> it(list);
    for( ; it.current(); ++it) {
      QLabel* la = new QLabel(it.current()->title() + ":", grid);
      la->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

      switch(it.current()->type()) {
        case BCAttribute::Line:
          if(!m_editDict) {
            m_editDict = new QDict<KLineEdit>();
            m_editDict->setAutoDelete(true);
          }
          kl = new KLineEdit(QString::null, grid);
          if(! (it.current()->flags() & BCAttribute::DontComplete)) {
            kl->completionObject()->setItems(coll_->valuesByAttributeName(it.current()->name()));
            kl->setAutoDeleteCompletionObject(true);
          }
          m_editDict->insert(QString::number(coll_->id()) + it.current()->name(), kl);
          connect(kl, SIGNAL(returnPressed(const QString&)), SLOT(slotHandleReturn(const QString&)));
          break;

        case BCAttribute::Para:
          if(!m_multiDict) {
            m_multiDict = new QDict<QMultiLineEdit>();
            m_multiDict->setAutoDelete(true);
          }
          mle = new QMultiLineEdit(grid);
          mle->setWordWrap(QMultiLineEdit::WidgetWidth);
          m_multiDict->insert(QString::number(coll_->id()) + it.current()->name(), mle);
          break;

        case BCAttribute::Choice:
          if(!m_comboDict) {
            m_comboDict = new QDict<KComboBox>();
            m_comboDict->setAutoDelete(true);
          }
          kc = new KComboBox(grid);
          kc->insertItem(QString::null);
          kc->insertStringList(it.current()->allowed());
          kc->setEditable(false);
          m_comboDict->insert(QString::number(coll_->id()) + it.current()->name(), kc);
          break;

        case BCAttribute::Bool:
          if(!m_checkDict) {
            m_checkDict = new QDict<QCheckBox>();
            m_checkDict->setAutoDelete(true);
          }
          cb = new QCheckBox(grid);
          m_checkDict->insert(QString::number(coll_->id()) + it.current()->name(), cb);
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

  m_pages->addWidget(w, coll_->id());
  m_pages->raiseWidget(w);
  // the tabs gets squashed unless the minSize is increased
  m_pages->setMinimumSize(m_pages->sizeHint());
}

void BCUnitEditWidget::slotRemovePage(BCCollection* coll_) {
  if(!coll_) {
    return;
  }
  kdDebug() << "BCUnitEditWidget::slotRemovePage() - " << coll_->title() << endl;

  QWidget* w = m_pages->widget(coll_->id());

  // need to remove all entry widgets from Dicts
  QListIterator<BCAttribute> it(coll_->attributeList());
  for( ; it.current(); ++it) {
    switch(it.current()->type()) {
      case BCAttribute::Line:
        m_editDict->remove(QString::number(coll_->id()) + it.current()->name());
        break;

      case BCAttribute::Para:
        m_multiDict->remove(QString::number(coll_->id()) + it.current()->name());
        break;

      case BCAttribute::Choice:
        m_comboDict->remove(QString::number(coll_->id()) + it.current()->name());
        break;

      case BCAttribute::Bool:
        m_checkDict->remove(QString::number(coll_->id()) + it.current()->name());
        break;

      default:
        kdWarning() << "BCUnitEditWidget::slotRemovePage() - unknown attribute type\n";
        break;
    } //end switch
  } // end attribute loop

  m_pages->removeWidget(w);
  delete w;
}

void BCUnitEditWidget::slotHandleReturn(const QString& text_) {
  //slotHandleSave();
}

void BCUnitEditWidget::slotHandleNew() {
  // if we've made a new one and not added it, we need to delete it
  if(m_currUnit && m_currUnit->collection()->unitList().containsRef(m_currUnit) == 0) {
    delete m_currUnit;
  }

  slotHandleClear();
  m_currUnit = new BCUnit(m_currColl);
}

void BCUnitEditWidget::slotHandleCopy() {
  if(m_currUnit && m_currUnit->collection()->unitList().containsRef(m_currUnit) > 0) {
    slotSetContents(new BCUnit(*m_currUnit));
    slotHandleSave();
  }
}

void BCUnitEditWidget::slotHandleSave() {
  BCUnit* unit;

  // check to see if we hold a pointer already, if not allocate new object
  if(m_currUnit) {
    unit = m_currUnit;
  } else {
    unit = new BCUnit(m_currColl);
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
        kl = m_editDict->find(QString::number(m_currColl->id()) + it.current()->name());
        if(kl) {
          temp = kl->text().simplifyWhiteSpace();
          // ok to set attribute empty string
          unit->setAttribute(it.current()->name(), temp);
          if(!temp.isEmpty() && !(it.current()->flags() & BCAttribute::DontComplete)) {
            kl->completionObject()->addItem(temp);
            empty = false;
          }
        } else {
          kdDebug() << "BCUnitEditWidget::slotHandleSave() - no lineedit object!\n";
        }
        break;

      case BCAttribute::Para:
        mle = m_multiDict->find(QString::number(m_currColl->id()) + it.current()->name());
        if(mle) {
          temp = mle->text().simplifyWhiteSpace();
          unit->setAttribute(it.current()->name(), temp);
          if(!temp.isEmpty()) {
            empty = false;
          }
        } else {
          kdDebug() << "BCUnitEditWidget::slotHandleSave() - no multilineedit object!\n";
        }
        break;

      case BCAttribute::Choice:
        kc = m_comboDict->find(QString::number(m_currColl->id()) + it.current()->name());
        if(kc) {
          temp = kc->currentText().simplifyWhiteSpace();
          // ok to set attribute empty
          unit->setAttribute(it.current()->name(), temp);
          if(!temp.isEmpty()) {
            empty = false;
          }
        } else {
          kdDebug() << "BCUnitEditWidget::slotHandleSave() - no combobox object!\n";
        }
        break;

      case BCAttribute::Bool:
        cb = m_checkDict->find(QString::number(m_currColl->id()) + it.current()->name());
        if(cb) {
          if(cb->isChecked()) {
            unit->setAttribute(it.current()->name(), "1");
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
    emit signalDoUnitSave(unit);
    // slotHandleClear();
  }
}

void BCUnitEditWidget::slotHandleDelete() {
  if(m_currUnit && m_currUnit->collection()->unitList().containsRef(m_currUnit) > 0) {
    kdDebug() << "BCUnitEditWidget::slotHandleDelete() - item " << m_currUnit->attribute("title") << endl;
    emit signalDoUnitDelete(m_currUnit);
  }

  // clear the widget whether or not we deleted anything
  slotHandleClear();
}

void BCUnitEditWidget::slotHandleClear() {
  kdDebug() << "BCUnitEditWidget::slotHandleClear()" << endl;
  // clear the linedits
  if(m_editDict) {
    QDictIterator<KLineEdit> it1(*m_editDict);
    for( ; it1.current(); ++it1) {
      it1.current()->clear();
    }
  }

  // clear the linedits
  if(m_multiDict) {
    QDictIterator<QMultiLineEdit> it2(*m_multiDict);
    for( ; it2.current(); ++it2) {
      it2.current()->clear();
    }
  }

  // set all the comboboxes to the first item, which should be the null string
  if(m_comboDict) {
    QDictIterator<KComboBox> it3(*m_comboDict);
    for( ; it3.current(); ++it3) {
      it3.current()->setCurrentItem(0);
    }
  }

  // clear the checkboxes, too
  if(m_checkDict) {
    QDictIterator<QCheckBox> it4(*m_checkDict);
    for( ; it4.current(); ++it4) {
      it4.current()->setChecked(false);
    }
  }

  // nullify the pointer...
  m_currUnit = NULL;
}

void BCUnitEditWidget::slotSetContents(BCUnit* unit_) {
  if(!unit_) {
    return;
  }
  kdDebug() << "BCUnitEditWidget::slotSetContents() - " << unit_->attribute("title") << endl;
  m_currUnit = unit_;
  m_currColl = unit_->collection();
  QWidget* w = m_pages->widget(m_currColl->id());
  // should never happen, but JIT
  if(!w) {
    slotAddPage(m_currColl);
    w = m_pages->widget(m_currColl->id());
  }
  if(w != m_pages->visibleWidget()) {
    m_pages->raiseWidget(w);
  }

  KLineEdit* kl;
  QMultiLineEdit* mle;
  KComboBox* kc;
  QCheckBox* cb;
  QListIterator<BCAttribute> it(m_currColl->attributeList());
  for( ; it.current(); ++it) {
    switch(it.current()->type()) {
      case BCAttribute::Line:
        kl = m_editDict->find(QString::number(m_currColl->id()) + it.current()->name());
        if(kl) {
          kl->setText(m_currUnit->attribute(it.current()->name()));
        }
        break;

      case BCAttribute::Para:
        mle = m_multiDict->find(QString::number(m_currColl->id()) + it.current()->name());
        break;

      case BCAttribute::Choice:
        kc = m_comboDict->find(QString::number(m_currColl->id()) + it.current()->name());
        for(int i = 0; kc && i < kc->count(); i++) {
          if(kc->text(i) == m_currUnit->attribute(it.current()->name())) {
            kc->setCurrentItem(i);
            break;
          }
        }
        break;

      case BCAttribute::Bool:
        cb = m_checkDict->find(QString::number(m_currColl->id()) + it.current()->name());
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
