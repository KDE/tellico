/***************************************************************************
                               bcgroupview.cpp
                             -------------------
    begin                : Sat Oct 13 2001
    copyright            : (C) 2001, 2002, 2003 by Robby Stephenson
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
#include "bookcase.h"
#include "bccollection.h"
#include "bcattribute.h"
#include "bcutils.h"

#include <kpopupmenu.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <kaction.h>

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

  QPixmap expand, collapse;
  expand = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("2downarrow"), KIcon::Small);
  collapse = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("2uparrow"), KIcon::Small);

  Bookcase* bookcase = static_cast<Bookcase*>(QObjectAncestor(parent_, "Bookcase"));
  m_collMenu = new KPopupMenu(this);
  bookcase->action("edit_rename_collection")->plug(m_collMenu);
  bookcase->action("edit_fields")->plug(m_collMenu);
  bookcase->action("edit_convert_bibtex")->plug(m_collMenu);
  bookcase->action("edit_string_macros")->plug(m_collMenu);

  m_groupMenu = new KPopupMenu(this);
  m_groupMenu->insertItem(expand, i18n("Expand All Groups"), this, SLOT(slotExpandAll()));
  m_groupMenu->insertItem(collapse, i18n("Collapse All Groups"), this, SLOT(slotCollapseAll()));

  m_unitMenu = new KPopupMenu(this);
  bookcase->action("edit_delete")->plug(m_unitMenu);

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

inline
QString BCGroupView::groupKey(const ParentItem* par_, QListViewItem* item_) const {
  return QString::number(par_->id()) + item_->text(0);
}

inline
QString BCGroupView::groupKey(const ParentItem* par_, const BCUnitGroup* group_) const {
  return QString::number(par_->id()) + group_->groupName();
}

inline
QString BCGroupView::groupKey(const BCCollection* coll_, QListViewItem* item_) const {
  return QString::number(coll_->id()) + item_->text(0);
}

inline
QString BCGroupView::groupKey(const BCCollection* coll_, const BCUnitGroup* group_) const {
  return QString::number(coll_->id()) + group_->groupName();
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
  // iterate over the collections, which are the top-level children
  QListViewItem* collItem = firstChild();
  for( ; collItem; collItem = collItem->nextSibling()) {
    // find the collItem matching the unit's collection and insert item inside
    ParentItem* par = static_cast<ParentItem*>(collItem);
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

const QString& BCGroupView::groupBy() const {
  return m_groupBy;
}

void BCGroupView::slotReset() {
  // don't really need to clear the collGroupBy map
  m_groupDict.clear();
  m_selectedUnits.clear();
  clear();
}

void BCGroupView::removeCollection(BCCollection* coll_) {
  if(!coll_) {
    kdWarning() << "BCGroupView::removeCollection() - null coll pointer!" << endl;
    return;
  }
//  kdDebug() << "BCGroupView::removeCollection() - " << coll_->title() << endl;

  ParentItem* collItem = locateItem(coll_);
  // first remove all groups in collection from dict
  QListViewItem* groupItem = collItem->firstChild();
  for( ; groupItem; groupItem = groupItem->nextSibling()) {
    QString key = groupKey(coll_, groupItem);
    m_groupDict.remove(key);
  }
  // automatically deletes all children
  delete collItem;

  m_selectedUnits.clear();
}

void BCGroupView::slotModifyGroup(BCCollection* coll_, const BCUnitGroup* group_) {
  if(!coll_ || !group_) {
    kdWarning() << "BCGroupView::slotModifyGroup() - null coll or group pointer!" << endl;
    return;
  }

  // if the units aren't grouped by attribute of the modified group,
  // we don't care, so return
  if(m_groupBy != group_->attributeName()) {
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

  QPixmap icon = KGlobal::iconLoader()->loadIcon(coll_->unitName(), KIcon::User);

  // in case the number of units in the group changed
  par->setCount(group_->count());

  // next add new listViewItems for items in the group, but not currently in the view
  QPtrListIterator<BCUnit> it(leftover);
  for( ; it.current(); ++it) {
    QString title = it.current()->title();

    BCUnitItem* item = new BCUnitItem(par, title, it.current());
    item->setPixmap(0, icon);
    if(isUpdatesEnabled()) {
      ensureItemVisible(item);
      par->setOpen(true);
    }
  }

  // don't want any selected
  clearSelection();

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
#if QT_VERSION >= 0x030200
  QListViewItemIterator it(this, QListViewItemIterator::Selected);
#else
  QListViewItemIterator it(this);
#endif
  for( ; it.current(); ++it) {
#if QT_VERSION < 0x030200
    if(!it.current()->isSelected()) {
      continue;
    }
#endif
    // it'd be nice if I could figure out how to enforce
    // only one collection or group being selected, while allowing multiple
    // unit items to be selected
    if(it.current()->depth() == 0) {
      ParentItem* pItem = static_cast<ParentItem*>(it.current());
      emit signalCollectionSelected(pItem->id());
      // only add selected if parent is open
    } else if(it.current()->depth() == 2) {
      if(!it.current()->parent()->isOpen()) {
        blockSignals(true);
        setSelected(it.current(), false);
        blockSignals(false);
        continue;
      }
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
  while(uIt.current()) {
    if(!newSelected.containsRef(uIt.current())) {
      m_selectedUnits.removeRef(uIt.current()); // when a unit is removed, the iterator goes to next
    } else {
      ++uIt;
    }
  }

  emit signalUnitSelected(this, m_selectedUnits);
}

// don't 'shadow' QListView::setSelected
void BCGroupView::setUnitSelected(BCUnit* unit_) {
//  kdDebug() << "BCGroupView::slotSetSelected()" << endl;
  // if unit_ is null pointer, set no selected
  if(!unit_) {
    // don't move this one outside the block since it calls setCurrentItem(0)
    clearSelection();
    return;
  }

  // if the selected unit is the same as the current one, just return
  if(currentItem() && unit_ == static_cast<BCUnitItem*>(currentItem())->unit()) {
    return;
  }

  // have to find a group whose attribute is the same as currently shown
  if(m_groupBy.isEmpty()) {
    kdDebug() << "BCGroupView::slotSetSelected() - no group attribute" << endl;
    return;
  }

  BCUnitGroup* group = 0;
  QPtrListIterator<BCUnitGroup> groupIt(unit_->groups());
  for( ; groupIt.current(); ++groupIt) {
    if(groupIt.current()->attributeName() == m_groupBy) {
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

  QListViewItem* item = par->firstChild();
  for( ; item; item = item->nextSibling()) {
    BCUnitItem* unitItem = static_cast<BCUnitItem*>(item);
    if(unitItem->unit() == unit_) {
      break;
    }
  }

  clearSelection();
  blockSignals(true);
  setSelected(item, true);
  setCurrentItem(item);
  blockSignals(false);
  ensureItemVisible(item);
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
  setSiblingsOpen(depth_, true);
}

void BCGroupView::slotCollapseAll(int depth_/*=-1*/) {
  if(childCount() == 0) {
    return;
  }
  setSiblingsOpen(depth_, false);
}

