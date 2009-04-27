/***************************************************************************
    Copyright (C) 2002-2009 Robby Stephenson <robby@periapsis.org>
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

class KIcon;

namespace Tellico {

class AbstractEntryModel;
class EntrySortModel;

/**
 * @author Robby Stephenson
 */
class EntryIconView : public QListView, public Observer {
Q_OBJECT

public:
  EntryIconView(QWidget* parent);
  ~EntryIconView();

  EntrySortModel* sortModel() const;
  AbstractEntryModel* sourceModel() const;

  void clear();
  void refresh();
  void showEntries(const Data::EntryList& entries);
  /**
   * Adds a new list item showing the details for a entry.
   *
   * @param entry A pointer to the entry
   */
  virtual void    addEntries(Data::EntryList entries);
  virtual void modifyEntries(Data::EntryList entries);
  virtual void removeEntries(Data::EntryList entries);

  void setMaxAllowedIconWidth(int width);
  int maxAllowedIconWidth() const { return m_maxAllowedIconWidth; }

protected:
  void contextMenuEvent(QContextMenuEvent* event);

private slots:
  void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
  void slotDoubleClicked(const QModelIndex& index);
  void slotSortMenuActivated(QAction* action);

private:
  int m_maxAllowedIconWidth;
};

} // end namespace
#endif
