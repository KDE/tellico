/***************************************************************************
    Copyright (C) 2017 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_ENTRYICONMODEL_H
#define TELLICO_ENTRYICONMODEL_H

#include "../datavectors.h"

#include <QIdentityProxyModel>
#include <QHash>
#include <QCache>

namespace Tellico {

/**
 * @author Robby Stephenson
 *
 * This identity model does nothing except modify EntryModel::data() to return an entry's icon
 * for every column. It's intended to be used in EntryIconView.
 */
class EntryIconModel : public QIdentityProxyModel {
Q_OBJECT

public:
  EntryIconModel(QObject* parent);
  virtual ~EntryIconModel();

  void setSourceModel(QAbstractItemModel* newSourceModel) Q_DECL_OVERRIDE;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

public Q_SLOTS:
  void clearCache();

private:
  const QIcon& defaultIcon(Data::CollPtr coll) const;

  mutable QHash<int, QIcon*> m_defaultIcons;
  mutable QCache<QString, QIcon> m_iconCache;
};

} // end namespace
#endif
