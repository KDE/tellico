/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_ENTRYMATCHDIALOG_H
#define TELLICO_ENTRYMATCHDIALOG_H

#include "datavectors.h"
#include "entryupdater.h"
#include "fetch/fetcher.h"

#include <kdialog.h>

#include <QHash>

namespace Tellico {
  class EntryView;
}

class QTreeWidget;
class QTreeWidgetItem;

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class EntryMatchDialog : public KDialog {
Q_OBJECT

public:
  /**
   * Constructor
   */
  EntryMatchDialog(QWidget* parent, Data::EntryPtr entryToUpdate,
                   Fetch::Fetcher::Ptr fetcher, const EntryUpdater::ResultList& matchResults);

  EntryUpdater::UpdateResult updateResult() const;

private slots:
  void slotShowEntry();

private:
  QTreeWidget* m_treeWidget;
  EntryView* m_entryView;

  QHash<QTreeWidgetItem*, EntryUpdater::UpdateResult> m_itemResults;
  QHash<QTreeWidgetItem*, Data::EntryPtr> m_itemEntries;
};

} //end namespace

#endif
