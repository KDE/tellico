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

#ifndef TELLICO_ABSTRACTSORTMODEL_H
#define TELLICO_ABSTRACTSORTMODEL_H

#include <QSortFilterProxyModel>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class AbstractSortModel : public QSortFilterProxyModel {
Q_OBJECT

public:
  AbstractSortModel(QObject* parent);
  virtual ~AbstractSortModel();

  int sortColumn() const;
  int secondarySortColumn() const;
  int tertiarySortColumn() const;
  void setSortColumn(int col);
  void setSecondarySortColumn(int col);
  void setTertiarySortColumn(int col);

  Qt::SortOrder sortOrder() const;
  void setSortOrder(Qt::SortOrder order);

  virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

private:
  int m_sortColumn;
  int m_secondarySortColumn;
  int m_tertiarySortColumn;
  Qt::SortOrder m_sortOrder;
};

} // end namespace
#endif
