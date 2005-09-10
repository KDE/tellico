/***************************************************************************
    copyright            : (C) 2001-2005 by Robby Stephenson
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
#include "document.h"
#include "field.h"
#include "filter.h"
#include "controller.h"
#include "entryitem.h"
#include "entrygroupitem.h"
#include "entry.h"
#include "field.h"
#include "filter.h"
#include "tellico_kernel.h"

#include <kpopupmenu.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kaction.h>

#include <qstringlist.h>
#include <qcolor.h>
#include <qregexp.h>
#include <qheader.h>

using Tellico::GroupView;

// by default don't show the number of items
GroupView::GroupView(QWidget* parent_, const char* name_/*=0*/)
    : GUI::ListView(parent_, name_), m_notSortedYet(true), m_coll(0) {
  // the app name isn't translated
  addColumn(QString::null); // header text gets updated later
  header()->setStretchEnabled(true, 0);
  setResizeMode(QListView::NoColumn);
  setRootIsDecorated(true);
  setShowSortIndicator(true);
  setTreeStepSize(15);
  setFullWidth(true);

  m_groupMenu = new KPopupMenu(this);
  m_groupMenu->insertItem(SmallIconSet(QString::fromLatin1("2downarrow")),
                          i18n("Expand All Groups"), this, SLOT(slotExpandAll()));
  m_groupMenu->insertItem(SmallIconSet(QString::fromLatin1("2uparrow")),
                          i18n("Collapse All Groups"), this, SLOT(slotCollapseAll()));
  m_groupMenu->insertItem(SmallIconSet(QString::fromLatin1("filter")),
                          i18n("Filter by Group"), this, SLOT(slotFilterGroup()));

  m_entryMenu = new KPopupMenu(this);
  Controller::self()->plugEntryActions(m_entryMenu);

  connect(this, SIGNAL(contextMenuRequested(QListViewItem*, const QPoint&, int)),
          SLOT(contextMenuRequested(QListViewItem*, const QPoint&, int)));

  connect(this, SIGNAL(expanded(QListViewItem*)),
          SLOT(slotExpanded(QListViewItem*)));

  connect(this, SIGNAL(collapsed(QListViewItem*)),
          SLOT(slotCollapsed(QListViewItem*)));

  m_groupOpenPixmap = SmallIcon(QString::fromLatin1("folder_open"));
  m_groupClosedPixmap = SmallIcon(QString::fromLatin1("folder"));
}

Tellico::EntryGroupItem* GroupView::addGroup(Data::EntryGroup* group_) {
  EntryGroupItem* item = new EntryGroupItem(this, group_);
  if(group_->groupName() == Data::Collection::s_emptyGroupTitle) {
    item->setPixmap(0, SmallIcon(QString::fromLatin1("folder_red")));
  } else {
    item->setPixmap(0, m_groupClosedPixmap);
  }

  m_groupDict.insert(group_->groupName(), item);

  return item;
}

void GroupView::slotReset() {
  clear();
  // don't really need to clear the collGroupBy map
  m_groupDict.clear();
}

void GroupView::removeCollection(Data::Collection* coll_) {
  if(!coll_) {
    kdWarning() << "GroupView::removeCollection() - null coll pointer!" << endl;
    return;
  }

//  kdDebug() << "GroupView::removeCollection() - " << coll_->title() << endl;

  blockSignals(true);
  slotReset();
  blockSignals(false);
}

