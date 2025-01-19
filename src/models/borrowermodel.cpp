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

#include "borrowermodel.h"
#include "models.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

#include <QIcon>

using Tellico::BorrowerModel;

class BorrowerModel::Node {
public:
  Node(Node* parent_) : m_parent(parent_) {}
  ~Node() { qDeleteAll(m_children); }

  Node* parent() const { return m_parent; }
  Node* child(int row) const { return m_children.at(row); }
  int row() const { return m_parent ? m_parent->m_children.indexOf(const_cast<Node*>(this)) : 0; }
  int childCount() const { return m_children.count(); }

  void addChild(Node* child) {  m_children.append(child); }
  void replaceChild(int i, Node* child) {  m_children.replace(i, child); }
  void removeChild(int i) {  delete m_children.takeAt(i); }
  void removeAll() { qDeleteAll(m_children); m_children.clear(); }

private:
  Node* m_parent;
  QList<Node*> m_children;
};

BorrowerModel::BorrowerModel(QObject* parent) : QAbstractItemModel(parent), m_rootNode(new Node(nullptr)) {
}

BorrowerModel::~BorrowerModel() {
  delete m_rootNode;
  m_rootNode = nullptr;
}

int BorrowerModel::rowCount(const QModelIndex& index_) const {
  if(!index_.isValid()) {
    return m_borrowers.count();
  }
  QModelIndex parent = index_.parent();
  if(parent.isValid()) {
    return 0; // a parent index means it points to an entry, not a filter, so there are no children
  }
  Node* node = static_cast<Node*>(index_.internalPointer());
  Q_ASSERT(node);
  return node->childCount();
}

int BorrowerModel::columnCount(const QModelIndex&) const {
  return 1;
}

QVariant BorrowerModel::headerData(int section_, Qt::Orientation orientation_, int role_) const {
  if(section_ < 0 || section_ >= columnCount() || orientation_ != Qt::Horizontal) {
    return QVariant();
  }
  if(role_ == Qt::DisplayRole) {
    return m_header;
  }
  return QVariant();
}

bool BorrowerModel::setHeaderData(int section_, Qt::Orientation orientation_,
                                  const QVariant& value_, int role_) {
  if(section_ < 0 || section_ >= columnCount() || orientation_ != Qt::Horizontal || role_ != Qt::EditRole) {
    return false;
  }
  m_header = value_.toString();
  Q_EMIT headerDataChanged(orientation_, section_, section_);
  return true;
}

QVariant BorrowerModel::data(const QModelIndex& index_, int role_) const {
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
        // it points to an entry
        return entry(index_)->title();
      }
      // it points to a borrower
      return borrower(index_)->name();
    case Qt::DecorationRole:
      return parent.isValid() ? QIcon(QLatin1String(":/icons/") + CollectionFactory::typeName(entry(index_)->collection()))
                              : QIcon::fromTheme(QLatin1String("kaddressbook"));
    case RowCountRole:
      return rowCount(index_);
    case EntryPtrRole:
      return QVariant::fromValue(entry(index_));
  }

  return QVariant();
}

QModelIndex BorrowerModel::index(int row_, int column_, const QModelIndex& parent_) const {
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
  if(!child) {
    return QModelIndex();
  }
  return createIndex(row_, column_, child);
}

QModelIndex BorrowerModel::parent(const QModelIndex& index_) const {
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

void BorrowerModel::clear() {
  beginResetModel();
  m_borrowers.clear();
  delete m_rootNode;
  m_rootNode = new Node(nullptr);
  endResetModel();
}

void BorrowerModel::addBorrowers(const Tellico::Data::BorrowerList& borrowers_) {
  beginInsertRows(QModelIndex(), rowCount(), rowCount()+borrowers_.count()-1);
  m_borrowers += borrowers_;
  foreach(Data::BorrowerPtr borrower, borrowers_) {
    Node* borrowerNode = new Node(m_rootNode);
    m_rootNode->addChild(borrowerNode);
    for(int i = 0; i < borrower->count(); ++i) {
      Node* childNode = new Node(borrowerNode);
      borrowerNode->addChild(childNode);
    }
  }
  endInsertRows();
}

QModelIndex BorrowerModel::addBorrower(Tellico::Data::BorrowerPtr borrower_) {
  Q_ASSERT(borrower_);
  addBorrowers(Data::BorrowerList() << borrower_);
  // rowCount() has increased now
  return index(rowCount()-1, 0);
}

QModelIndex BorrowerModel::modifyBorrower(Tellico::Data::BorrowerPtr borrower_) {
  Q_ASSERT(borrower_);
  Q_ASSERT(!borrower_->isEmpty());
  int idx = m_borrowers.indexOf(borrower_);
  if(idx < 0) {
    myWarning() << "no borrower named" << borrower_->name();
    return QModelIndex();
  }

  QModelIndex borrowerIndex = index(idx, 0);
  Node* borrowerNode = m_rootNode->child(idx);

  beginRemoveRows(borrowerIndex, 0, borrowerNode->childCount() - 1);
  borrowerNode->removeAll();
  endRemoveRows();

  beginInsertRows(borrowerIndex, 0, borrower_->count() - 1);
  for(int i = 0; i < borrower_->count(); ++i) {
    Node* childNode = new Node(borrowerNode);
    borrowerNode->addChild(childNode);
  }
  endInsertRows();

  Q_EMIT dataChanged(borrowerIndex, borrowerIndex);
  return borrowerIndex;
}

void BorrowerModel::removeBorrower(Tellico::Data::BorrowerPtr borrower_) {
  Q_ASSERT(borrower_);
  int idx = m_borrowers.indexOf(borrower_);
  if(idx < 0) {
    myWarning() << "no borrower named" << borrower_->name();
    return;
  }

  beginRemoveRows(QModelIndex(), idx, idx);
  m_borrowers.removeAt(idx);
  m_rootNode->removeChild(idx);
  endRemoveRows();
}

Tellico::Data::BorrowerPtr BorrowerModel::borrower(const QModelIndex& index_) const {
  // if the parent isn't invalid, then it's not a top-level borrower
  if(!index_.isValid() || index_.parent().isValid() || index_.row() >= m_borrowers.count()) {
    return Data::BorrowerPtr();
  }
  return m_borrowers.at(index_.row());
}

Tellico::Data::EntryPtr BorrowerModel::entry(const QModelIndex& index_) const {
  // if there's not a parent, then it's a top-level item, no entry
  if(!index_.parent().isValid()) {
    return Data::EntryPtr();
  }
  Data::EntryPtr entry;
  Data::LoanPtr loan = this->loan(index_);
  if(loan) {
    entry = loan->entry();
  }
  return entry;
}

Tellico::Data::LoanPtr BorrowerModel::loan(const QModelIndex& index_) const {
  // if there's not a parent, then it's a top-level item, no entry
  if(!index_.parent().isValid()) {
    return Data::LoanPtr();
  }
  Data::LoanPtr loan;
  Data::BorrowerPtr borrower = this->borrower(index_.parent());
  // could have already removed the loan from the borrower
  if(borrower && index_.row() < borrower->loans().size()) {
    loan = borrower->loans().at(index_.row());
  }
  return loan;
}
