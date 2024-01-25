/***************************************************************************
    Copyright (C) 2001-2020 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#include "groupview.h"
#include "collection.h"
#include "field.h"
#include "entry.h"
#include "entrygroup.h"
#include "filter.h"
#include "controller.h"
#include "models/entrygroupmodel.h"
#include "models/groupsortmodel.h"
#include "models/modelmanager.h"
#include "models/models.h"
#include "gui/countdelegate.h"
#include "tellico_debug.h"

#include <KLocalizedString>

#include <QMenu>
#include <QIcon>
#include <QStringList>
#include <QColor>
#include <QHeaderView>
#include <QContextMenuEvent>

using Tellico::GroupView;

GroupView::GroupView(QWidget* parent_)
    : GUI::TreeView(parent_), m_notSortedYet(true) {
  header()->setSectionResizeMode(QHeaderView::Stretch);
  setHeaderHidden(false);
  setSelectionMode(QAbstractItemView::ExtendedSelection);

  connect(this, &GroupView::expanded, this, &GroupView::slotExpanded);
  connect(this, &GroupView::collapsed, this, &GroupView::slotCollapsed);
  connect(this, &GroupView::doubleClicked, this, &GroupView::slotDoubleClicked);
  connect(header(), &QHeaderView::sortIndicatorChanged, this, &GroupView::slotSortingChanged);

  m_groupOpenIconName = QStringLiteral("folder-open");
  m_groupClosedIconName = QStringLiteral("folder");
  EntryGroupModel* groupModel = new EntryGroupModel(this);
  GroupSortModel* sortModel = new GroupSortModel(this);
  sortModel->setSourceModel(groupModel);
  setModel(sortModel);
  setItemDelegate(new GUI::CountDelegate(this));

  ModelManager::self()->setGroupModel(model());
}

Tellico::EntryGroupModel* GroupView::sourceModel() const {
  return static_cast<EntryGroupModel*>(sortModel()->sourceModel());
}

void GroupView::addCollection(Tellico::Data::CollPtr coll_) {
  if(!coll_) {
    myWarning() << "null coll pointer!";
    return;
  }

  m_coll = coll_;
  // if the collection doesn't have the grouped field, and it's not the pseudo-group,
  // change it to default
  if(m_groupBy.isEmpty() || (!coll_->hasField(m_groupBy) && m_groupBy != Data::Collection::s_peopleGroupName)) {
    m_groupBy = coll_->defaultGroupField();
  }

  // when the coll gets set for the first time, the pixmaps need to be updated
  if((m_coll->hasField(m_groupBy) && m_coll->fieldByName(m_groupBy)->formatType() == FieldFormat::FormatName)
     || m_groupBy == Data::Collection::s_peopleGroupName) {
    m_groupOpenIconName = QStringLiteral(":/icons/person-open");
    m_groupClosedIconName = QStringLiteral(":/icons/person");
  }

  updateHeader();
  populateCollection();
  setEntrySortField(m_entrySortField);
}

void GroupView::removeCollection(Tellico::Data::CollPtr coll_) {
  if(!coll_) {
    myWarning() << "null coll pointer!";
    return;
  }

  blockSignals(true);
  slotReset();
  blockSignals(false);
}

void GroupView::populateCollection() {
  if(!m_coll) {
    return;
  }

//  myDebug() << m_groupBy;
  if(m_groupBy.isEmpty()) {
    m_groupBy = m_coll->defaultGroupField();
  }

  setUpdatesEnabled(false);
  slotReset(); // delete all groups

  // if there's no group field, just return
  if(m_groupBy.isEmpty()) {
    setUpdatesEnabled(true);
    return;
  }

  Data::EntryGroupDict* dict = m_coll->entryGroupDictByName(m_groupBy);
  if(!dict) { // could happen if m_groupBy is non empty, but there are no entries with a value
    setUpdatesEnabled(true);
    return;
  }

  // add all the groups
  sourceModel()->addGroups(dict->values(), m_groupClosedIconName);
  // still need to change icon for empty group
  foreach(Data::EntryGroup* group, *dict) {
    if(group->hasEmptyGroupName()) {
      QModelIndex emptyGroupIndex = sourceModel()->indexFromGroup(group);
      sourceModel()->setData(emptyGroupIndex, QLatin1String("folder-red"), Qt::DecorationRole);
      break;
    }
  }

  setUpdatesEnabled(true);
  // must re-sort in order to update view
  model()->sort(0, sortOrder());
}

// don't 'shadow' QListView::setSelected
void GroupView::setEntrySelected(Tellico::Data::EntryPtr entry_) {
//  DEBUG_LINE;
  // if entry_ is null pointer, set no selected
  if(!entry_) {
    // don't move this one outside the block since it calls setCurrentItem(0)
    clearSelection();
    return;
  }

  // if the selected entry is the same as the current one, just return
  QModelIndex curr = currentIndex();
  if(entry_ == sourceModel()->entry(curr)) {
    return;
  }

  // have to find a group whose field is the same as currently shown
  if(m_groupBy.isEmpty()) {
    myDebug() << "no group field";
    return;
  }

  Data::EntryGroup* group = nullptr;
  foreach(Data::EntryGroup* tmpGroup, entry_->groups()) {
    if(tmpGroup->fieldName() == m_groupBy) {
      group = tmpGroup;
      break;
    }
  }
  if(!group) {
    myDebug() << "entry is not in any current groups!";
    return;
  }

  QModelIndex index = sourceModel()->indexFromGroup(group);

  clearSelection();
  for(; index.isValid(); index = index.sibling(index.row()+1, 0)) {
    if(sourceModel()->entry(index) == entry_) {
      blockSignals(true);
      selectionModel()->select(index, QItemSelectionModel::Select);
      setCurrentIndex(index);
      blockSignals(false);
      scrollTo(index);
      break;
    }
  }
}

void GroupView::modifyField(Tellico::Data::CollPtr, Tellico::Data::FieldPtr, Tellico::Data::FieldPtr newField_) {
  if(newField_->name() == m_groupBy) {
    updateHeader(newField_);
  }
  // if the grouping changed at all, our groups got deleted out from under us
  populateCollection();
}

void GroupView::slotReset() {
  sourceModel()->clear();
}

void GroupView::slotModifyGroups(Tellico::Data::CollPtr coll_, QList<Tellico::Data::EntryGroup*> groups_) {
  if(!coll_ || groups_.isEmpty()) {
    myWarning() << "null coll or group pointer!";
    return;
  }

  /* for each group
     - remove existing empty ones
     - modify existing ones
     - add new ones
  */
  foreach(Data::EntryGroup* group, groups_) {
    // if the entries aren't grouped by field of the modified group,
    // we don't care, so skip
    if(m_groupBy != group->fieldName()) {
      continue;
    }

//    myDebug() << group->fieldName() << "/" << group->groupName();
    QModelIndex groupIndex = sourceModel()->indexFromGroup(group);
    if(groupIndex.isValid()) {
      if(group->isEmpty()) {
        sourceModel()->removeGroup(group);
        continue;
      }
      sourceModel()->modifyGroup(group);
    } else {
      if(group->isEmpty()) {
        myDebug() << "trying to add empty group";
        continue;
      }
      addGroup(group);
    }
  }
  // don't want any selected
  clearSelection();
}

