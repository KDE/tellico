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

#include "abstractsortmodel.h"

using Tellico::AbstractSortModel;

AbstractSortModel::AbstractSortModel(QObject* parent) : QSortFilterProxyModel(parent)
    , m_sortColumn(-1), m_secondarySortColumn(-1), m_tertiarySortColumn(-1) {
}

AbstractSortModel::~AbstractSortModel() {
}

int AbstractSortModel::sortColumn() const {
  return m_sortColumn;
}

int AbstractSortModel::secondarySortColumn() const {
  return m_secondarySortColumn;
}

int AbstractSortModel::tertiarySortColumn() const {
  return m_tertiarySortColumn;
}

void AbstractSortModel::setSortColumn(int col)
{
  m_sortColumn = col;
}

void AbstractSortModel::setSecondarySortColumn(int col)
{
  m_secondarySortColumn = col;
}

void AbstractSortModel::setTertiarySortColumn(int col)
{
  m_tertiarySortColumn = col;
}

void AbstractSortModel::sort(int col_, Qt::SortOrder order_) {
  if(col_ != m_sortColumn) {
    m_tertiarySortColumn = m_secondarySortColumn;
    m_secondarySortColumn = m_sortColumn;
    m_sortColumn = col_;
  }
  QSortFilterProxyModel::sort(col_, order_);
}

#include "abstractsortmodel.moc"
