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

#ifndef TELLICO_ENTRYTITLEMODEL_H
#define TELLICO_ENTRYTITLEMODEL_H

#include "abstractentrymodel.h"

#include <QHash>

class KIcon;

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class EntryTitleModel : public AbstractEntryModel {
Q_OBJECT

public:
  EntryTitleModel(QObject* parent);
  virtual ~EntryTitleModel();

  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

  void clear();

private:
  const KIcon& defaultIcon(Data::CollPtr coll) const;
  QString imageField(Data::CollPtr coll) const;

  mutable QHash<int, KIcon*> m_defaultIcons;
  mutable QHash<int, QString> m_imageFields;
};

} // end namespace
#endif
