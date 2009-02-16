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
  m_entries += entries_;
  reset();
}

void AbstractEntryModel::modifyEntries(const Tellico::Data::EntryList&) {
  reset();
}

void AbstractEntryModel::removeEntries(const Tellico::Data::EntryList& entries_) {
  foreach(Data::EntryPtr entry, entries_) {
    m_entries.removeOne(entry);
  }
  reset();
}

Tellico::Data::EntryPtr AbstractEntryModel::entry(const QModelIndex& index_) const {
  Data::EntryPtr entry;
  if(index_.isValid() && index_.row() < m_entries.count()) {
    entry = m_entries.at(index_.row());
  }
  return entry;
}

#include "abstractentrymodel.moc"
