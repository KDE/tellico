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

  void    setFields(const Data::FieldList& fields);
  void    addFields(const Data::FieldList& fields);
  void modifyFields(const Data::FieldList& fields);
  void removeFields(const Data::FieldList& fields);

private:
  Data::FieldList m_fields;
  KIcon m_checkPix;
  QHash<int, int> m_saveStates;
};

} // end namespace
#endif
