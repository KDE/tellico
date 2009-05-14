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

#ifndef TELLICO_ENTRYMODEL_H
#define TELLICO_ENTRYMODEL_H

#include "abstractentrymodel.h"
#include "../datavectors.h"

#include <KIcon>

#include <QHash>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class EntryModel : public AbstractEntryModel {
Q_OBJECT

public:
  EntryModel(QObject* parent);
  virtual ~EntryModel();

  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  Data::FieldPtr field(const QModelIndex& index) const;

  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

  void clear();
  void clearSaveState();
  void setImagesAreAvailable(bool b);

  void    setFields(const Data::FieldList& fields);
  void    addFields(const Data::FieldList& fields);
  void modifyFields(const Data::FieldList& fields);
  void removeFields(const Data::FieldList& fields);

private:
  Data::FieldList m_fields;
  KIcon m_checkPix;
  QHash<int, int> m_saveStates;
  bool m_imagesAreAvailable;
};

} // end namespace
#endif
