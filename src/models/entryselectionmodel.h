/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_ENTRYSELECTIONMODEL_H
#define TELLICO_ENTRYSELECTIONMODEL_H

#include "../datavectors.h"

#include <KLinkItemSelectionModel>

#include <QList>
#include <QPointer>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class EntrySelectionModel : public KLinkItemSelectionModel {
Q_OBJECT

public:
  EntrySelectionModel(QAbstractItemModel* targetModel,
                      QItemSelectionModel* selModel,
                      QObject* parent=nullptr);

  void addSelectionProxy(QItemSelectionModel* selModel);
  Tellico::Data::EntryList selectedEntries() const { return m_selectedEntries; }

public Q_SLOTS:
  void clear() override;

Q_SIGNALS:
  void entriesSelected(Tellico::Data::EntryList entries);

private Q_SLOTS:
  void selectedEntriesChanged(const QItemSelection& selected, const QItemSelection& deselected);

private:
  Data::EntryList m_selectedEntries;
  QList< QPointer<QItemSelectionModel> > m_modelList;
  bool m_processing;
};

} // end namespace
#endif
