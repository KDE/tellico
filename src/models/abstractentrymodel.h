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
