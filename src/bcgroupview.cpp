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
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bcgroupview.h"
#include "bcattribute.h"
#include "bcunitgroup.h"

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
  addColumn(QString::fromLatin1("Bookcase"));
  // hide the header since there's only one column
  header()->hide();
  setRootIsDecorated(true);
  setTreeStepSize(10);
  // turn off the alternate background color
  setAlternateBackground(QColor());
  setSelectionMode(QListView::Extended);

  QPixmap rename, fields, expand, collapse, remove;
  rename = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("editclear"), KIcon::Small);
  fields = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("edit"), KIcon::Small);
  expand = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("2downarrow"), KIcon::Small);
  collapse = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("2uparrow"), KIcon::Small);
  remove = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("remove"), KIcon::Small);

  m_collMenu.insertItem(rename, i18n("Rename Collection..."), this, SLOT(slotHandleRename()));
  m_collMenu.insertItem(fields, i18n("Edit Collection Fields..."), this, SLOT(slotHandleProperties()));

  m_groupMenu.insertItem(expand, i18n("Expand All Groups"), this, SLOT(slotExpandAll()));
  m_groupMenu.insertItem(collapse, i18n("Collapse All Groups"), this, SLOT(slotCollapseAll()));

  m_unitMenu.insertItem(remove, i18n("Delete Book"), this, SLOT(slotHandleDelete()));

  connect(this, SIGNAL(rightButtonPressed(QListViewItem*, const QPoint&, int)),
          SLOT(slotRMB(QListViewItem*, const QPoint&, int)));

//  connect(this, SIGNAL(clicked(QListViewItem*)), SLOT(slotSelected(QListViewItem*)));

  connect(this, SIGNAL(selectionChanged()),
          SLOT(slotSelectionChanged()));

  connect(this, SIGNAL(doubleClicked(QListViewItem*)),
          SLOT(slotToggleItem(QListViewItem*)));

  connect(this, SIGNAL(expanded(QListViewItem*)),
          SLOT(slotExpanded(QListViewItem*)));

  connect(this, SIGNAL(collapsed(QListViewItem*)),
          SLOT(slotCollapsed(QListViewItem*)));
          
  // TODO: maybe allow this to be customized
  m_collOpenPixmap = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("folder_open"),
                                                      KIcon::Small);
  m_collClosedPixmap = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("folder"),
                                                        KIcon::Small);
  m_groupOpenPixmap = m_collOpenPixmap;
  m_groupClosedPixmap = m_collClosedPixmap;

}

ParentItem* BCGroupView::insertItem(ParentItem* collItem_, const BCUnitGroup* group_) {
  QString text = group_->groupName();
  
  ParentItem* par = new ParentItem(collItem_, text);
  par->setPixmap(0, m_groupClosedPixmap);
  par->setCount(group_->count());
  
  QString key = groupKey(collItem_, group_);
  m_groupDict.insert(key, par);
  
  return par;
}

ParentItem* BCGroupView::locateItem(ParentItem* collItem_, const BCUnitGroup* group_) {
  QString key = groupKey(collItem_, group_);
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
//    kdDebug() << "BCGroupView::locateItem() - adding new collection" << endl;
    root = new ParentItem(this, coll_->title(), coll_->id());
  }
  return root;
}

const QString& BCGroupView::collGroupBy(const QString& type_) const {
  return m_collGroupBy[type_];
}

void BCGroupView::slotReset() {
  // don't really need to clear the collGroupBy map
  m_groupDict.clear();
  m_selectedUnits.clear();
  clear();
}

void BCGroupView::slotRemoveItem(BCCollection* coll_) {
  if(!coll_) {
    kdWarning() << "BCGroupView::slotRemoveItem() - null coll pointer!" << endl;
    return;
  }
//  kdDebug() << "BCGroupView::slotRemoveItem(BCCollection) - " << coll_->title() << endl;

  ParentItem* collItem = locateItem(coll_);
  // first remove all groups in collection from dict
  QListViewItem* groupItem = collItem->firstChild();
  for( ; groupItem; groupItem = groupItem->nextSibling()) {
    QString key = groupKey(coll_, groupItem);
    m_groupDict.remove(key);
  }
  // automatically deletes all children
  delete collItem;
}

