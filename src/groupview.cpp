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
#include "collection.h"
#include "field.h"
#include "filter.h"
#include "controller.h"

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
    : MultiSelectionListView(parent_, name_), m_showCount(false) {
  // the app name isn't translated
  addColumn(QString::fromLatin1("Bookcase"));
  addColumn(QString::null, 0); // hide this column, use for sorting by group count
  setResizeMode(QListView::NoColumn);
  // hide the header since there's only one column
  header()->hide();
  setRootIsDecorated(true);
  setTreeStepSize(10);
  // turn off the alternate background color
  setAlternateBackground(QColor());

  QPixmap expand, collapse, filter;
  expand = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("2downarrow"), KIcon::Small);
  collapse = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("2uparrow"), KIcon::Small);
  filter = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("filter"), KIcon::Small);

  m_collMenu = new KPopupMenu(this);
  Controller::self()->plugCollectionActions(m_collMenu);
  m_collMenu->insertSeparator();
  m_collMenu->insertItem(i18n("Sort by Group, Ascending"), this, SLOT(slotSortByGroupAscending()));
  m_collMenu->insertItem(i18n("Sort by Group, Descending"), this, SLOT(slotSortByGroupDescending()));
  m_collMenu->insertItem(i18n("Sort by Count, Ascending"), this, SLOT(slotSortByCountAscending()));
  m_collMenu->insertItem(i18n("Sort by Count, Descending"), this, SLOT(slotSortByCountDescending()));

  m_groupMenu = new KPopupMenu(this);
  m_groupMenu->insertItem(expand, i18n("Expand All Groups"), this, SLOT(slotExpandAll()));
  m_groupMenu->insertItem(collapse, i18n("Collapse All Groups"), this, SLOT(slotCollapseAll()));
  m_groupMenu->insertItem(filter, i18n("Filter by Group"), this, SLOT(slotFilterGroup()));
  m_groupMenu->insertSeparator();
  m_groupMenu->insertItem(i18n("Sort by Group, Ascending"), this, SLOT(slotSortByGroupAscending()));
  m_groupMenu->insertItem(i18n("Sort by Group, Descending"), this, SLOT(slotSortByGroupDescending()));
  m_groupMenu->insertItem(i18n("Sort by Count, Ascending"), this, SLOT(slotSortByCountAscending()));
  m_groupMenu->insertItem(i18n("Sort by Count, Descending"), this, SLOT(slotSortByCountDescending()));

  m_entryMenu = new KPopupMenu(this);
  Controller::self()->plugEntryActions(m_entryMenu);

  connect(this, SIGNAL(contextMenuRequested(QListViewItem*, const QPoint&, int)),
          SLOT(contextMenuRequested(QListViewItem*, const QPoint&, int)));
  // when an entry is double clicked, make sure the editor is visible or toggle parent
  connect(this, SIGNAL(doubleClicked(QListViewItem*)),
          SLOT(slotDoubleClicked(QListViewItem*)));

//  connect(this, SIGNAL(clicked(QListViewItem*)), SLOT(slotSelected(QListViewItem*)));

  connect(this, SIGNAL(selectionChanged()),
          SLOT(slotSelectionChanged()));

  connect(this, SIGNAL(expanded(QListViewItem*)),
          SLOT(slotExpanded(QListViewItem*)));

  connect(this, SIGNAL(collapsed(QListViewItem*)),
          SLOT(slotCollapsed(QListViewItem*)));

  // FIXME: maybe allow this to be customized
  m_collOpenPixmap = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("folder_open"), KIcon::Small);
  m_collClosedPixmap = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("folder"), KIcon::Small);
  m_groupOpenPixmap = m_collOpenPixmap;
  m_groupClosedPixmap = m_collClosedPixmap;
}

Bookcase::ParentItem* GroupView::insertItem(ParentItem* collItem_, const Data::EntryGroup* group_) {
  QString text = group_->groupName();

  ParentItem* par = new ParentItem(collItem_, text, group_);
  par->setPixmap(0, m_groupClosedPixmap);
  par->setCount(group_->count());

  m_groupDict.insert(group_->groupName(), par);

  return par;
}

Bookcase::ParentItem* GroupView::locateItem(ParentItem* collItem_, const Data::EntryGroup* group_) {
  ParentItem* par = m_groupDict.find(group_->groupName());
  if(par) {
    return par;
  }

  return insertItem(collItem_, group_);
}

