/***************************************************************************
                          bccollectionview.cpp  -  description
                             -------------------
    begin                : Sat Oct 13 2001
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

#include "bccollectionview.h"
#include "bccollection.h"
#include "bcattribute.h"
#include "bcunititem.h"
#include <kpopupmenu.h>
#include <klineeditdlg.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qlist.h>
#include <qcolor.h>

BCCollectionView::BCCollectionView(QWidget* parent_, const char* name_/*=0*/)
 : KListView(parent_, name_) {
  //kdDebug() << "BCCollectionView()" << endl;
  addColumn(i18n("Bookcase"));
  setRootIsDecorated(true);
  setTreeStepSize(10);
  // turn off the alternate background color
  setAlternateBackground(QColor());

  m_groupDict = new QDict<ParentItem>();
  //m_groupDict->setAutoDelete(true);

  m_menu = new KPopupMenu(this);
  KIconLoader* loader = KGlobal::iconLoader();
  QPixmap rename, expand, collapse;
  if(loader) {
    rename = loader->loadIcon("editclear", KIcon::Small);
    expand = loader->loadIcon("2downarrow", KIcon::Small);
    collapse = loader->loadIcon("2uparrow", KIcon::Small);

    m_groupOpen = loader->loadIcon("folder_red_open", KIcon::Small);
    m_groupClosed = loader->loadIcon("folder_red", KIcon::Small);
  }
  m_menu->insertItem(rename, i18n("Rename"), this, SLOT(slotHandleRename()));
  m_menu->insertItem(expand, i18n("Expand All"), this, SLOT(slotExpandAll()));
  m_menu->insertItem(collapse, i18n("Collapse All"), this, SLOT(slotCollapseAll()));

  connect(this, SIGNAL(rightButtonPressed(QListViewItem*, const QPoint&, int)),
          SLOT(slotRMB(QListViewItem*, const QPoint&, int)));

//  connect(this, SIGNAL(clicked(QListViewItem*)), SLOT(slotSelected(QListViewItem*)));

  connect(this, SIGNAL(selectionChanged(QListViewItem*)), SLOT(slotSelected(QListViewItem*)));

  connect(this, SIGNAL(doubleClicked(QListViewItem*)), SLOT(slotToggleItem(QListViewItem*)));

  connect(this, SIGNAL(expanded(QListViewItem*)), SLOT(slotExpanded(QListViewItem*)));

  connect(this, SIGNAL(collapsed(QListViewItem*)), SLOT(slotCollapsed(QListViewItem*)));
}

BCCollectionView::~BCCollectionView() {
  delete m_groupDict;
  m_groupDict = NULL;
}

void BCCollectionView::insertItem(BCAttribute* att_, ParentItem* root_, BCUnit* unit_) {
  if(!att_ || !unit_) {
    return;
  }

  // possible for unit to be in multiple groups, so use stringlist
  int flags = att_->flags();
  QStringList groups;
  QString attValue = unit_->attribute(att_->name());
  if((flags & BCAttribute::AllowMultiple) && !attValue.isEmpty()) {
    groups = QStringList::split(";", attValue);
  } else if(!attValue.isEmpty()) {
    groups << attValue;
  } else {
    groups << i18n("(None)");
  }

  KIconLoader* loader = KGlobal::iconLoader();
  QPixmap icon;
  if(loader) {
    icon = loader->loadIcon(unit_->collection()->iconName(), KIcon::User);
  }

  // guaranteed to have a "title" attribute
  QString title = unit_->attribute("title");
  QStringList::Iterator groupIt;
  for(groupIt = groups.begin() ; groupIt != groups.end(); ++groupIt) {
    QString group = static_cast<QString>(*groupIt).simplifyWhiteSpace();
    if(flags & BCAttribute::FormatTitle) {
      group = BCAttribute::formatTitle(group);
    } else if(flags & BCAttribute::FormatName) {
      group = BCAttribute::formatName(group);
    } else if(flags & BCAttribute::FormatDate) {
      group = BCAttribute::formatDate(group);
    }

    ParentItem* par = m_groupDict->find(QString::number(unit_->collection()->id()) + group);
    if(!par) {
      par = new ParentItem(root_, group);
      par->setPixmap(0, m_groupClosed);
      m_groupDict->insert(QString::number(unit_->collection()->id()) + group, par);
    }
    BCUnitItem* item =  new BCUnitItem(par, BCAttribute::formatTitle(title), unit_);
    item->setPixmap(0, icon);
    ensureItemVisible(item);
    par->setOpen(true);
  }
}