void BCGroupView::slotModifyGroup(BCCollection* coll_, const BCUnitGroup* group_) {
  if(!coll_ || !group_) {
    kdWarning() << "BCGroupView::slotModifyGroup() - null coll or group pointer!" << endl;
    return;
  }

  // if the units aren't grouped by attribute of the modified group,
  // we don't care, so return
  QString unitName = coll_->unitName();
  if(m_collGroupBy.contains(unitName) && m_collGroupBy[unitName] != group_->attributeName()) {
    return;
  }
  
//  kdDebug() << "BCGroupView::slotModifyGroup() - " << group_->attributeName() << endl;

  ParentItem* root = locateItem(coll_);
  ParentItem* par = locateItem(root, group_);
  //make a copy of the group
  BCUnitGroup leftover(*group_);

  // first delete all items in this view but no longer in the group
  QListViewItem* item = par->firstChild();
  QListViewItem* next;
  while(item) {
    BCUnit* unit = static_cast<BCUnitItem*>(item)->unit();
    if(group_->containsRef(unit)) {
      leftover.removeRef(unit);
      item = item->nextSibling();
    } else {
      // if it's not in the group, delete it
//      kdDebug() << "\tdeleting unit - " << unit->title() << endl;
      m_selectedUnits.removeRef(unit);
      next = item->nextSibling();
      delete item;
      item = next;
    }
  }
  
  // if there are no more child items in the group, no units in the leftover group
  // then delete the parent item
  if(par->childCount() == 0 && leftover.isEmpty()) {
    QString key = groupKey(coll_, par);
    m_groupDict.remove(key);
    delete par;
    return;
  }

  QPixmap icon;
  icon = KGlobal::iconLoader()->loadIcon(coll_->iconName(), KIcon::User);

  // in case the number of units in the group changed
//  par->repaint();
  par->setCount(group_->count());
    
  // next add new listViewItems for items in the group, but not currently in the view
  QPtrListIterator<BCUnit> it(leftover);
  for( ; it.current(); ++it) {
    QString title = it.current()->title();

    BCUnitItem* item = new BCUnitItem(par, title, it.current());
    item->setPixmap(0, icon);
    if(isUpdatesEnabled()) {
      ensureItemVisible(item);
//      blockSignals(true);
//      setSelected(item, true);
//      blockSignals(false);
      par->setOpen(true);
    }
  }

  // don't want any selected
  slotClearSelection();
  // need to refresh
  triggerUpdate();
  root->setOpen(true);
}

void BCGroupView::slotSelectionChanged() {
  // since the group view might have multiple unit items
  // that point to the same unit, need to keep track separately
  BCUnitList newSelected;

  // all items with depth of 2 in the listview are unitItems
  BCUnitItem* item;
  QListViewItemIterator it(this);
  for( ; it.current(); ++it) {
    if(!it.current()->isSelected()) {
      continue;
    }
    // it'd be nice if I could figure out how to enforce
    // only one collection or group being selected, while allowing multiple
    // unit items to be selected
    if(it.current()->depth() == 0) {
      ParentItem* pItem = static_cast<ParentItem*>(it.current());
      emit signalCollectionSelected(pItem->id());
    } else if(it.current()->depth() == 2) {
      item = static_cast<BCUnitItem*>(it.current());
      newSelected.append(item->unit());

      if(item->unit() && !m_selectedUnits.containsRef(item->unit())) {
        // the reason I do it this way is because I want the first item in the list
        // to be the first item that was selected, so the edit widget can fill its
        // contents with the first unit
        m_selectedUnits.append(item->unit());
      }
    }
  }

  // now if any units in m_selectedUnits are not in newSelected, remove them
  BCUnitListIterator uIt(m_selectedUnits);
  for( ; uIt.current(); ++uIt) {
    if(!newSelected.containsRef(uIt.current())) {
      m_selectedUnits.removeRef(uIt.current());
      --uIt; // when a unit is removed, the iterator goes to next
    }
  }

  emit signalUnitSelected(m_selectedUnits);
}

