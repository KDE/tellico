/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#include "entryselectionmodel.h"
#include "models.h"
#include "../tellico_debug.h"

#include <QSet>

using Tellico::EntrySelectionModel;

EntrySelectionModel::EntrySelectionModel(QAbstractItemModel* targetModel_,
                                         QItemSelectionModel* selModel_,
                                         QObject* parent_)
    : KLinkItemSelectionModel(targetModel_, selModel_, parent_)
    , m_processing(false) {
  addSelectionProxy(selModel_);
  // when the entry model is reset, the selection signal is not triggered
  // using the modelReset signal does not work
  connect(targetModel_, &QAbstractItemModel::modelAboutToBeReset,
          this, &QItemSelectionModel::clear);
}

void EntrySelectionModel::addSelectionProxy(QItemSelectionModel* selModel_) {
  Q_ASSERT(selModel_);
  m_modelList += selModel_;
  connect(selModel_, &QItemSelectionModel::selectionChanged,
          this, &EntrySelectionModel::selectedEntriesChanged);
}

void EntrySelectionModel::selectedEntriesChanged(const QItemSelection& selected_, const QItemSelection& deselected_) {
  // when clearSelection() is called on the other models, then there's a cascading series of calls to
  // selectedEntriesChanged(). But we only care about the first one
  if(m_processing) {
    return;
  }
  m_processing = true;

  QItemSelectionModel* selectionModel = qobject_cast<QItemSelectionModel*>(sender());
  Q_ASSERT(selectionModel);
  if(!selectionModel) {
    return;
  }

  if(m_recentSelectionModel != selectionModel) {
    m_selectedEntries.clear();
  }
  m_recentSelectionModel = selectionModel;

  // clearing the selection in the other models will have cascading calls to selectionChanged()
  // now, add and remove selected entries from the list
  // the selection will include an index for every column, need to check for duplicates
  // can't use a QSet of entries since we want to retain the selection ordering
  QSet<Data::ID> IDlist;
  foreach(const QModelIndex& index, deselected_.indexes()) {
    Data::EntryPtr entry = index.data(EntryPtrRole).value<Data::EntryPtr>();
    if(entry && !IDlist.contains(entry->id())) {
      m_selectedEntries.removeOne(entry);
      IDlist += entry->id();
    }
  }
  IDlist.clear();
  foreach(const QModelIndex& index, selected_.indexes()) {
    Data::EntryPtr entry = index.data(EntryPtrRole).value<Data::EntryPtr>();
    if(entry && !IDlist.contains(entry->id())) {
      m_selectedEntries += entry;
      IDlist += entry->id();
    }
  }

  emit entriesSelected(m_selectedEntries);
  // for every selection model which did not call this function, clear the selection
  foreach(const QPointer<QItemSelectionModel>& ptr, m_modelList) { //krazy:exclude=foreach
    QItemSelectionModel* const otherModel = ptr.data();
    if(otherModel && otherModel != selectionModel) {
      otherModel->clearSelection();
    } else if(!otherModel) {
      // since the filter or loan view could be created multiple times
      // the selection model might be added multiple times
      // since foreach() creates a copy of the list, it's ok to remove this here
      m_modelList.removeOne(ptr);
    }
  }
  m_processing = false;
}