QList<BCUnitItem> BCCollectionView::locateItem(BCUnit* unit_) {
//  kdDebug() << "BCCollectionView::locateItem() - " << unit_->attribute("title") << endl;
  QList<BCUnitItem> list;
  // iterate over the collections, which are the top-level children
  for(QListViewItem* collItem = firstChild(); collItem; collItem = collItem->nextSibling()) {

    // find the collItem matching the unit's collection and iterate over all its children
    if(static_cast<ParentItem*>(collItem)->id() == unit_->collection()->id()) {
      QListViewItemIterator it(collItem);
      for( ; it.current(); ++it) {

        // unitItems always have a depth of 2
        if(it.current()->depth() == 2) {
          if(static_cast<BCUnitItem*>(it.current())->unit() == unit_) {
            list.append(static_cast<BCUnitItem*>(it.current()));
          }
        }
      } // end UnitItem for loop
    }
  } // end collection for loop
  return list;
}

ParentItem* BCCollectionView::locateItem(BCCollection* coll_) {
  ParentItem* root = NULL;
  // iterate over the collections, which are the top-level children
  for(QListViewItem* collItem = firstChild(); collItem; collItem = collItem->nextSibling()) {
    // find the collItem matching the unit's collection and insert item inside
    if(static_cast<ParentItem*>(collItem)->id() == coll_->id()) {
      root = static_cast<ParentItem*>(collItem);
      break;
    }
  }
  if(!root) {
    root = new ParentItem(this, coll_->title(), coll_->id());
  }
  return root;
}

void BCCollectionView::slotReset() {
  m_groupDict->clear();
  clear();
}

void BCCollectionView::slotAddItem(BCCollection* coll_) {
  if(!coll_) {
    return;
  }

//  kdDebug() << "BCCollectionView::slotAddItem() - collection: " << coll_->name() << endl;
  ParentItem* collItem = new ParentItem(this, coll_->title(), coll_->id());
  if(coll_->unitCount() == 0) {
    return;
  }

  QListIterator<BCUnit> unitIt(coll_->unitList());
  for( ; unitIt.current(); ++unitIt) {
    insertItem(coll_->groupAttribute(), collItem, unitIt.current());
  }
  setSelected(collItem, true);
  slotCollapseAll();
  ensureItemVisible(collItem);
  collItem->setOpen(true);
}

void BCCollectionView::slotRemoveItem(BCCollection* coll_) {
  if(!coll_) {
    return;
  }

//  kdDebug() << "BCCollectionView::slotRemoveItem() - " << unit_->attribute("title") << endl;
  ParentItem* collItem = locateItem(coll_);
  // first remove all groups in collection from dict
  QListViewItem* groupItem = collItem->firstChild();
  for( ; groupItem; groupItem = groupItem->nextSibling()) {
    m_groupDict->remove(QString::number(coll_->id()) + groupItem->text(0));
  }
  // automatically deletes all children
  delete collItem;
}

void BCCollectionView::slotAddItem(BCUnit* unit_) {
  if(!unit_) {
    return;
  }

//  kdDebug() << "BCCollectionView::slotAddItem() - " << unit_->attribute("title") << endl;

  ParentItem* root = locateItem(unit_->collection());
  insertItem(unit_->collection()->groupAttribute(), root, unit_);
}

void BCCollectionView::slotModifyItem(BCUnit* unit_) {
  if(!unit_) {
    return;
  }

  BCAttribute* att = unit_->collection()->groupAttribute();
  if(!att) {
    return;
  }

  int flags = att->flags();
  QString groupText = unit_->attribute(att->name());
  if(flags & BCAttribute::FormatTitle) {
    groupText = BCAttribute::formatTitle(groupText);
  } else if(flags & BCAttribute::FormatName) {
    groupText = BCAttribute::formatName(groupText);
  } else if(flags & BCAttribute::FormatDate) {
    groupText = BCAttribute::formatDate(groupText);
  }

  QStringList groups;
  if(flags & BCAttribute::AllowMultiple) {
    groups = QStringList::split(";", groupText);
  } else {
    groups << groupText;
  }

  kdDebug() << "BCCollectionView::slotModifyItem() - " << unit_->attribute("title") << endl;
  QList<BCUnitItem> list = locateItem(unit_);
  QListIterator<BCUnitItem> it(list);
  for( ; it.current(); ++it) {
    ParentItem* par = static_cast<ParentItem*>(it.current()->parent());

    // easy case first, a parent with correct group name already exists
    if(groups.contains(par->text(0)) > 0) {
      // more efficient to check to see if update needed? probably not
      it.current()->setText(0, BCAttribute::formatTitle(unit_->attribute("title")));
      par->setOpen(true);
      // remove the string from the list we're checking
      groups.remove(par->text(0));
      // no longer have to worry about that item
      list.remove(it.current());
    } else {
      delete it.current();
      if(par->childCount() == 0) {
        m_groupDict->remove(QString::number(unit_->collection()->id()) + par->text(0));
        delete par;
      }
      // triggerUpdate();
    }
  } // end BCUnit iterator

  // if groups is empty, everything's done
  if(groups.isEmpty()) {
    return;
  }

  KIconLoader* loader = KGlobal::iconLoader();
  QPixmap icon;
  if(loader) {
    icon = loader->loadIcon(unit_->collection()->iconName(), KIcon::User);
  }

  // but if any groups are left, they either need to be created or a new item inserted
  ParentItem* root = locateItem(unit_->collection());
  QStringList::Iterator groupIt = groups.begin();
  for( ; groupIt != groups.end(); ++groupIt) {
    QString groupName = static_cast<QString>(*groupIt).simplifyWhiteSpace();
    // kdDebug() << groupName << endl;
    ParentItem* par = m_groupDict->find(QString::number(unit_->collection()->id()) + groupName);
    if(!par) {
      par = new ParentItem(root, groupName);
      par->setPixmap(0, m_groupClosed);
      m_groupDict->insert(QString::number(unit_->collection()->id()) + groupName, par);
    }
    BCUnitItem* item = new BCUnitItem(par, BCAttribute::formatTitle(unit_->attribute("title")), unit_);
    item->setPixmap(0, icon);
    par->setOpen(true);
  }
}

