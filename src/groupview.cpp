/***************************************************************************
    copyright            : (C) 2001-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "groupview.h"
#include "mainwindow.h"
#include "collection.h"
#include "field.h"
#include "utils.h"

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

using Bookcase::GroupView;

// by default don't show the number of items
GroupView::GroupView(QWidget* parent_, const char* name_/*=0*/)
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

  MainWindow* bookcase = static_cast<MainWindow*>(QObjectAncestor(parent_, "Bookcase::MainWindow"));
  m_collMenu = new KPopupMenu(this);
  bookcase->action("edit_rename_collection")->plug(m_collMenu);
  bookcase->action("edit_fields")->plug(m_collMenu);
  bookcase->action("edit_convert_bibtex")->plug(m_collMenu);
  bookcase->action("edit_string_macros")->plug(m_collMenu);

  m_groupMenu = new KPopupMenu(this);
  m_groupMenu->insertItem(expand, i18n("Expand All Groups"), this, SLOT(slotExpandAll()));
  m_groupMenu->insertItem(collapse, i18n("Collapse All Groups"), this, SLOT(slotCollapseAll()));

  m_entryMenu = new KPopupMenu(this);
  bookcase->action("edit_edit_entry")->plug(m_entryMenu);
  bookcase->action("edit_copy_entry")->plug(m_entryMenu);
  bookcase->action("edit_delete_entry")->plug(m_entryMenu);

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
  m_collOpenPixmap = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("folder_open"), KIcon::Small);
  m_collClosedPixmap = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("folder"), KIcon::Small);
  m_groupOpenPixmap = m_collOpenPixmap;
  m_groupClosedPixmap = m_collClosedPixmap;
}

inline
QString GroupView::groupKey(const ParentItem* par_, QListViewItem* item_) const {
  return QString::number(par_->id()) + item_->text(0);
}

inline
QString GroupView::groupKey(const ParentItem* par_, const Data::EntryGroup* group_) const {
  return QString::number(par_->id()) + group_->groupName();
}

inline
QString GroupView::groupKey(const Data::Collection* coll_, QListViewItem* item_) const {
  return QString::number(coll_->id()) + item_->text(0);
}

inline
QString GroupView::groupKey(const Data::Collection* coll_, const Data::EntryGroup* group_) const {
  return QString::number(coll_->id()) + group_->groupName();
}

Bookcase::ParentItem* GroupView::insertItem(ParentItem* collItem_, const Data::EntryGroup* group_) {
  QString text = group_->groupName();

  ParentItem* par = new ParentItem(collItem_, text);
  par->setPixmap(0, m_groupClosedPixmap);
  par->setCount(group_->count());

  QString key = groupKey(collItem_, group_);
  m_groupDict.insert(key, par);

  return par;
}

Bookcase::ParentItem* GroupView::locateItem(ParentItem* collItem_, const Data::EntryGroup* group_) {
  QString key = groupKey(collItem_, group_);
  ParentItem* par = m_groupDict.find(key);
  if(par) {
    return par;
  }

  return insertItem(collItem_, group_);
}

Bookcase::ParentItem* GroupView::locateItem(Data::Collection* coll_) {
  ParentItem* root = 0;
  // iterate over the collections, which are the top-level children
  for(QListViewItem* collItem = firstChild(); collItem; collItem = collItem->nextSibling()) {
    // find the collItem matching the unit's collection and insert item inside
    ParentItem* par = static_cast<ParentItem*>(collItem);
    if(par->id() == coll_->id()) {
      root = par;
      break;
    }
  }
  if(!root) {
//    kdDebug() << "GroupView::locateItem() - adding new collection" << endl;
    root = new ParentItem(this, coll_->title(), coll_->id());
    root->setPixmap(0, m_collClosedPixmap);
  }
  return root;
}

const QString& GroupView::groupBy() const {
  return m_groupBy;
}

void GroupView::slotReset() {
  // don't really need to clear the collGroupBy map
  m_groupDict.clear();
  m_selectedEntries.clear();
  clear();
}

void GroupView::removeCollection(Data::Collection* coll_) {
  if(!coll_) {
    kdWarning() << "GroupView::removeCollection() - null coll pointer!" << endl;
    return;
  }
//  kdDebug() << "GroupView::removeCollection() - " << coll_->title() << endl;

#if 0 // since there is never more than one collection
  ParentItem* collItem = locateItem(coll_);
  // first remove all groups in collection from dict
  for(QListViewItem* groupItem = collItem->firstChild(); groupItem; groupItem = groupItem->nextSibling()) {
    m_groupDict.remove(groupKey(coll_, groupItem));
  }
  // automatically deletes all children
  delete collItem;
#endif
  blockSignals(true);
  clear();
  m_groupDict.clear();
  blockSignals(false);

  m_selectedEntries.clear();
}

