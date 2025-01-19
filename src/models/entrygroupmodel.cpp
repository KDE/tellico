/***************************************************************************
    Copyright (C) 2008-2009 Robby Stephenson <robby@periapsis.org>
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

#include "entrygroupmodel.h"
#include "models.h"
#include "../entrygroup.h"
#include "../collectionfactory.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <QIcon>

using Tellico::EntryGroupModel;

class EntryGroupModel::Node {
public:
  Node(Node* parent_) : m_parent(parent_), m_row(-1) { }
  ~Node() { qDeleteAll(m_children); }

  Node* parent() const { return m_parent; }
  Node* child(int row) const { return row < m_children.count() ? m_children.at(row) : nullptr; }
  int row() const { return m_row; }
  int childCount() const { return m_children.count(); };

  void addChild(Node* child) {
    child->m_row = m_children.count();
    m_children.append(child);
  }
  void removeChild(int i) {
    delete m_children.takeAt(i);
    // all subsequent children move up a row
    for(int j = i; j < m_children.count(); ++j) { --m_children.at(j)->m_row; }
  }
  void removeAll() {
    qDeleteAll(m_children);
    m_children.clear();
  }

private:
  Node* m_parent;
  QList<Node*> m_children;
  int m_row;
};

EntryGroupModel::EntryGroupModel(QObject* parent) : QAbstractItemModel(parent), m_rootNode(new Node(nullptr)) {
  m_groupHeader = i18nc("Group Name Header", "Group");
}

EntryGroupModel::~EntryGroupModel() {
  delete m_rootNode;
  m_rootNode = nullptr;
}

int EntryGroupModel::rowCount(const QModelIndex& index_) const {
  if(!index_.isValid()) {
    return m_rootNode->childCount();
  }

  Node* node = static_cast<Node*>(index_.internalPointer());
  Q_ASSERT(node);
  return node->childCount();
}

int EntryGroupModel::columnCount(const QModelIndex&) const {
  return 1;
}

QVariant EntryGroupModel::headerData(int section_, Qt::Orientation orientation_, int role_) const {
  if(section_ < 0 || section_ >= columnCount() || orientation_ != Qt::Horizontal) {
    return QVariant();
  }
  if(role_ == Qt::DisplayRole) {
    return m_groupHeader;
  }
  return QVariant();
}

bool EntryGroupModel::setHeaderData(int section_, Qt::Orientation orientation_,
                                    const QVariant& value_, int role_) {
  if(section_ < 0 || section_ >= columnCount() || orientation_ != Qt::Horizontal || role_ != Qt::EditRole) {
    return false;
  }
  m_groupHeader = value_.toString();
  Q_EMIT headerDataChanged(orientation_, section_, section_);
  return true;
}

QVariant EntryGroupModel::data(const QModelIndex& index_, int role_) const {
  if(!index_.isValid()) {
    return QVariant();
  }

  switch(role_) {
    case Qt::DisplayRole:
      if(hasValidParent(index_)) {
        // it probably points to an entry
        Tellico::Data::EntryPtr e = entry(index_);
        if(e) {
          return e->title();
        }
      } else {
        // it probably points to a group
        Tellico::Data::EntryGroup* g = group(index_);
        if(g) {
          return g->groupName();
        }
      }
      return QString(); // DisplayRole should get an empty string, supposedly...
    case Qt::DecorationRole:
      if(hasValidParent(index_)) {
        // assume all the entries have the same icon
        // so no need to lookup the entry(index_), just use first one we find
        foreach(Data::EntryGroup* group, m_groups) {
          if(!group->isEmpty()) {
            return QIcon(QLatin1String(":/icons/") + CollectionFactory::typeName(group->first()->collection()));
          }
        }
      }
      // for groups, check the icon name list
      return QIcon::fromTheme(m_groupIconNames.at(index_.row()));
    case RowCountRole:
      return rowCount(index_);
    case EntryPtrRole:
      return QVariant::fromValue(entry(index_));
    case GroupPtrRole:
      return QVariant::fromValue(group(index_));
    case ValidParentRole:
      return hasValidParent(index_);
  }

  return QVariant();
}

bool EntryGroupModel::setData(const QModelIndex& index_, const QVariant& value_, int role_) {
  // if the index has a parent, then it's not a group
  if(!index_.isValid() || hasValidParent(index_) || index_.row() >= rowCount() || role_ != Qt::DecorationRole) {
    return false;
  }
  m_groupIconNames.replace(index_.row(), value_.toString());
  return true;
}

QModelIndex EntryGroupModel::index(int row_, int column_, const QModelIndex& parent_) const {
  if(!hasIndex(row_, column_, parent_)) {
    return QModelIndex();
  }

  Node* parentNode;
  if(parent_.isValid()) {
    parentNode = static_cast<Node*>(parent_.internalPointer());
  } else {
    parentNode = m_rootNode;
  }

  Node* child = parentNode->child(row_);
  Q_ASSERT(child);
/*
if(!child) {
    myWarning() << "no child at row" << row_;
    return QModelIndex();
  }
*/
  return createIndex(row_, column_, child);
}

