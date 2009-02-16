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

  virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

private:
  int m_sortColumn;
  int m_secondarySortColumn;
  int m_tertiarySortColumn;
};

} // end namespace
#endif
