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

#ifndef TELLICO_ENTRYSORTMODEL_H
#define TELLICO_ENTRYSORTMODEL_H

#include "abstractsortmodel.h"
#include "../datavectors.h"
#include "../filter.h"

#include <QHash>

namespace Tellico {

class ListViewComparison;

/**
 * @author Robby Stephenson
 */
class EntrySortModel : public AbstractSortModel {
Q_OBJECT

public:
  EntrySortModel(QObject* parent);
  virtual ~EntrySortModel();

  void setFilter(FilterPtr filter);
  FilterPtr filter() const;

  void clear();

protected:
  virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;
  virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const;

private:
  ListViewComparison* getComparison(const QModelIndex& index) const;

  FilterPtr m_filter;
  mutable QHash<int, ListViewComparison*> m_comparisons;
};

} // end namespace
#endif