void BCGroupView::slotSetSelected(BCUnit* unit_) {
//  kdDebug() << "BCGroupView::slotSetSelected()" << endl;
  // if unit_ is null pointer, set no selected
  if(!unit_) {
    setSelected(currentItem(), false);
    return;
  }

  // if the selected unit is the same as the current one, just return
  if(currentItem() && unit_ == static_cast<BCUnitItem*>(currentItem())->unit()) {
    return;
  }

  // have to find a group whose attribute is the same as currently shown
  QString unitName = unit_->collection()->unitName();
  if(!m_collGroupBy.contains(unitName)) {
    kdDebug() << "BCGroupView::slotSetSelected() - no group attribute for " << unitName << endl;
    return;
  }

  BCUnitGroup* group = 0;
  QString currentGroup = m_collGroupBy[unitName];
  QPtrListIterator<BCUnitGroup> groupIt(unit_->groups());
  for( ; groupIt.current(); ++groupIt) {
    if(groupIt.current()->attributeName() == currentGroup) {
      group = groupIt.current();
      break;
    }
  }
  if(!group) {
    kdDebug() << "BCGroupView::slotSetSelected() - unit is not in any current groups!" << endl;
    return;
  }
  
  QString key = groupKey(unit_->collection(), group);
  ParentItem* par = m_groupDict.find(key);
  if(!par) {
    return;
  }

  BCUnitItem* unitItem = 0;
  QListViewItem* item = par->firstChild();
  for( ; item; item = item->nextSibling()) {
    unitItem = static_cast<BCUnitItem*>(item);
    if(unitItem->unit() == unit_) {
      break;
    }
  }

  slotClearSelection();
  blockSignals(true);
  setSelected(unitItem, true);
  setCurrentItem(item);
  blockSignals(false);
  ensureItemVisible(unitItem);
}

void BCGroupView::slotToggleItem(QListViewItem* item_) {
  if(!item_) {
    return;
  }
  item_->setOpen(!item_->isOpen());
}

void BCGroupView::slotExpandAll(int depth_/*=-1*/) {
  if(childCount() == 0) {
    return;
  }

  QListViewItem* item = 0;

  if(depth_ == -1) {
    item = currentItem();
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
  if(childCount() == 0) {
    return;
  }

  QListViewItem* item = 0;

  if(depth_ == -1) {
    item = currentItem();
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
  // items at depth==0 are for collections
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

void BCGroupView::slotHandleProperties() {
  QListViewItem* item = currentItem();
  // items at depth==0 are for collections
  if(item && item->depth() == 0) {
    emit signalModifyCollection(static_cast<ParentItem*>(item)->id());
  }
}

void BCGroupView::slotCollapsed(QListViewItem* item_) {
  // only change icon for collection and group items
  if(item_->depth() == 0) {
    item_->setPixmap(0, m_collClosedPixmap);
  } else if(item_->depth() == 1) {
    item_->setPixmap(0, m_groupClosedPixmap);
  }
}

void BCGroupView::slotExpanded(QListViewItem* item_) {
  // only change icon for collection and group items
  if(item_->depth() == 0) {
    item_->setPixmap(0, m_collOpenPixmap);
  } else if(item_->depth() == 1) {
    item_->setPixmap(0, m_groupOpenPixmap);
  }
}

void BCGroupView::slotClearSelection() {
//  kdDebug() << "BCGroupView::slotClearSelection()" << endl;
  blockSignals(true);
  selectAll(false);
  setCurrentItem(0);
  blockSignals(false);
  m_selectedUnits.clear();
}

void BCGroupView::slotAddCollection(BCCollection* coll_) {
  if(!coll_) {
    kdWarning() << "BCGroupView::slotAddCollection() - null coll pointer!" << endl;
    return;
  }
  
//  kdDebug() << "BCGroupView::slotAddCollection" << endl;

  QString groupBy;
  QString unitName = coll_->unitName();
  if(m_collGroupBy.contains(unitName)) {
    groupBy = m_collGroupBy[unitName];
  } else {
    groupBy = coll_->defaultGroupAttribute();
    m_collGroupBy.insert(unitName, groupBy);
//    kdDebug() << "\tm_groupAttribute was empty, now is " << groupBy << endl;
  }

  ParentItem* collItem = populateCollection(coll_, groupBy);
  if(!collItem) {
    return;
  }
  
  setSelected(collItem, true);
  slotCollapseAll();
  ensureItemVisible(collItem);
  collItem->setOpen(true);
//  kdDebug() << "BCGroupView::slotAddCollection - done" << endl;
}

void BCGroupView::setGroupAttribute(BCCollection* coll_, const QString& groupAtt_) {
//  kdDebug() << "BCGroupView::setGroupAttribute - " << groupAtt_ << endl;

  BCAttribute* att = coll_->attributeByName(groupAtt_);
  if(!att) {
    return;
  }
  
  // as a hack, when a new collection is added,  this gets called
  // if the collection item is empty, go ahead and populate it
  // even if the group attribute has not changed
  ParentItem* collItem = locateItem(coll_);
  QString unitName = coll_->unitName();
  if(!m_collGroupBy.contains(unitName)
     || m_collGroupBy[unitName] != groupAtt_
     || collItem->childCount() == 0) {
    m_collGroupBy.insert(unitName, groupAtt_);
    if(att->formatFlag() == BCAttribute::FormatName) {
      m_groupOpenPixmap = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("person-open"),
                                                          KIcon::Small);
      m_groupClosedPixmap = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("person"),
                                                            KIcon::Small);
    } else {
      m_groupOpenPixmap = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("folder_open"),
                                                          KIcon::Small);
      m_groupClosedPixmap = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("folder"),
                                                            KIcon::Small);
    }
    populateCollection(coll_, groupAtt_);
  }
}

