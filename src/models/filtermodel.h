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

  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role=Qt::EditRole);
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  virtual QModelIndex index(int row, int column=0, const QModelIndex& parent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex& index) const;

  void clear();
  void addFilters(const FilterList& filters);
  QModelIndex addFilter(FilterPtr filter);
  void removeFilter(FilterPtr filter);

  FilterPtr filter(const QModelIndex& index) const;
  Data::EntryPtr entry(const QModelIndex& index) const;
  void invalidate(const QModelIndex& index);

private:
  FilterList m_filters;
  QString m_header;
  class Node;
  Node* m_rootNode;
};

} // end namespace
#endif
