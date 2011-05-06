/***************************************************************************
    Copyright (C) 2008-2009 Robby Stephenson <robby@periapsis.org>
    Copyright (C) 2011 Pedro Miguel Carvalho <kde@pmc.com.pt>
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

#ifndef TELLICO_ENTRYTITLEMODEL_H
#define TELLICO_ENTRYTITLEMODEL_H

#include "abstractentrymodel.h"

#include <KIcon>

#include <QHash>
#include <QCache>

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
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

  virtual void clear();

private:
  const KIcon& defaultIcon(Data::CollPtr coll) const;
  QString imageField(Data::CollPtr coll) const;

  mutable QHash<int, KIcon*> m_defaultIcons;
  mutable QHash<long, QString> m_imageFields;
  mutable QCache<QString, KIcon> m_iconCache;
};

} // end namespace
#endif