bool BCGroupView::showCount() const {
  return m_showCount;
}

void BCGroupView::showCount(bool showCount_) {
//  kdDebug() << "BCGroupView::showCount()" << endl;
  if(m_showCount != showCount_) {
    m_showCount = showCount_;
    triggerUpdate();
  }
}

ParentItem* BCGroupView::populateCollection(BCCollection* coll_,
  const QString& groupBy_/*=QString::null*/) {

//  kdDebug() << "BCGroupView::populateCollection()" << endl;
  BCUnitGroupDict* dict;
  if(groupBy_.isEmpty()) {
    QString groupBy;
    QString unitName = coll_->unitName();
    if(m_collGroupBy.contains(unitName)) {
      groupBy = m_collGroupBy[unitName];
    } else {
      groupBy = coll_->defaultGroupAttribute();
      m_collGroupBy.insert(unitName, groupBy);
//      kdDebug() << "\tm_groupAttribute was empty, now is " << groupBy << endl;
    }
    dict = coll_->unitGroupDictByName(groupBy);
  } else {
    dict = coll_->unitGroupDictByName(groupBy_);
    if(!dict) {
      kdDebug() << "BCGroupView::populateCollection() - no dict found for " << groupBy_ << endl;
      return 0;
    }
  }

  //if there is not a root item for the collection, it is created
  ParentItem* collItem = locateItem(coll_);
  // delete all the children;
  if(collItem->childCount() > 0) {
//    kdDebug() << "\tdeleting all the children" << endl;
    QListViewItem* child = collItem->firstChild();
    QListViewItem* next;
    while(child) {
      next = child->nextSibling();
      QString key = groupKey(collItem, child);
      m_groupDict.remove(key);
      delete child;
      child = next;
    }
  }

  QPixmap icon = KGlobal::iconLoader()->loadIcon(coll_->iconName(), KIcon::User);

  // iterate over all the groups in the dict
  // e.g. if the dict is "author", loop over all the author groups
  QDictIterator<BCUnitGroup> it(*dict);
  for( ; it.current(); ++it) {
    ParentItem* par = insertItem(collItem, it.current());

    QPtrListIterator<BCUnit> unitIt(*it.current());
    for( ; unitIt.current(); ++unitIt) {
      QString title = unitIt.current()->title();
      BCUnitItem* item = new BCUnitItem(par, title, unitIt.current());
      item->setPixmap(0, icon);
    }
  }
  return collItem;
}

void BCGroupView::slotHandleDelete() {
  BCUnitItem* item = static_cast<BCUnitItem*>(currentItem());
  if(!item || !item->unit()) {
    return;
  }

  emit signalDeleteUnit(item->unit());
}