void BCCollectionView::slotRemoveItem(BCUnit* unit_) {
  if(!unit_) {
    return;
  }

//  kdDebug() << "BCCollectionView::slotRemoveItem() - unit: " << unit_->attribute("title") << endl;

  QList<BCUnitItem> list = locateItem(unit_);
  QListIterator<BCUnitItem> it(list);
  for( ; it.current(); ++it) {
    ParentItem* par = static_cast<ParentItem*>(it.current()->parent());
    delete it.current();
    // if the parent now has no children, delete it
    if(par->childCount() == 0) {
      m_groupDict->remove(QString::number(unit_->collection()->id()) + par->text(0));
      delete par;
    }
  }
}

void BCCollectionView::slotSelected(QListViewItem* item_) {
  // catch the case of a NULL pointer, i.e. not clicking on an item
  if(!item_) {
    emit signalClear();
    return;
  }

  // unitItems always have a depth of 2
  if(item_->depth() == 2) {
    BCUnitItem* item = static_cast<BCUnitItem*>(item_);
    if(item->unit()) {
      emit signalUnitSelected(item->unit());
    }
  } else {
    // TODO: does anything happen when a collection or group is selected?
    //emit signalCollectionSelected();
  }
}

void BCCollectionView::slotSetSelected(BCUnit* unit_) {
  // if unit_ is null pointer, set no selected
  if(!unit_) {
    setSelected(currentItem(), false);
    return;
  }

  QList<BCUnitItem> list = locateItem(unit_);
  // just select the first one
  // this ends up calling BCUnitEditWidget::slotSetContents() twice
  // since the selectedItem() signal gets sent by both this object and the other listview
  setSelected(list.getFirst(), true);
  ensureItemVisible(list.getFirst());
}

void BCCollectionView::slotToggleItem(QListViewItem* item_) {
  if(!item_) {
    return;
  }
  item_->setOpen(!item_->isOpen());
}

void BCCollectionView::slotExpandAll() {
  QListViewItem* item = selectedItem();
  if(!item) {
    return;
  }

  if(item->depth() == 0) { // collection item, close all top-level items
    item = firstChild();
  } else {
    item = item->parent()->firstChild();
  }

  // now open all siblings
  for( ; item; item = item->nextSibling()) {
    item->setOpen(true);
  }
}

void BCCollectionView::slotCollapseAll() {
  QListViewItem* item = selectedItem();
  if(!item) {
    return;
  }

  if(item->depth() == 0) { // collection item, close all top-level items
    item = firstChild();
  } else {
    item = item->parent()->firstChild();
  }

  // now close all siblings
  for( ; item; item = item->nextSibling()) {
    item->setOpen(false);
  }
}

void BCCollectionView::slotRMB(QListViewItem* item_, const QPoint& point_, int) {
  if(item_ && m_menu->count() > 0) {
    setSelected(item_, true);
    m_menu->popup(point_);
  }
}

void BCCollectionView::slotHandleRename() {
  QListViewItem* item = currentItem();
  if(item && item->depth() == 0) {
    QString newName;
    bool ok;
    newName = KLineEditDlg::getText(i18n("New Collection Name"), item->text(0), &ok, this);
    if(ok) {
      item->setText(0, newName);
      emit signalDoCollectionRename(static_cast<ParentItem*>(item)->id(), newName);
    }
  }
}

void BCCollectionView::slotCollapsed(QListViewItem* item_) {
  item_->setPixmap(0, m_groupClosed);
}

void BCCollectionView::slotExpanded(QListViewItem* item_) {
  item_->setPixmap(0, m_groupOpen);
}