Bookcase::ParentItem* GroupView::locateItem(Data::Collection* coll_) {
  ParentItem* root = 0;
  // iterate over the collections, which are the top-level children
  for(QListViewItem* collItem = firstChild(); collItem; collItem = collItem->nextSibling()) {
    // find the collItem matching the entry's collection and insert item inside
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

void GroupView::slotReset() {
  // don't really need to clear the collGroupBy map
  m_groupDict.clear();
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
}

void GroupView::slotModifyGroup(Data::Collection* coll_, const Data::EntryGroup* group_) {
  if(!coll_ || !group_) {
    kdWarning() << "GroupView::slotModifyGroup() - null coll or group pointer!" << endl;
    return;
  }

  // if the entries aren't grouped by field of the modified group,
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
    Data::Entry* entry = static_cast<EntryItem*>(item)->entry();
    if(group_->containsRef(entry)) {
      leftover.removeRef(entry);
      item = item->nextSibling();
    } else {
      // if it's not in the group, delete it
//      kdDebug() << "\tdeleting entry - " << entry->title() << endl;
      next = item->nextSibling();
      delete item;
      item = next;
    }
  }

  // if there are no more child items in the group, no entries in the leftover group
  // then delete the parent item
  if(par->childCount() == 0 && leftover.isEmpty()) {
    m_groupDict.remove(par->text(0));
    delete par;
    return;
  }

  QPixmap icon = KGlobal::iconLoader()->loadIcon(coll_->entryName(), KIcon::User);

  // in case the number of entries in the group changed
  par->setCount(group_->count());

  // next add new listViewItems for items in the group, but not currently in the view
  for(Data::EntryListIterator it(leftover); it.current(); ++it) {
    QString title = it.current()->title();

    EntryItem* item = new EntryItem(par, title, it.current());
    item->setPixmap(0, icon);
    // no don't do this after all
//    if(isUpdatesEnabled()) {
//      ensureItemVisible(item);
//      par->setOpen(true);
//    }
  }

  // don't want any selected
  clearSelection();
  sort(); // in case the count changed, or group name

  // need to refresh
  // triggerUpdate(); // sort does this anyway?
  root->setOpen(true);
}

void GroupView::slotSelectionChanged() {
  const QPtrList<MultiSelectionListViewItem>& items = selectedItems();

  if(items.isEmpty()) {
    // FIXME: hack, should have proper clearSelection method
    Controller::self()->slotUpdateSelection(this, Data::EntryList());
    return;
  }

  MultiSelectionListViewItem* item = items.getFirst();
  // check to see if a collection is selected
  if(item->depth() == 0) {
    // FIXME
    // if the item is empty, no way to get a collection pointer
    if(item->childCount() == 0) {
      return;
    }
    // now, group the entry from the first item, and use that to get a collection points
    EntryItem* entryItem = static_cast<EntryItem*>(item->firstChild()->firstChild());
    Controller::self()->slotUpdateSelection(this, entryItem->entry()->collection());
    return;
  }

  // check to see if a group is selected
  if(item->depth() == 1) {
    Controller::self()->slotUpdateSelection(this, static_cast<ParentItem*>(item)->group());
    return;
  }

  // now just iterate over the selected items, making a list of entries
  Data::EntryList list;
  for(QPtrListIterator<MultiSelectionListViewItem> it(items); it.current(); ++it) {
    if(!list.containsRef(static_cast<EntryItem*>(it.current())->entry())) {
      list.append(static_cast<EntryItem*>(it.current())->entry());
    }
  }
  Controller::self()->slotUpdateSelection(this, list);
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

  // if the selected entry is the same as the current one, just return
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
    kdDebug() << "GroupView::slotSetSelected() - entry is not in any current groups!" << endl;
    return;
  }

  ParentItem* par = m_groupDict.find(group->groupName());
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

void GroupView::contextMenuRequested(QListViewItem* item_, const QPoint& point_, int) {
  if(!item_) {
    return;
  }

//  setSelected(item_, true);

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
  if(!selectedItems().isEmpty()) {
    selectAll(false);
  }
  setCurrentItem(0);
  blockSignals(false);
}

void GroupView::addCollection(Data::Collection* coll_) {
  if(!coll_) {
    kdWarning() << "GroupView::addCollection() - null coll pointer!" << endl;
    return;
  }

//  kdDebug() << "GroupView::addCollection" << endl;

  // if the collection doesn't have the grouped field, and it's not the pseudo-group,
  // change it to default
  if(m_groupBy.isEmpty() || (coll_->fieldByName(m_groupBy) == 0 && m_groupBy != Data::Collection::s_peopleGroupName)) {
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
  if(!field && groupField_ != Data::Collection::s_peopleGroupName) {
    return;
  }

  if(m_groupBy != groupField_) {
    m_groupBy = groupField_;
    if((field && field->formatFlag() == Data::Field::FormatName)
       || groupField_ == Data::Collection::s_peopleGroupName) {
      m_groupOpenPixmap = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("person-open"), KIcon::User, KIcon::SizeSmall);
      m_groupClosedPixmap = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("person"), KIcon::User, KIcon::SizeSmall);
    } else {
      m_groupOpenPixmap = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("folder_open"), KIcon::Small);
      m_groupClosedPixmap = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("folder"), KIcon::Small);
    }
    populateCollection(coll_);
  }
}

