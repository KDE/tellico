/***************************************************************************
    Copyright (C) 2008-2011 Robby Stephenson <robby@periapsis.org>
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

#include "abstractentrymodel.h"
#include "../entry.h"

using Tellico::AbstractEntryModel;

AbstractEntryModel::AbstractEntryModel(QObject* parent) : QAbstractItemModel(parent) {
}

AbstractEntryModel::~AbstractEntryModel() {
}

int AbstractEntryModel::rowCount(const QModelIndex& index_) const {
  // no children for valid indexes
  return index_.isValid() ? 0 : m_entries.count();
}

QModelIndex AbstractEntryModel::index(int row_, int column_, const QModelIndex& parent_) const {
  return hasIndex(row_, column_, parent_) ? createIndex(row_, column_, 0) : QModelIndex();
}

QModelIndex AbstractEntryModel::indexFromEntry(Data::EntryPtr entry_) const {
  const int idx = m_entries.indexOf(entry_);
  if(idx == -1) {
    return QModelIndex();
  }
  return createIndex(idx, 0);
}

QModelIndex AbstractEntryModel::parent(const QModelIndex&) const {
  return QModelIndex();
}

void AbstractEntryModel::clear() {
  m_entries.clear();
  reset();
}

// make it public
void AbstractEntryModel::reset() {
  QAbstractItemModel::reset();
}

void AbstractEntryModel::setEntries(const Tellico::Data::EntryList& entries_) {
  m_entries = entries_;
  reset();
}

void AbstractEntryModel::addEntries(const Tellico::Data::EntryList& entries_) {
  beginInsertRows(QModelIndex(), m_entries.count(), m_entries.count() + entries_.count() - 1);
  m_entries += entries_;
  endInsertRows();
}

void AbstractEntryModel::modifyEntries(const Tellico::Data::EntryList& entries_) {
  foreach(Data::EntryPtr entry, entries_) {
    QModelIndex index = indexFromEntry(entry);
    if(index.isValid()) {
      emit dataChanged(index, index);
    }
  }
}

void AbstractEntryModel::removeEntries(const Tellico::Data::EntryList& entries_) {
  // for performance reasons, if more than 10 entries are being removed, rather than
  // iterating over all of them, which really hurts, just signal a full replacement
  const bool bigRemoval = (entries_.size() > 10);
  if(bigRemoval) {
    beginResetModel();
  }
  foreach(Data::EntryPtr entry, entries_) {
    int idx = m_entries.indexOf(entry);
    if(idx > -1) {
      if(!bigRemoval) {
        beginRemoveRows(QModelIndex(), idx, idx);
      }
      m_entries.removeAt(idx);
      if(!bigRemoval) {
        endRemoveRows();
      }
    }
  }
  if(bigRemoval) {
    endResetModel();
  }
}

Tellico::Data::EntryPtr AbstractEntryModel::entry(const QModelIndex& index_) const {
  Data::EntryPtr entry;
  if(index_.isValid() && index_.row() < m_entries.count()) {
    entry = m_entries.at(index_.row());
  }
  return entry;
}

#include "abstractentrymodel.moc"
