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

#ifndef TELLICO_ENTRYSORTMODEL_H
#define TELLICO_ENTRYSORTMODEL_H

#include "abstractsortmodel.h"
#include "../datavectors.h"
#include "../filter.h"

#include <QHash>

namespace Tellico {

class FieldComparison;

/**
 * @author Robby Stephenson
 */
class EntrySortModel : public AbstractSortModel {
Q_OBJECT

public:
  EntrySortModel(QObject* parent);

  void setFilter(FilterPtr filter);
  FilterPtr filter() const;

protected:
  virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
  virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private Q_SLOTS:
  void clearData();

private:
  FieldComparison* getComparison(const QModelIndex& index) const;

  FilterPtr m_filter;
  mutable QHash<int, FieldComparison*> m_comparisons;
};

} // end namespace
#endif
