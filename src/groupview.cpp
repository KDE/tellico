/***************************************************************************
    copyright            : (C) 2001-2006 by Robby Stephenson
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

GroupView::GroupView(QWidget* parent_, const char* name_/*=0*/)
    : GUI::ListView(parent_, name_), m_notSortedYet(true), m_coll(0) {
  addColumn(QString::null); // header text gets updated later
  header()->setStretchEnabled(true, 0);
  setResizeMode(QListView::NoColumn);
  setRootIsDecorated(true);
  setShowSortIndicator(true);
  setTreeStepSize(15);
  setFullWidth(true);

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
  int type = -1;
  if(m_coll && m_coll->hasField(group_->fieldName())) {
    type = m_coll->fieldByName(group_->fieldName())->type();
  }
  EntryGroupItem* item = new EntryGroupItem(this, group_, type);
  if(group_->groupName() == Data::Collection::s_emptyGroupTitle) {
    item->setPixmap(0, SmallIcon(QString::fromLatin1("folder_red")));
    item->setSortWeight(10);
  } else {
    item->setPixmap(0, m_groupClosedPixmap);
  }

  m_groupDict.insert(group_->groupName(), item);
  item->setExpandable(!group_->isEmpty());

  return item;
}

void GroupView::slotReset() {
  clear();
  m_groupDict.clear();
}

void GroupView::removeCollection(Data::CollPtr coll_) {
  if(!coll_) {
    kdWarning() << "GroupView::removeCollection() - null coll pointer!" << endl;
    return;
  }

//  kdDebug() << "GroupView::removeCollection() - " << coll_->title() << endl;

  blockSignals(true);
  slotReset();
  blockSignals(false);
}

void GroupView::slotModifyGroup(Data::CollPtr coll_, Data::EntryGroup* group_) {
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
  if(par) {
    if(group_->isEmpty()) {
      m_groupDict.remove(par->text(0));
      delete par;
      return;
    }
    // the group might get deleted and recreated out from under us,
    // so do a sanity check
    par->setGroup(group_);
  } else {
    par = addGroup(group_);
  }

  setUpdatesEnabled(false);
  bool open = par->isOpen();
  par->setOpen(false); // closing and opening the item will clear the items
  par->setOpen(open);
  setUpdatesEnabled(true);

  // don't want any selected
  clearSelection();
  sort(); // in case the count changed, or group name
}

// don't 'shadow' QListView::setSelected
void GroupView::setEntrySelected(Data::EntryPtr entry_) {
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

  KPopupMenu menu(this);
  GUI::ListViewItem* item = static_cast<GUI::ListViewItem*>(item_);
  if(item->isEntryGroupItem()) {
    menu.insertItem(SmallIconSet(QString::fromLatin1("2downarrow")),
                    i18n("Expand All Groups"), this, SLOT(slotExpandAll()));
    menu.insertItem(SmallIconSet(QString::fromLatin1("2uparrow")),
                    i18n("Collapse All Groups"), this, SLOT(slotCollapseAll()));
    menu.insertItem(SmallIconSet(QString::fromLatin1("filter")),
                    i18n("Filter by Group"), this, SLOT(slotFilterGroup()));
  } else if(item->isEntryItem()) {
    Controller::self()->plugEntryActions(&menu);
  }
  menu.exec(point_);
}

void GroupView::slotCollapsed(QListViewItem* item_) {
  // only change icon for group items
  if(static_cast<GUI::ListViewItem*>(item_)->isEntryGroupItem()) {
    if(item_->text(0) == Data::Collection::s_emptyGroupTitle) {
      item_->setPixmap(0, SmallIcon(QString::fromLatin1("folder_red")));
    } else {
      item_->setPixmap(0, m_groupClosedPixmap);
    }
    static_cast<GUI::ListViewItem*>(item_)->clear();
  }
}

void GroupView::slotExpanded(QListViewItem* item_) {
  // only change icon for group items
  if(!static_cast<GUI::ListViewItem*>(item_)->isEntryGroupItem()) {
    kdWarning() << "GroupView::slotExpanded() - non entry group item - " << item_->text(0) << endl;
    return;
  }

  setUpdatesEnabled(false);

  EntryGroupItem* item = static_cast<EntryGroupItem*>(item_);
  if(item->text(0) == Data::Collection::s_emptyGroupTitle) {
    item->setPixmap(0, SmallIcon(QString::fromLatin1("folder_red_open")));
  } else {
    item->setPixmap(0, m_groupOpenPixmap);
  }

  Data::EntryGroup* group = item->group();
  for(Data::EntryVecIt entryIt = group->begin(); entryIt != group->end(); ++entryIt) {
    new EntryItem(item, entryIt);
  }

  setUpdatesEnabled(true);
  triggerUpdate();
}