void GroupView::contextMenuEvent(QContextMenuEvent* event_) {
  QModelIndex index = indexAt(event_->pos());
  if(!index.isValid()) {
    return;
  }

  QMenu menu(this);
  // no parent means it's a top-level item
  if(!index.parent().isValid()) {
    menu.addAction(QIcon::fromTheme(QStringLiteral("arrow-down-double")),
                   i18n("Expand All Groups"), this, &QTreeView::expandAll);
    menu.addAction(QIcon::fromTheme(QStringLiteral("arrow-up-double")),
                   i18n("Collapse All Groups"), this, &QTreeView::collapseAll);
    menu.addAction(QIcon::fromTheme(QStringLiteral("view-filter")),
                   i18n("Filter by Group"), this, &GroupView::slotFilterGroup);
  } else {
    Controller::self()->plugEntryActions(&menu);
  }
  menu.addSeparator();
  QMenu* sortMenu = Controller::self()->plugSortActions(&menu);
  connect(sortMenu, &QMenu::triggered, this, &GroupView::slotSortMenuActivated);
  menu.exec(event_->globalPos());
}

void GroupView::slotCollapsed(const QModelIndex& index_) {
  if(model()->data(index_, Qt::DecorationRole).toString() == m_groupOpenIconName) {
    model()->setData(index_, m_groupClosedIconName, Qt::DecorationRole);
  }
}

void GroupView::slotExpanded(const QModelIndex& index_) {
  if(model()->data(index_, Qt::DecorationRole).toString() == m_groupClosedIconName) {
    model()->setData(index_, m_groupOpenIconName, Qt::DecorationRole);
  }
}

void GroupView::setGroupField(const QString& groupField_) {
//  myDebug() << groupField_;
  if(groupField_.isEmpty() || groupField_ == m_groupBy) {
    return;
  }

  m_groupBy = groupField_;
  if(!m_coll) {
    return; // can't do anything yet, but still need to set the variable
  }
  if((m_coll->hasField(groupField_) && m_coll->fieldByName(groupField_)->formatType() == Tellico::FieldFormat::FormatName)
     || groupField_ == Tellico::Data::Collection::s_peopleGroupName) {
    m_groupOpenIconName = QStringLiteral(":/icons/person-open");
    m_groupClosedIconName = QStringLiteral(":/icons/person");
  } else {
    m_groupOpenIconName = QStringLiteral("folder-open");
    m_groupClosedIconName = QStringLiteral("folder");
  }
  updateHeader();
  populateCollection();
}