void BCGroupView::setSiblingsOpen(int depth_, bool open_) {
  QListViewItem* item = 0;

  if(depth_ == -1) {
    item = currentItem();
    if(!item) {
      return;
    }
    depth_ = item->depth();
  }

  switch(depth_) {
    case 0:
      item = firstChild();
      break;

    case 1:
      item = firstChild();
      while(item && item->childCount() == 0) {
        item = item->nextSibling();
      }
      if(!item) {
        return;
      }
      item = item->firstChild();
      break;

    default:
      return;
  }

  for( ; item; item = item->nextSibling()) {
    item->setOpen(open_);
  }
}

void BCGroupView::slotRMB(QListViewItem* item_, const QPoint& point_, int) {
  if(!item_) {
    return;
  }

  setSelected(item_, true);

  if(item_->depth() == 0 && m_collMenu->count() > 0) {
    m_collMenu->popup(point_);
  } else if(item_->depth() == 1 && m_groupMenu->count() > 0) {
    m_groupMenu->popup(point_);
  } else if(item_->depth() == 2 && m_unitMenu->count() > 0) {
    m_unitMenu->popup(point_);
  }
}

void BCGroupView::renameCollection(const QString& name_) {
  // items at depth==0 are for collections
  QListViewItem* item = firstChild();
  if(item) {
    item->setText(0, name_);
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

void BCGroupView::clearSelection() {
//  kdDebug() << "BCGroupView::clearSelection()" << endl;
  blockSignals(true);
  if(isUpdatesEnabled()) {
    selectAll(false);
  }
  setCurrentItem(0);
  blockSignals(false);
  m_selectedUnits.clear();
}

void BCGroupView::addCollection(BCCollection* coll_) {
  if(!coll_) {
    kdWarning() << "BCGroupView::addCollection() - null coll pointer!" << endl;
    return;
  }
  
//  kdDebug() << "BCGroupView::addCollection" << endl;

  // if the collection doesn't have the grouped attribute, and it's not the pseudo-group,
  // change it to default
  if(coll_->attributeByName(m_groupBy) == 0 && m_groupBy != QString::fromLatin1("_people")) {
    m_groupBy = coll_->defaultGroupAttribute();
  }

  ParentItem* collItem = populateCollection(coll_);
  if(!collItem) {
    return;
  }
  
//  slotClearSelection();
//  setSelected(collItem, true);
  slotCollapseAll();
  ensureItemVisible(collItem);
  collItem->setOpen(true);
//  kdDebug() << "BCGroupView::addCollection - done" << endl;
}

void BCGroupView::setGroupAttribute(BCCollection* coll_, const QString& groupAtt_) {
//  kdDebug() << "BCGroupView::setGroupAttribute - " << groupAtt_ << endl;

  // groupAtt_ could be"_people" for the pseudo-group;
  BCAttribute* att = coll_->attributeByName(groupAtt_);
  if(!att && groupAtt_ != QString::fromLatin1("_people")) {
    return;
  }
  
  // as a hack, when a new collection is added, this gets called
  // if the collection item is empty, go ahead and populate it
  // even if the group attribute has not changed
//  ParentItem* collItem = locateItem(coll_);
//  if(m_groupBy != groupAtt_ || collItem->childCount() == 0) {
  if(m_groupBy != groupAtt_) {
    m_groupBy = groupAtt_;
    if((att && att->formatFlag() == BCAttribute::FormatName)
       || groupAtt_ == QString::fromLatin1("_people")) {
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
    populateCollection(coll_);
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

ParentItem* BCGroupView::populateCollection(BCCollection* coll_) {
//  kdDebug() << "BCGroupView::populateCollection()" << endl;
  if(m_groupBy.isEmpty()) {
    m_groupBy = coll_->defaultGroupAttribute();
  }

  BCUnitGroupDict* dict = coll_->unitGroupDictByName(m_groupBy);
  if(!dict) {
    kdDebug() << "BCGroupView::populateCollection() - no dict found for " << m_groupBy << endl;
    return 0;
  }

  clear();
  m_groupDict.clear();
  //if there is not a root item for the collection, it is created
  ParentItem* collItem = locateItem(coll_);

  QPixmap icon = KGlobal::iconLoader()->loadIcon(coll_->unitName(), KIcon::User);

  // iterate over all the groups in the dict
  // e.g. if the dict is "author", loop over all the author groups
  QDictIterator<BCUnitGroup> it(*dict);
  for(unsigned j = 0; it.current(); ++it, ++j) {
    ParentItem* par = insertItem(collItem, it.current());

    BCUnitListIterator unitIt(*it.current());
    for( ; unitIt.current(); ++unitIt) {
      QString title = unitIt.current()->title();
      BCUnitItem* item = new BCUnitItem(par, title, unitIt.current());
      item->setPixmap(0, icon);
    }
  }
  collItem->setOpen(true);
  return collItem;
}