QModelIndex EntryGroupModel::parent(const QModelIndex& index_) const {
  if(!index_.isValid()) {
    return QModelIndex();
  }

  Node* node = static_cast<Node*>(index_.internalPointer());
  Q_ASSERT(node);
  Node* parentNode = node->parent();
  Q_ASSERT(parentNode);

  // if it's top-level, it has no parent
  if(parentNode == m_rootNode) {
    return QModelIndex();
  }
  return createIndex(parentNode->row(), 0, parentNode);
}

void EntryGroupModel::clear() {
  beginResetModel();
  m_groups.clear();
  delete m_rootNode;
  m_rootNode = new Node(nullptr);
  m_groupIconNames.clear();
  endResetModel();
}

void EntryGroupModel::addGroups(const QList<Tellico::Data::EntryGroup*>& groups_, const QString& iconName_) {
  if(groups_.isEmpty()) { // shouldn't ever happen
    myWarning() << "adding empty group list!";
    return;
  }
  beginInsertRows(QModelIndex(), rowCount(), rowCount()+groups_.count()-1);
  m_groups += groups_;
  foreach(Tellico::Data::EntryGroup* group, groups_) {
    Node* groupNode = new Node(m_rootNode);
    m_rootNode->addChild(groupNode);
    for(int i = 0; i < group->count(); ++i) {
      Node* childNode = new Node(groupNode);
      groupNode->addChild(childNode);
    }
    m_groupIconNames.append(iconName_);
  }
  endInsertRows();
}

QModelIndex EntryGroupModel::addGroup(Tellico::Data::EntryGroup* group_) {
  Q_ASSERT(group_);
  addGroups(QList<Tellico::Data::EntryGroup*>() << group_, QString());
  // rowCount() has increased now
  return index(rowCount()-1, 0);
}

QModelIndex EntryGroupModel::modifyGroup(Tellico::Data::EntryGroup* group_) {
  Q_ASSERT(group_);
  const int idx = m_groups.indexOf(group_);
  if(idx < 0) {
    myWarning() << "no group named" << group_->groupName();
    return QModelIndex();
  }

  QModelIndex groupIndex = index(idx, 0);
  Node* groupNode = m_rootNode->child(idx);
  const int oldCount = groupNode->childCount();

  beginRemoveRows(groupIndex, 0, groupNode->childCount() - 1);
  groupNode->removeAll();
  endRemoveRows();

  beginInsertRows(groupIndex, 0, group_->count() - 1);
  for(int i = 0; i < group_->count(); ++i) {
    Node* childNode = new Node(groupNode);
    groupNode->addChild(childNode);
  }
  endInsertRows();

  // the only data that might have changed is the count
  if(oldCount != groupNode->childCount()) {
    Q_EMIT dataChanged(groupIndex, groupIndex);
  }
  return groupIndex;
}

void EntryGroupModel::removeGroup(Tellico::Data::EntryGroup* group_) {
  Q_ASSERT(group_);
  int idx = m_groups.indexOf(group_);
  if(idx < 0) {
    myWarning() << "no group named" << group_->groupName();
    return;
  }

  beginRemoveRows(QModelIndex(), idx, idx);
  m_groups.removeAt(idx);
  m_rootNode->removeChild(idx);
  m_groupIconNames.removeAt(idx);
  endRemoveRows();
}

Tellico::Data::EntryGroup* EntryGroupModel::group(const QModelIndex& index_) const {
  // if the parent isn't invalid, then it's not a top-level group
  if(!index_.isValid() || hasValidParent(index_) || index_.row() >= m_groups.count()) {
    return nullptr;
  }
  return m_groups.at(index_.row());
}

Tellico::Data::EntryPtr EntryGroupModel::entry(const QModelIndex& index_) const {
  // if there's not a parent, then it's a top-level item, no entry
  if(!hasValidParent(index_)) {
    return Tellico::Data::EntryPtr();
  }
  Tellico::Data::EntryPtr entry;
  Tellico::Data::EntryGroup* group = this->group(index_.parent());
  // it's possible to have an inconsistent model where the number of rows != number of entries in group
  // like when an entry is removed. modeltest catches this
  if(group && index_.row() < group->count()) {
    entry = group->at(index_.row());
  }
  return entry;
}

QModelIndex EntryGroupModel::indexFromGroup(Tellico::Data::EntryGroup* group_) const {
  if(!group_) {
    return QModelIndex();
  }
  int idx = m_groups.indexOf(group_);
  return idx < 0 ? QModelIndex() : index(idx, 0);
}

// quick parent check for when we don't actually need to know the parent
// just that the parent is valid
// since calling parent() does the node indexOf(), which turns out to be slightly expensive
bool EntryGroupModel::hasValidParent(const QModelIndex& index_) const {
  if(!index_.isValid()) {
    return false;
  }

  Node* node = static_cast<Node*>(index_.internalPointer());
  Q_ASSERT(node);
  Node* parentNode = node->parent();
  Q_ASSERT(parentNode);

  // if it's top-level, it has no parent
  return parentNode && parentNode != m_rootNode;
}
