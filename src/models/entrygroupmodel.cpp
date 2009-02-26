/***************************************************************************
    copyright            : (C) 2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "entrygroupmodel.h"
#include "models.h"
#include "../entry.h"
#include "../collection.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kicon.h>

using Tellico::EntryGroupModel;

class EntryGroupModel::Node {
public:
  Node(Node* parent_) : m_parent(parent_) { }
  ~Node() { qDeleteAll(m_children); }

  Node* parent() const { return m_parent; }
  Node* child(int row) const { return row < m_children.count() ? m_children.at(row) : 0; }
  int row() const { return m_parent ? m_parent->m_children.indexOf(const_cast<Node*>(this)) : -1; }
  int childCount() const { return m_children.count(); };

  void addChild(Node* child) { m_children.append(child); }
  void replaceChild(int i, Node* child) { m_children.replace(i, child); }
  void removeChild(int i) { delete m_children.takeAt(i); }
  void removeAll() { qDeleteAll(m_children); m_children.clear(); }

private:
  Node* m_parent;
  QList<Node*> m_children;
};

EntryGroupModel::EntryGroupModel(QObject* parent) : QAbstractItemModel(parent), m_rootNode(new Node(0)) {
  m_groupHeader = i18nc("Group Name Header", "Group");
}

EntryGroupModel::~EntryGroupModel() {
  delete m_rootNode;
  m_rootNode = 0;
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
  emit headerDataChanged(orientation_, section_, section_);
  return true;
}

QVariant EntryGroupModel::data(const QModelIndex& index_, int role_) const {
  if(!index_.isValid()) {
    return QVariant();
  }

  QModelIndex parent = index_.parent();

  if(index_.row() >= rowCount(parent)) {
    return QVariant();
  }

  switch(role_) {
    case Qt::DisplayRole:
      if(parent.isValid()) {
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
      return parent.isValid() ? KIcon(entry(index_)->collection()->typeName())
                              : KIcon(m_groupIconNames.at(index_.row()));
    case RowCountRole:
      return rowCount(index_);
    case EntryPtrRole:
      return qVariantFromValue(entry(index_));
    case GroupPtrRole:
      return QVariant::fromValue<void*>(group(index_));
  }

  return QVariant();
}

bool EntryGroupModel::setData(const QModelIndex& index_, const QVariant& value_, int role_) {
  // if the index has a parent, then it's not a group
  if(!index_.isValid() || index_.parent().isValid() || index_.row() >= rowCount() || role_ != Qt::DecorationRole) {
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
  m_groups.clear();
  delete m_rootNode;
  m_rootNode = new Node(0);
  m_groupIconNames.clear();
  reset();
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
  int idx = m_groups.indexOf(group_);
  if(idx < 0) {
    myWarning() << "no group named" << group_->groupName();
    return QModelIndex();
  }

  QModelIndex groupIndex = index(idx, 0);
  Node* groupNode = m_rootNode->child(idx);

  beginRemoveRows(groupIndex, 0, groupNode->childCount() - 1);
  groupNode->removeAll();
  endRemoveRows();

  beginInsertRows(groupIndex, 0, group_->count() - 1);
  for(int i = 0; i < group_->count(); ++i) {
    Node* childNode = new Node(groupNode);
    groupNode->addChild(childNode);
  }
  endInsertRows();

  emit dataChanged(groupIndex, groupIndex);
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
  if(!index_.isValid() || index_.parent().isValid() || index_.row() >= m_groups.count()) {
    return 0;
  }
  return m_groups.at(index_.row());
}

Tellico::Data::EntryPtr EntryGroupModel::entry(const QModelIndex& index_) const {
  // if there's not a parent, then it's a top-level item, no entry
  if(!index_.parent().isValid()) {
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

#include "entrygroupmodel.moc"