void GroupView::slotModifyGroup(Data::Collection* coll_, const Data::EntryGroup* group_) {
  if(!coll_ || !group_) {
    kdWarning() << "GroupView::slotModifyGroup() - null coll or group pointer!" << endl;
    return;
  }

  // if the units aren't grouped by field of the modified group,
  // we don't care, so return
  if(m_groupBy != group_->fieldName()) {
    return;
  }

//  kdDebug() << "GroupView::slotModifyGroup() - " << group_->fieldName() << endl;

  ParentItem* root = locateItem(coll_);
  ParentItem* par = locateItem(root, group_);

  //make a copy of the group
  Data::EntryGroup leftover(*group_);

  // first delete all items in this view but no longer in the group
  QListViewItem* item = par->firstChild();
  QListViewItem* next;
  while(item) {
    Data::Entry* unit = static_cast<EntryItem*>(item)->entry();
    if(group_->containsRef(unit)) {
      leftover.removeRef(unit);
      item = item->nextSibling();
    } else {
      // if it's not in the group, delete it
//      kdDebug() << "\tdeleting unit - " << unit->title() << endl;
      m_selectedEntries.removeRef(unit);
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

  QPixmap icon = KGlobal::iconLoader()->loadIcon(coll_->entryName(), KIcon::User);

  // in case the number of units in the group changed
  par->setCount(group_->count());

  // next add new listViewItems for items in the group, but not currently in the view
  for(Data::EntryListIterator it(leftover); it.current(); ++it) {
    QString title = it.current()->title();

    EntryItem* item = new EntryItem(par, title, it.current());
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

void GroupView::slotSelectionChanged() {
//  kdDebug() << "GroupView::slotSelectionChanged()" << endl;
  // since the group view might have multiple unit items
  // that point to the same unit, need to keep track separately
  Data::EntryList newSelected;

  // all items with depth of 2 in the listview are unitItems
  EntryItem* item;
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
      item = static_cast<EntryItem*>(it.current());
      newSelected.append(item->entry());

      if(item->entry() && !m_selectedEntries.containsRef(item->entry())) {
        // the reason I do it this way is because I want the first item in the list
        // to be the first item that was selected, so the edit widget can fill its
        // contents with the first unit
        m_selectedEntries.append(item->entry());
      }
    }
  }

  // now if any units in m_selectedEntries are not in newSelected, remove them
  Data::EntryListIterator uIt(m_selectedEntries);
  while(uIt.current()) {
    if(!newSelected.containsRef(uIt.current())) {
      m_selectedEntries.removeRef(uIt.current()); // when a unit is removed, the iterator goes to next
    } else {
      ++uIt;
    }
  }

  emit signalEntrySelected(this, m_selectedEntries);
}

// don't 'shadow' QListView::setSelected
void GroupView::setEntrySelected(Data::Entry* entry_) {
//  kdDebug() << "GroupView::slotSetSelected()" << endl;
  // if entry_ is null pointer, set no selected
  if(!entry_) {
    // don't move this one outside the block since it calls setCurrentItem(0)
    clearSelection();
    return;
  }

  // if the selected unit is the same as the current one, just return
  if(currentItem() && entry_ == static_cast<EntryItem*>(currentItem())->entry()) {
    return;
  }

  // have to find a group whose field is the same as currently shown
  if(m_groupBy.isEmpty()) {
    kdDebug() << "GroupView::slotSetSelected() - no group field" << endl;
    return;
  }

  Data::EntryGroup* group = 0;
  for(QPtrListIterator<Data::EntryGroup> groupIt(entry_->groups()); groupIt.current(); ++groupIt) {
    if(groupIt.current()->fieldName() == m_groupBy) {
      group = groupIt.current();
      break;
    }
  }
  if(!group) {
    kdDebug() << "GroupView::slotSetSelected() - unit is not in any current groups!" << endl;
    return;
  }

  QString key = groupKey(entry_->collection(), group);
  ParentItem* par = m_groupDict.find(key);
  if(!par) {
    return;
  }

  for(QListViewItem* item = par->firstChild(); item; item = item->nextSibling()) {
    EntryItem* entryItem = static_cast<EntryItem*>(item);
    if(entryItem->entry() == entry_) {
      clearSelection();
      blockSignals(true);
      setSelected(item, true);
      setCurrentItem(item);
      blockSignals(false);
      ensureItemVisible(item);
      return;
    }
  }
}

void GroupView::slotToggleItem(QListViewItem* item_) {
  if(!item_) {
    return;
  }
  item_->setOpen(!item_->isOpen());
}

void GroupView::slotExpandAll(int depth_/*=-1*/) {
  if(childCount() == 0) {
    return;
  }
  setSiblingsOpen(depth_, true);
}

void GroupView::slotCollapseAll(int depth_/*=-1*/) {
  if(childCount() == 0) {
    return;
  }
  setSiblingsOpen(depth_, false);
}

void GroupView::setSiblingsOpen(int depth_, bool open_) {
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

void GroupView::slotRMB(QListViewItem* item_, const QPoint& point_, int) {
  if(!item_) {
    return;
  }

  setSelected(item_, true);

  if(item_->depth() == 0 && m_collMenu->count() > 0) {
    m_collMenu->popup(point_);
  } else if(item_->depth() == 1 && m_groupMenu->count() > 0) {
    m_groupMenu->popup(point_);
  } else if(item_->depth() == 2 && m_entryMenu->count() > 0) {
    m_entryMenu->popup(point_);
  }
}

void GroupView::renameCollection(const QString& name_) {
  // items at depth==0 are for collections
  QListViewItem* item = firstChild();
  if(item) {
    item->setText(0, name_);
  }
}

void GroupView::slotCollapsed(QListViewItem* item_) {
  // only change icon for collection and group items
  if(item_->depth() == 0) {
    item_->setPixmap(0, m_collClosedPixmap);
  } else if(item_->depth() == 1) {
    item_->setPixmap(0, m_groupClosedPixmap);
  }
}

void GroupView::slotExpanded(QListViewItem* item_) {
  // only change icon for collection and group items
  if(item_->depth() == 0) {
    item_->setPixmap(0, m_collOpenPixmap);
  } else if(item_->depth() == 1) {
    item_->setPixmap(0, m_groupOpenPixmap);
  }
}

void GroupView::clearSelection() {
//  kdDebug() << "GroupView::clearSelection()" << endl;
  blockSignals(true);
  if(isUpdatesEnabled()) {
    selectAll(false);
  }
  setCurrentItem(0);
  blockSignals(false);
  m_selectedEntries.clear();
}

void GroupView::addCollection(Data::Collection* coll_) {
  if(!coll_) {
    kdWarning() << "GroupView::addCollection() - null coll pointer!" << endl;
    return;
  }
  
//  kdDebug() << "GroupView::addCollection" << endl;

  // if the collection doesn't have the grouped field, and it's not the pseudo-group,
  // change it to default
  if(coll_->fieldByName(m_groupBy) == 0 && m_groupBy != QString::fromLatin1("_people")) {
    m_groupBy = coll_->defaultGroupField();
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
//  kdDebug() << "GroupView::addCollection - done" << endl;
}

void GroupView::setGroupField(Data::Collection* coll_, const QString& groupField_) {
//  kdDebug() << "GroupView::setGroupField - " << groupField_ << endl;
  if(groupField_.isEmpty()) {
    return;
  }

  // groupField_ could be"_people" for the pseudo-group;
  Data::Field* field = coll_->fieldByName(groupField_);
  if(!field && groupField_ != QString::fromLatin1("_people")) {
    return;
  }
  
  // as a hack, when a new collection is added, this gets called
  // if the collection item is empty, go ahead and populate it
  // even if the group field has not changed
//  ParentItem* collItem = locateItem(coll_);
//  if(m_groupBy != groupField_ || collItem->childCount() == 0) {
  if(m_groupBy != groupField_) {
    m_groupBy = groupField_;
    if((field && field->formatFlag() == Data::Field::FormatName)
       || groupField_ == QString::fromLatin1("_people")) {
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

bool GroupView::showCount() const {
  return m_showCount;
}

void GroupView::showCount(bool showCount_) {
//  kdDebug() << "GroupView::showCount()" << endl;
  if(m_showCount != showCount_) {
    m_showCount = showCount_;
    triggerUpdate();
  }
}

Bookcase::ParentItem* GroupView::populateCollection(Data::Collection* coll_) {
//  kdDebug() << "GroupView::populateCollection()" << endl;
  if(m_groupBy.isEmpty()) {
    m_groupBy = coll_->defaultGroupField();
  }

  clear();
  m_groupDict.clear();
  //if there is not a root item for the collection, it is created
  ParentItem* collItem = locateItem(coll_);

  // if there's no group field, just return
  if(m_groupBy.isEmpty()) {
    return collItem;
  }

  Data::EntryGroupDict* dict = coll_->entryGroupDictByName(m_groupBy);
  QPixmap icon = KGlobal::iconLoader()->loadIcon(coll_->entryName(), KIcon::User);

  // iterate over all the groups in the dict
  // e.g. if the dict is "author", loop over all the author groups
  unsigned j = 0;
  for(QDictIterator<Data::EntryGroup> it(*dict); it.current(); ++it, ++j) {
    ParentItem* par = insertItem(collItem, it.current());

    for(Data::EntryListIterator unitIt(*it.current()); unitIt.current(); ++unitIt) {
      QString title = unitIt.current()->title();
      EntryItem* item = new EntryItem(par, title, unitIt.current());
      item->setPixmap(0, icon);
    }
  }
  collItem->setOpen(true);
  return collItem;
}
