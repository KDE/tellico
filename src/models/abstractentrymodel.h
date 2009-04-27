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

#ifndef TELLICO_ABSTRACTENTRYMODEL_H
#define TELLICO_ABSTRACTENTRYMODEL_H

#include <QAbstractListModel>

#include "models.h"
#include "../datavectors.h"

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class AbstractEntryModel : public QAbstractItemModel {
Q_OBJECT

public:
  AbstractEntryModel(QObject* parent);
  virtual ~AbstractEntryModel();

  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex& index) const;

  virtual void clear();
  void reset();

  void    setEntries(const Data::EntryList& entries);
  void    addEntries(const Data::EntryList& entries);
  void modifyEntries(const Data::EntryList& entries);
  void removeEntries(const Data::EntryList& entries);

protected:
  Data::EntryPtr entry(const QModelIndex& index) const;

private:
  Data::EntryList m_entries;
};

} // end namespace
#endif