QString GroupView::entrySortField() const {
  return m_entrySortField;
}

void GroupView::setEntrySortField(const QString& groupSortName_) {
  m_entrySortField = groupSortName_;
  GroupSortModel* model = static_cast<GroupSortModel*>(sortModel());
  Q_ASSERT(model);
  model->setEntrySortField(groupSortName_);
}

void GroupView::slotFilterGroup() {
  QModelIndexList indexes = selectionModel()->selectedIndexes();
  if(indexes.isEmpty()) {
    return;
  }

  FilterPtr filter(new Filter(Filter::MatchAny));
  foreach(const QModelIndex& index, indexes) {
    // the indexes pointing to a group have no parent
    if(index.parent().isValid()) {
      continue;
    }

    if(!model()->hasChildren(index)) { //ignore empty items
      continue;
    }
    // need to check for people group
    if(m_groupBy == Data::Collection::s_peopleGroupName) {
      QModelIndex firstChild = model()->index(0, 0, index);
      QModelIndex sourceIndex = sortModel()->mapToSource(firstChild);
      Data::EntryPtr entry = sourceModel()->entry(sourceIndex);
      Q_ASSERT(entry);
      Data::FieldList fields = entry->collection()->peopleFields();
      foreach(Data::FieldPtr field, fields) {
        filter->append(new FilterRule(field->name(),
                                      FieldFormat::matchValueRegularExpression(model()->data(index).toString()),
                                      FilterRule::FuncRegExp));
      }
    } else {
      Data::EntryGroup* group = model()->data(index, GroupPtrRole).value<Data::EntryGroup*>();
      if(group) {
        if(group->hasEmptyGroupName()) {
          filter->append(new FilterRule(m_groupBy, QString(), FilterRule::FuncEquals));
        } else {
          // if the field does not allow multiple values and is not a table
          // then can just do an equal match
          Data::FieldPtr field = group->at(0)->collection()->fieldByName(group->fieldName());
          if(field && field->type() != Data::Field::Table && !field->hasFlag(Data::Field::AllowMultiple)) {
            filter->append(new FilterRule(m_groupBy, group->groupName(), FilterRule::FuncEquals));
          } else {
            filter->append(new FilterRule(m_groupBy,
                                          FieldFormat::matchValueRegularExpression(group->groupName()),
                                          FilterRule::FuncRegExp));
          }
        }
      }
    }
  }

  if(!filter->isEmpty()) {
    emit signalUpdateFilter(filter);
  }
}

void GroupView::slotDoubleClicked(const QModelIndex& index_) {
  QModelIndex realIndex = sortModel()->mapToSource(index_);
  Data::EntryPtr entry = sourceModel()->entry(realIndex);
  if(entry) {
    Controller::self()->editEntry(entry);
  }
}

// this gets called when header() is clicked, so cycle through
void GroupView::slotSortingChanged(int col_, Qt::SortOrder order_) {
  Q_UNUSED(col_);
  if(order_ == Qt::AscendingOrder && !m_notSortedYet) { // cycle through after ascending
    if(sortModel()->sortRole() == RowCountRole) {
      sortModel()->setSortRole(Qt::DisplayRole);
    } else {
      sortModel()->setSortRole(RowCountRole);
    }
  }

  updateHeader();
  m_notSortedYet = false;
}

void GroupView::slotSortMenuActivated(QAction* action_) {
  Data::FieldPtr field = action_->data().value<Data::FieldPtr>();
  Q_ASSERT(field);
  if(!field) {
    return;
  }
  setEntrySortField(field->name());
}

void GroupView::updateHeader(Tellico::Data::FieldPtr field_/*=0*/) {
  QString t = field_ ? field_->title() : groupTitle();
  if(sortModel()->sortRole() == Qt::DisplayRole) {
    model()->setHeaderData(0, Qt::Horizontal, t);
  } else {
    model()->setHeaderData(0, Qt::Horizontal, i18n("%1 (Sort by Count)", t));
  }
}

QString GroupView::groupTitle() {
  QString title;
  if(!m_coll || m_groupBy.isEmpty()) {
    title = i18nc("Group Name Header", "Group");
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

void GroupView::addGroup(Tellico::Data::EntryGroup* group_) {
  if(group_->isEmpty()) {
    return;
  }
  QModelIndex index = sourceModel()->addGroup(group_);
  if(group_->hasEmptyGroupName()) {
    sourceModel()->setData(index, QLatin1String("folder-red"), Qt::DecorationRole);
  } else {
    sourceModel()->setData(index, m_groupClosedIconName, Qt::DecorationRole);
  }
}
