/***************************************************************************
                               bcgroupview.cpp
                             -------------------
    begin                : Sat Oct 13 2001
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

#include "bcgroupview.h"
#include "bccollection.h"
#include "bcattribute.h"
#include "bcunititem.h"
#include "bcunitgroup.h"
#include "bookcasedoc.h"
#include "bookcase.h"

#include <klineeditdlg.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kglobal.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qcolor.h>
#include <qregexp.h>
#include <qheader.h>

// by default don't show the number of items
BCGroupView::BCGroupView(QWidget* parent_, const char* name_/*=0*/)
    : KListView(parent_, name_), m_showCount(false) {
  // the app name isn't translated
  addColumn("Bookcase");
  // hide the header since there's only one column
  header()->hide();
  setRootIsDecorated(true);
  setTreeStepSize(10);
  // turn off the alternate background color
  setAlternateBackground(QColor());

  QPixmap rename, expand, collapse;
  rename = KGlobal::iconLoader()->loadIcon("editclear", KIcon::Small);
  expand = KGlobal::iconLoader()->loadIcon("2downarrow", KIcon::Small);
  collapse = KGlobal::iconLoader()->loadIcon("2uparrow", KIcon::Small);

  m_groupOpenPixmap = KGlobal::iconLoader()->loadIcon("folder_red_open", KIcon::Small);
  m_groupClosedPixmap = KGlobal::iconLoader()->loadIcon("folder_red", KIcon::Small);

  m_collMenu.insertItem(rename, i18n("Rename Collection"), this, SLOT(slotHandleRename()));

  m_groupMenu.insertItem(expand, i18n("Expand All Groups"), this, SLOT(slotExpandAll()));
  m_groupMenu.insertItem(collapse, i18n("Collapse All Groups"), this, SLOT(slotCollapseAll()));

  connect(this, SIGNAL(rightButtonPressed(QListViewItem*, const QPoint&, int)),
          SLOT(slotRMB(QListViewItem*, const QPoint&, int)));

//  connect(this, SIGNAL(clicked(QListViewItem*)), SLOT(slotSelected(QListViewItem*)));

  connect(this, SIGNAL(selectionChanged(QListViewItem*)), SLOT(slotSelected(QListViewItem*)));

  connect(this, SIGNAL(doubleClicked(QListViewItem*)), SLOT(slotToggleItem(QListViewItem*)));

  connect(this, SIGNAL(expanded(QListViewItem*)), SLOT(slotExpanded(QListViewItem*)));

  connect(this, SIGNAL(collapsed(QListViewItem*)), SLOT(slotCollapsed(QListViewItem*)));
}

ParentItem* BCGroupView::insertItem(ParentItem* collItem_, BCUnitGroup* group_) {
  QString text = group_->groupName();
  if(m_showCount) {
   text += " (" + QString::number(group_->count()) + ")";
  }
  ParentItem* par = new ParentItem(collItem_, text);
  par->setPixmap(0, m_groupClosedPixmap);
  m_groupDict.insert(QString::number(collItem_->id()) + group_->groupName(), par);
  return par;
}

ParentItem* BCGroupView::locateItem(ParentItem* collItem_, BCUnitGroup* group_) {
  QString key = QString::number(collItem_->id()) + group_->groupName();
  ParentItem* par = m_groupDict.find(key);
  if(par) {
    return par;
  }

  return insertItem(collItem_, group_);
}


ParentItem* BCGroupView::locateItem(BCCollection* coll_) {
  ParentItem* root = 0;
  ParentItem* par;
  // iterate over the collections, which are the top-level children
  QListViewItem* collItem = firstChild();
  for( ; collItem; collItem = collItem->nextSibling()) {
    // find the collItem matching the unit's collection and insert item inside
    par = static_cast<ParentItem*>(collItem);
    if(par->id() == coll_->id()) {
      root = par;
      break;
    }
  }
  if(!root) {
    kdDebug() << "BCGroupView::locateItem() - adding new collection" << endl;
    root = new ParentItem(this, coll_->title(), coll_->id());
  }
  return root;
}

const QString& BCGroupView::groupAttribute() const {
  return m_groupAttribute;
}

void BCGroupView::slotReset() {
  m_groupDict.clear();
  clear();
}

void BCGroupView::slotRemoveItem(BCCollection* coll_) {
  if(!coll_) {
    return;
  }

//  kdDebug() << "BCGroupView::slotRemoveItem() - " << unit_->attribute("title") << endl;
  ParentItem* collItem = locateItem(coll_);
  // first remove all groups in collection from dict
  QListViewItem* groupItem = collItem->firstChild();
  for( ; groupItem; groupItem = groupItem->nextSibling()) {
    m_groupDict.remove(QString::number(coll_->id()) + groupItem->text(0));
  }
  // automatically deletes all children
  delete collItem;
}

