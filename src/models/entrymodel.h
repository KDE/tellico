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

#include <QIcon>
#include <QAbstractItemModel>
#include <QMultiHash>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class EntryModel : public QAbstractItemModel {
Q_OBJECT

public:
  EntryModel(QObject* parent);
  virtual ~EntryModel();

  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

  virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

  void clear();
  void clearSaveState();
  void setImagesAreAvailable(bool b);

  void    setEntries(const Data::EntryList& entries);
  void    addEntries(const Data::EntryList& entries);
  void modifyEntries(const Data::EntryList& entries);
  void removeEntries(const Data::EntryList& entries);

  void     setFields(const Data::FieldList& fields);
  void reorderFields(const Data::FieldList& fields);
  void     addFields(const Data::FieldList& fields);
  void   modifyField(Data::FieldPtr oldField, Data::FieldPtr newField);
  void  removeFields(const Data::FieldList& fields);

  QModelIndex indexFromEntry(Data::EntryPtr entry) const;

private Q_SLOTS:
  void refreshImage(const QString& id);

private:
  Data::EntryPtr entry(const QModelIndex& index) const;
  Data::FieldPtr field(const QModelIndex& index) const;
  QVariant requestImage(Data::EntryPtr entry, const QString& id) const;

  Data::EntryList m_entries;
  Data::FieldList m_fields;
  QIcon m_checkPix;
  QHash<int, int> m_saveStates;
  bool m_imagesAreAvailable;

  // maps ids of requested images into entries
  mutable QMultiHash<QString, Data::EntryPtr> m_requestedImages;
};

} // end namespace
#endif