void GroupView::addCollection(Data::CollPtr coll_) {
//  myDebug() << "GroupView::addCollection()" << endl;
  if(!coll_) {
    kdWarning() << "GroupView::addCollection() - null coll pointer!" << endl;
    return;
  }

  m_coll = coll_;
  // if the collection doesn't have the grouped field, and it's not the pseudo-group,
  // change it to default
  if(m_groupBy.isEmpty() || (!coll_->hasField(m_groupBy) && m_groupBy != Data::Collection::s_peopleGroupName)) {
    m_groupBy = coll_->defaultGroupField();
  }

  // when the coll gets set for the first time, the pixmaps need to be updated
  if((m_coll->hasField(m_groupBy) && m_coll->fieldByName(m_groupBy)->formatFlag() == Data::Field::FormatName)
     || m_groupBy == Data::Collection::s_peopleGroupName) {
    m_groupOpenPixmap = UserIcon(QString::fromLatin1("person-open"));
    m_groupClosedPixmap = UserIcon(QString::fromLatin1("person"));
  }

  updateHeader();
  populateCollection();

  slotCollapseAll();
//  kdDebug() << "GroupView::addCollection - done" << endl;
}

void GroupView::setGroupField(const QString& groupField_) {
//  myDebug() << "GroupView::setGroupField - " << groupField_ << endl;
  if(groupField_.isEmpty() || groupField_ == m_groupBy) {
    return;
  }

  m_groupBy = groupField_;
  if(!m_coll) {
    return; // can't do anything yet, but still need to set the variable
  }
  if((m_coll->hasField(groupField_) && m_coll->fieldByName(groupField_)->formatFlag() == Data::Field::FormatName)
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

//  myDebug() << "GroupView::populateCollection() - " << m_groupBy << endl;
  if(m_groupBy.isEmpty()) {
    m_groupBy = m_coll->defaultGroupField();
  }

  setUpdatesEnabled(false);
  clear(); // delete all groups
  m_groupDict.clear();

  // if there's no group field, just return
  if(m_groupBy.isEmpty()) {
    setUpdatesEnabled(true);
    return;
  }

  Data::EntryGroupDict* dict = m_coll->entryGroupDictByName(m_groupBy);
  if(!dict) { // could happen if m_groupBy is non empty, but there are no entries with a value
    return;
  }

  // iterate over all the groups in the dict
  // e.g. if the dict is "author", loop over all the author groups
  for(QDictIterator<Data::EntryGroup> it(*dict); it.current(); ++it) {
    addGroup(it.current());
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

  FilterPtr filter = new Filter(Filter::MatchAny);

  for(GUI::ListViewItemListIt it(items); it.current(); ++it) {
    if(it.current()->childCount() == 0) { //ignore empty items
      continue;
    }
    // need to check for people group
    if(m_groupBy == Data::Collection::s_peopleGroupName) {
      Data::EntryPtr entry = static_cast<EntryItem*>(it.current()->firstChild())->entry();
      Data::FieldVec fields = entry->collection()->peopleFields();
      for(Data::FieldVec::Iterator fIt = fields.begin(); fIt != fields.end(); ++fIt) {
        filter->append(new FilterRule(fIt->name(), it.current()->text(0), FilterRule::FuncContains));
      }
    } else {
      QString s = it.current()->text(0);
      if(s != Data::Collection::s_emptyGroupTitle) {
        filter->append(new FilterRule(m_groupBy, it.current()->text(0), FilterRule::FuncContains));
      }
    }
  }

  if(!filter->isEmpty()) {
    emit signalUpdateFilter(filter);
  }
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

void GroupView::modifyField(Data::CollPtr, Data::FieldPtr, Data::FieldPtr newField_) {
  if(newField_->name() != m_groupBy) {
    return;
  }
  updateHeader(newField_);
}

void GroupView::updateHeader(Data::FieldPtr field_/*=0*/) {
  QString t = field_ ? field_->title() : groupTitle();
  if(sortStyle() == ListView::SortByText) {
    setColumnText(0, t);
  } else {
    setColumnText(0, i18n("%1 (Sort by Count)").arg(t));
  }
}

QString GroupView::groupTitle() {
  QString title;
  if(!m_coll || m_groupBy.isEmpty()) {
    title = i18n("Group Name Header", "Group");
  } else {
    Data::FieldPtr f = m_coll->fieldByName(m_groupBy);
    if(f) {
      title = f->title();
    } else if(m_groupBy == Data::Collection::s_peopleGroupName) {
      title = i18n("People");
    }
  }
  return title;
}

#include "groupview.moc"