void GroupView::showCount(bool showCount_) {
//  kdDebug() << "GroupView::showCount()" << endl;
  if(m_showCount != showCount_) {
    m_showCount = showCount_;
    triggerUpdate();
  }
}

Bookcase::ParentItem* GroupView::populateCollection(Data::Collection* coll_) {
//  kdDebug() << "GroupView::populateCollection() - " << m_groupBy << endl;
  if(m_groupBy.isEmpty()) {
    m_groupBy = coll_->defaultGroupField();
  }

  setUpdatesEnabled(false);
  clear();
  m_groupDict.clear();
  // if there is not a root item for the collection, it is created
  ParentItem* collItem = locateItem(coll_);

  // if there's no group field, just return
  if(m_groupBy.isEmpty()) {
    setUpdatesEnabled(true);
    blockSignals(false);
    return collItem;
  }

  Data::EntryGroupDict* dict = coll_->entryGroupDictByName(m_groupBy);
  if(!dict) { // could happen if m_groupBy is non empty, but there are no entries with a value
    return collItem;
  }
  QPixmap icon = KGlobal::iconLoader()->loadIcon(coll_->entryName(), KIcon::User);

  // iterate over all the groups in the dict
  // e.g. if the dict is "author", loop over all the author groups
  for(QDictIterator<Data::EntryGroup> it(*dict); it.current(); ++it) {
    ParentItem* par = insertItem(collItem, it.current());

    for(Data::EntryListIterator entryIt(*it.current()); entryIt.current(); ++entryIt) {
      QString title = entryIt.current()->title();
      EntryItem* item = new EntryItem(par, title, entryIt.current());
      item->setPixmap(0, icon);
    }
  }

  setUpdatesEnabled(true);
  triggerUpdate();

  collItem->setOpen(true);
  return collItem;
}

void GroupView::slotSortByGroupAscending() {
  setSorting(0, true);
}

void GroupView::slotSortByGroupDescending() {
  setSorting(0, false);
}

void GroupView::slotSortByCountAscending() {
  setSorting(1, true);
}

void GroupView::slotSortByCountDescending() {
  setSorting(1, false);
}

void GroupView::slotFilterGroup() {
  Filter* filter = new Filter(Filter::MatchAny);

  // keep track if we need to repaint
  bool update = false;
#if QT_VERSION >= 0x030200
  for(QListViewItemIterator it(this, QListViewItemIterator::Selected); it.current(); ++it) {
#else
  for(QListViewItemIterator it(this); it.current(); ++it) {
    if(!it.current()->isSelected()) {
      continue;
    }
#endif
    if(it.current()->depth() == 1) {
      Data::Entry* entry = static_cast<EntryItem*>(it.current()->firstChild())->entry();
      // need to check for people group
      if(m_groupBy == Data::Collection::s_peopleGroupName) {
        for(Data::FieldListIterator fIt(entry->collection()->fieldList()); fIt.current(); ++fIt) {
          if(fIt.current()->formatFlag() == Data::Field::FormatName) {
            filter->append(new FilterRule(fIt.current()->name(), it.current()->text(0), FilterRule::FuncContains));
          }
        }
      } else {
        filter->append(new FilterRule(m_groupBy, it.current()->text(0), FilterRule::FuncContains));
      }
    } else {
      blockSignals(true);
      it.current()->setSelected(false);
      blockSignals(false);
      update = true;
    }
  }

  if(update) {
    triggerUpdate();
    slotSelectionChanged();
  }
  emit signalUpdateFilter(filter);
}

void GroupView::slotDoubleClicked(QListViewItem* item_) {
  if(!item_) {
    return;
  }
  // if it's a collection or group, just toggle
  if(item_->depth() < 2) {
    item_->setOpen(!item_->isOpen());
    return;
  }

  // otherwise, open entry editor
  EntryItem* i = dynamic_cast<Bookcase::EntryItem*>(item_);
  if(i) {
    Controller::self()->editEntry(*i->entry());
  }
}

bool GroupView::isSelectable(MultiSelectionListViewItem* item_) const {
  // just selecting a single item is always ok
  if(selectedItems().isEmpty()) {
    return true;
  }

  // selecting multiple entries is ok, but not groups or collections
  // and only select if parent is open
  switch(item_->depth()) {
    case 0:
      return false;
    case 1:
      return false;
    case 2:
      return (selectedItems().getFirst()->depth() == 2 && item_->parent()->isOpen());
  }

  // default, just return true;
  return true;
}

#include "groupview.moc"
