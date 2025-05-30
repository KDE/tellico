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

#ifndef TELLICO_FILTERMODEL_H
#define TELLICO_FILTERMODEL_H

#include "../datavectors.h"

#include <QAbstractItemModel>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class FilterModel : public QAbstractItemModel {
Q_OBJECT

public:
  FilterModel(QObject* parent);
  virtual ~FilterModel();

  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role=Qt::EditRole) override;
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  virtual QModelIndex index(int row, int column=0, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;

  void clear();
  void addFilters(const FilterList& filters);
  QModelIndex addFilter(FilterPtr filter);
  void removeFilter(FilterPtr filter);

  FilterPtr filter(const QModelIndex& index) const;
  Data::EntryPtr entry(const QModelIndex& index) const;
  void invalidate(const QModelIndex& index);
  bool indexContainsEntry(const QModelIndex& parent, Data::EntryPtr entry) const;

private:
  class Node;
  Tellico::Data::EntryList filteredEntries(const FilterPtr filter) const;
  void populateFilterNode(Node* node, const Data::EntryList& entries) const;

  FilterList m_filters;
  QString m_header;
  Node* m_rootNode;
  bool m_beingInvalidated;
};

} // end namespace
#endif