void BCGroupView::slotModifyGroup(BCCollection* coll_, BCUnitGroup* group_) {
  if(!coll_ || !group_ || m_groupAttribute != group_->attributeName()) {
    return;
  }
//  kdDebug() << "BCGroupView::slotModifyGroup() - " << group_->attributeName() << endl;

  ParentItem* root = locateItem(coll_);
  ParentItem* par = locateItem(root, group_);
  BCUnitGroup leftover(*group_);

  // first delete all items not in the group
  QListViewItem* item = par->firstChild();
  while(item) {
    BCUnit* unit = static_cast<BCUnitItem*>(item)->unit();
    if(group_->containsRef(unit)) {
      leftover.removeRef(unit);
      item = item->nextSibling();
    } else {
//      kdDebug() << "\tdeleting unit - " << unit->attribute("title") << endl;
      QListViewItem* next = item->nextSibling();
      delete item;
      item = next;
    }
  }
  if(par->childCount() == 0 && leftover.isEmpty()) {
    m_groupDict.remove(QString::number(coll_->id()) + par->text(0));
    delete par;
    return;
  }

  QPixmap icon;
  icon = KGlobal::iconLoader()->loadIcon(coll_->iconName(), KIcon::User);

  // in case the number of units in the group changed
  if(m_showCount) {
    par->setText(0, group_->groupName() + " (" + QString::number(group_->count()) + ")");
  }

  // next add new listViewItems for items in the group, but not in the view
  QPtrListIterator<BCUnit> it(leftover);
  for( ; it.current(); ++it) {
    // guaranteed to have a "title" attribute
    QString title = it.current()->attribute("title");
    BCAttribute::formatTitle(title);

    BCUnitItem* item = new BCUnitItem(par, title, it.current());
    item->setPixmap(0, icon);
    if(isUpdatesEnabled()) {
      ensureItemVisible(item);
      setSelected(item, true);
      par->setOpen(true);
    }
  }
  root->setOpen(true);
}

void BCGroupView::slotSelected(QListViewItem* item_) {
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
  } else if(item_->depth() == 0) {   // collections are at the root
    ParentItem* item = static_cast<ParentItem*>(item_);
    emit signalCollectionSelected(item->id());
  }
}

void BCGroupView::slotSetSelected(BCUnit* unit_) {
  // if unit_ is null pointer, set no selected
  if(!unit_) {
    setSelected(currentItem(), false);
    return;
  }

  // just grab the listitem for the first group
  BCUnitGroup* group = unit_->groups().getFirst();
  QString dictKey = QString::number(unit_->collection()->id()) + group->groupName();
  ParentItem* par = m_groupDict.find(dictKey);
  if(!par) {
    return;
  }

  BCUnitItem* unitItem = 0;
  QListViewItem* item = par->firstChild();
  for( ; item; item->nextSibling()) {
    unitItem = static_cast<BCUnitItem*>(item);
    if(unitItem->unit() == unit_) {
      break;
    }
  }

  // this ends up calling BCUnitEditWidget::slotSetContents() twice
  // since the selectedItem() signal gets sent by both this object and the
  // detailed listview
  setSelected(unitItem, true);
  ensureItemVisible(unitItem);
}

void BCGroupView::slotToggleItem(QListViewItem* item_) {
  if(!item_) {
    return;
  }
  item_->setOpen(!item_->isOpen());
}

void BCGroupView::slotExpandAll(int depth_/*=-1*/) {
  QListViewItem* item;

  if(depth_ == -1) {
    item = selectedItem();
    if(!item) {
      return;
    }
    depth_ = item->depth();
  }

  switch(depth_) {
    //TODO: should be iterating over all collections
    case 0:
      item = firstChild();
      break;

    case 1:
      item = firstChild()->firstChild();
      break;

    default:
      return;
  }

  // now open all siblings
  for( ; item; item = item->nextSibling()) {
    item->setOpen(true);
  }
}

void BCGroupView::slotCollapseAll(int depth_/*=-1*/) {
  QListViewItem* item;

  if(depth_ == -1) {
    item = selectedItem();
    if(!item) {
      return;
    }
    depth_ = item->depth();
  }

  switch(depth_) {
    //TODO: should be iterating over all collections
    case 0:
      item = firstChild();
      break;

    case 1:
      item = firstChild()->firstChild();
      break;

    default:
      return;
  }

  // now close all siblings
  for( ; item; item = item->nextSibling()) {
    item->setOpen(false);
  }
}