void GroupView::slotModifyGroup(Data::Collection* coll_, Data::EntryGroup* group_) {
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
  EntryGroupItem* par = m_groupDict.find(group_->groupName());
  if(!par) {
    par = addGroup(group_);
  }

  //make a copy of the group
  Data::EntryGroup leftover(*group_);

  // first delete all items in this view but no longer in the group
  QListViewItem* item = par->firstChild();
  QListViewItem* next;
  while(item) {
    Data::Entry* entry = static_cast<EntryItem*>(item)->entry();
    if(group_->contains(entry)) {
      leftover.remove(entry);
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

  // next add new listViewItems for items in the group, but not currently in the view
  for(Data::EntryVecIt it = leftover.begin(); it != leftover.end(); ++it) {
    new EntryItem(par, it);
  }

  // don't want any selected
  clearSelection();
  sort(); // in case the count changed, or group name

  // need to refresh
//  triggerUpdate(); // sort does this anyway?
//  root->setOpen(true);
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
  GUI::ListViewItem* it = static_cast<GUI::ListViewItem*>(currentItem());
  if(it && it->isEntryItem() && entry_ == static_cast<EntryItem*>(it)->entry()) {
    return;
  }

  // have to find a group whose field is the same as currently shown
  if(m_groupBy.isEmpty()) {
    kdDebug() << "GroupView::slotSetSelected() - no group field" << endl;
    return;
  }

  const Data::EntryGroup* group = 0;
  for(PtrVector<Data::EntryGroup>::ConstIterator it = entry_->groups().begin(); it != entry_->groups().end(); ++it) {
    if(it->fieldName() == m_groupBy) {
      group = it.ptr();
      break;
    }
  }
  if(!group) {
    kdDebug() << "GroupView::slotSetSelected() - entry is not in any current groups!" << endl;
    return;
  }

  EntryGroupItem* groupItem = m_groupDict.find(group->groupName());
  if(!groupItem) {
    return;
  }

  clearSelection();
  for(QListViewItem* item = groupItem->firstChild(); item; item = item->nextSibling()) {
    EntryItem* entryItem = static_cast<EntryItem*>(item);
    if(entryItem->entry() == entry_) {
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
      item = firstChild()->firstChild();
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

  GUI::ListViewItem* item = static_cast<GUI::ListViewItem*>(item_);
  if(item->isEntryGroupItem() && m_groupMenu->count() > 0) {
    m_groupMenu->popup(point_);
  } else if(item->isEntryItem() && m_entryMenu->count() > 0) {
    m_entryMenu->popup(point_);
  }
}

void GroupView::slotCollapsed(QListViewItem* item_) {
  // only change icon for group items
  if(static_cast<GUI::ListViewItem*>(item_)->isEntryGroupItem()) {
    if(item_->text(0) == Data::Collection::s_emptyGroupTitle) {
      item_->setPixmap(0, SmallIcon(QString::fromLatin1("folder_red")));
    } else {
      item_->setPixmap(0, m_groupClosedPixmap);
    }
  }
}

void GroupView::slotExpanded(QListViewItem* item_) {
  // only change icon for group items
  if(static_cast<GUI::ListViewItem*>(item_)->isEntryGroupItem()) {
    if(item_->text(0) == Data::Collection::s_emptyGroupTitle) {
      item_->setPixmap(0, SmallIcon(QString::fromLatin1("folder_red_open")));
    } else {
      item_->setPixmap(0, m_groupOpenPixmap);
    }
  }
}

void GroupView::addCollection(Data::Collection* coll_) {
  if(!coll_) {
    kdWarning() << "GroupView::addCollection() - null coll pointer!" << endl;
    return;
  }

  m_coll = coll_;
  // if the collection doesn't have the grouped field, and it's not the pseudo-group,
  // change it to default
  if(m_groupBy.isEmpty() || (coll_->fieldByName(m_groupBy) == 0 && m_groupBy != Data::Collection::s_peopleGroupName)) {
    m_groupBy = coll_->defaultGroupField();
  }

  updateHeader();
  populateCollection();

//  slotClearSelection();
//  setSelected(collItem, true);
  slotCollapseAll();
//  kdDebug() << "GroupView::addCollection - done" << endl;
}

void GroupView::setGroupField(const QString& groupField_) {
//  kdDebug() << "GroupView::setGroupField - " << groupField_ << endl;
  if(!m_coll || groupField_.isEmpty() || groupField_ == m_groupBy) {
    return;
  }

  m_groupBy = groupField_;
  if((m_coll->fieldByName(groupField_) && m_coll->fieldByName(groupField_)->formatFlag() == Data::Field::FormatName)
     || groupField_ == Data::Collection::s_peopleGroupName) {
    m_groupOpenPixmap = UserIcon(QString::fromLatin1("person-open"));
    m_groupClosedPixmap = UserIcon(QString::fromLatin1("person"));
  } else {
    m_groupOpenPixmap = SmallIcon(QString::fromLatin1("folder_open"));
    m_groupClosedPixmap = SmallIcon(QString::fromLatin1("folder"));
  }
  updateHeader();
  populateCollection();
}

void GroupView::populateCollection() {
  if(!m_coll) {
    return;
  }

//  kdDebug() << "GroupView::populateCollection() - " << m_groupBy << endl;
  if(m_groupBy.isEmpty()) {
    m_groupBy = m_coll->defaultGroupField();
  }

  setUpdatesEnabled(false);
  clear(); // delete all groups
  m_groupDict.clear();

  // if there's no group field, just return
  if(m_groupBy.isEmpty()) {
    setUpdatesEnabled(true);
    blockSignals(false);
    return;
  }

  Data::EntryGroupDict* dict = m_coll->entryGroupDictByName(m_groupBy);
  if(!dict) { // could happen if m_groupBy is non empty, but there are no entries with a value
    return;
  }

  // iterate over all the groups in the dict
  // e.g. if the dict is "author", loop over all the author groups
  for(QDictIterator<Data::EntryGroup> it(*dict); it.current(); ++it) {
    EntryGroupItem* groupItem = addGroup(it.current());

    for(Data::EntryVecIt entryIt = it.current()->begin(); entryIt != it.current()->end(); ++entryIt) {
      new EntryItem(groupItem, entryIt);
    }
  }

  setUpdatesEnabled(true);
  triggerUpdate();
}

void GroupView::slotFilterGroup() {
  const GUI::ListViewItemList& items = selectedItems();
  GUI::ListViewItem* item = items.getFirst();
  // if no children, can't get collection pointer
  if(!item || !item->isEntryGroupItem()) {
    return;
  }

  Filter* filter = new Filter(Filter::MatchAny);

  for(GUI::ListViewItemListIt it(items); it.current(); ++it) {
    if(it.current()->childCount() == 0) { //ignore empty items
      continue;
    }
    // need to check for people group
    if(m_groupBy == Data::Collection::s_peopleGroupName) {
      Data::Entry* entry = static_cast<EntryItem*>(it.current()->firstChild())->entry();
      Data::FieldVec fields = entry->collection()->peopleFields();
      for(Data::FieldVec::Iterator fIt = fields.begin(); fIt != fields.end(); ++fIt) {
        filter->append(new FilterRule(fIt->name(), it.current()->text(0), FilterRule::FuncContains));
      }
    } else {
      filter->append(new FilterRule(m_groupBy, it.current()->text(0), FilterRule::FuncContains));
    }
  }

  emit signalUpdateFilter(filter);
}

// this gets called when header() is clicked, so cycle through
void GroupView::setSorting(int col_, bool asc_) {
  if(asc_ && !m_notSortedYet) { // cycle through after ascending
    if(sortStyle() == ListView::SortByText) {
      setSortStyle(ListView::SortByCount);
    } else {
      setSortStyle(ListView::SortByText);
    }
  }
  updateHeader();
  m_notSortedYet = false;
  ListView::setSorting(col_, asc_);
}

void GroupView::modifyField(Data::Collection*, Data::Field*, Data::Field* newField_) {
  if(newField_->name() != m_groupBy) {
    return;
  }
  firstChild()->setText(0, newField_->title());
}

void GroupView::updateHeader() {
  if(sortStyle() == ListView::SortByText) {
    setColumnText(0, groupTitle());
  } else {
    setColumnText(0, i18n("%1 (Sort by Count)").arg(groupTitle()));
  }
}

QString GroupView::groupTitle() {
  QString title;
  if(!m_coll || m_groupBy.isEmpty()) {
    title = i18n("Group Name Header", "Group");
  } else {
    Data::Field* f = m_coll->fieldByName(m_groupBy);
    if(f) {
      title = f->title();
    } else if(m_groupBy == Data::Collection::s_peopleGroupName) {
      title = i18n("People");
    }
  }
  return title;
}

#include "groupview.moc"
