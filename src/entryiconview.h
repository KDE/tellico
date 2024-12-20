/***************************************************************************
    Copyright (C) 2002-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_ENTRYICONVIEW_H
#define TELLICO_ENTRYICONVIEW_H

#include "observer.h"

#include <QListView>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class EntryIconView : public QListView, public Observer {
Q_OBJECT

public:
  EntryIconView(QWidget* parent);
  ~EntryIconView();

  void setModel(QAbstractItemModel* model) override;
  int maxAllowedIconWidth() const { return m_maxAllowedIconWidth; }

public Q_SLOTS:
  void setMaxAllowedIconWidth(int width);

protected:
  void contextMenuEvent(QContextMenuEvent* event) override;

private Q_SLOTS:
  void slotDoubleClicked(const QModelIndex& index);
  void slotSortMenuActivated(QAction* action);
  void slotOpenUrlMenuActivated(QAction* action=nullptr);
  void updateModelColumn();

private:
  int m_maxAllowedIconWidth;
};

} // end namespace
#endif