void BCGroupView::slotRMB(QListViewItem* item_, const QPoint& point_, int) {
  if(!item_) {
    return;
  }

  setSelected(item_, true);
  if(item_->depth() == 0 && m_collMenu.count() > 0) {
    m_collMenu.popup(point_);
  } else if(item_->depth() == 1 && m_groupMenu.count() > 0) {
    m_groupMenu.popup(point_);
  } else if(item_->depth() == 2 && m_unitMenu.count() > 0) {
    m_unitMenu.popup(point_);
  }
}

void BCGroupView::slotHandleRename() {
  QListViewItem* item = currentItem();
  if(item && item->depth() == 0) {
    QString newName;
    bool ok;
    newName = KLineEditDlg::getText(i18n("New Collection Name"), item->text(0), &ok, this);
    if(ok) {
      item->setText(0, newName);
      emit signalRenameCollection(static_cast<ParentItem*>(item)->id(), newName);
    }
  }
}

void BCGroupView::slotCollapsed(QListViewItem* item_) {
  // only change icon for collection and group items
  if(item_->depth() < 2) {
    item_->setPixmap(0, m_groupClosedPixmap);
  }
}

void BCGroupView::slotExpanded(QListViewItem* item_) {
  // only change icon for collection and group items
  if(item_->depth() < 2) {
    item_->setPixmap(0, m_groupOpenPixmap);
  }
}

void BCGroupView::slotClearSelection() {
  selectAll(false);
}

void BCGroupView::slotAddCollection(BCCollection* coll_) {
  if(!coll_) {
    return;
  }
  kdDebug() << "BCGroupView::slotAddCollection" << endl;

  if(m_groupAttribute.isEmpty()) {
    m_groupAttribute = coll_->defaultGroupAttribute();
    kdDebug() << "\tm_groupAttribute was empty, now is " << m_groupAttribute << endl;
  }

  BCUnitGroupDict* dict = coll_->unitGroupDictByName(m_groupAttribute);
  if(!dict) {
//    kdDebug() << "\tno dict found" << endl;
    return;
  }

  ParentItem* collItem = locateItem(coll_);
  // delete all the children;
  if(collItem->childCount() > 0) {
    QListViewItem* child = collItem->firstChild();
    QListViewItem* next;
    while(child) {
      next = child->nextSibling();
      m_groupDict.remove(QString::number(collItem->id()) + child->text(0));
      delete child;
      child = next;
    }
  }

  QPixmap icon = KGlobal::iconLoader()->loadIcon(coll_->iconName(), KIcon::User);

  QDictIterator<BCUnitGroup> it(*dict);
  for( ; it.current(); ++it) {
    ParentItem* par = insertItem(collItem, it.current());

    QPtrListIterator<BCUnit> unitIt(*it.current());
    for( ; unitIt.current(); ++unitIt) {
      QString title = unitIt.current()->attribute("title");
      BCAttribute::formatTitle(title);
      BCUnitItem* item = new BCUnitItem(par, title, unitIt.current());
      item->setPixmap(0, icon);
    }
  }

  setSelected(collItem, true);
  slotCollapseAll();
  ensureItemVisible(collItem);
  collItem->setOpen(true);
//  kdDebug() << "BCGroupView::slotAddCollection - done" << endl;
}

void BCGroupView::setGroupAttribute(BCCollection* coll_, const QString& groupAtt_) {
  kdDebug() << "BCGroupView::setGroupAttribute - " << groupAtt_ << endl;
  // this gets called when a file is read and then, the groupAttributes match
  // but there's no children yet, so check for that, too
  if(!coll_) {
    m_groupAttribute = groupAtt_;
//    kdDebug() << "\tempty collection - done" << endl;
    return;
  }

  ParentItem* collItem = locateItem(coll_);
  if(m_groupAttribute != groupAtt_ || collItem->childCount() == 0) {
    m_groupAttribute = groupAtt_;
    slotAddCollection(coll_);
  }
//  kdDebug() << "BCGroupView::setGroupAttribute - done" << endl;
}

void BCGroupView::slotShowCount(bool showCount_) {
  kdDebug() << "BCGroupView::slotShowCount" << endl;
  if(m_showCount != showCount_) {
    m_showCount = showCount_;
    // TODO: fix this for multiple collections
    // parent is actually a qsplit. take parent of parent
    Bookcase* app = static_cast<Bookcase*>(parent()->parent());
    BCCollection* coll = app->doc()->collectionById(0);
    slotAddCollection(coll);
  }
}
