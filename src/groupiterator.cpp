/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "groupiterator.h"
#include "models/models.h"

#include <QAbstractItemModel>

using Tellico::GroupIterator;

GroupIterator::GroupIterator(QAbstractItemModel* model_) : m_model(model_), m_row(0) {
}

GroupIterator& GroupIterator::operator++() {
  ++m_row;
  return *this;
}

Tellico::Data::EntryGroup* GroupIterator::group() {
  if(m_row >= m_model->rowCount()) {
    return 0;
  }

  QVariant v = m_model->data(m_model->index(m_row, 0), GroupPtrRole);
  return reinterpret_cast<Data::EntryGroup*>(v.value<void*>());
}
