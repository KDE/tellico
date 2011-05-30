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

#include "../datavectors.h"
#include "models.h"

#include <KIcon>

#include <QAbstractItemModel>
#include <QHash>
#include <QCache>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class EntryModel : public QAbstractItemModel {
Q_OBJECT

public:
  EntryModel(QObject* parent);
  virtual ~EntryModel();

  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex& index) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

  virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

  void clear();
  void clearSaveState();
  void reset();
  void setImagesAreAvailable(bool b);

  void    setEntries(const Data::EntryList& entries);
  void    addEntries(const Data::EntryList& entries);
  void modifyEntries(const Data::EntryList& entries);
  void removeEntries(const Data::EntryList& entries);

  void    setFields(const Data::FieldList& fields);
  void    addFields(const Data::FieldList& fields);
  void modifyField(Data::FieldPtr oldField, Data::FieldPtr newField);
  void removeFields(const Data::FieldList& fields);

  QModelIndex indexFromEntry(Data::EntryPtr entry) const;

private:
  Data::EntryPtr entry(const QModelIndex& index) const;
  Data::FieldPtr field(const QModelIndex& index) const;
  const KIcon& defaultIcon(Data::CollPtr coll) const;
  QString imageField(Data::CollPtr coll) const;

  Data::EntryList m_entries;
  Data::FieldList m_fields;
  KIcon m_checkPix;
  QHash<int, int> m_saveStates;
  bool m_imagesAreAvailable;

  mutable QHash<int, KIcon*> m_defaultIcons;
  mutable QHash<long, QString> m_imageFields;
  mutable QCache<QString, KIcon> m_iconCache;
};

} // end namespace
#endif
