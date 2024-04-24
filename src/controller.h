/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_CONTROLLER_H
#define TELLICO_CONTROLLER_H

#include "entry.h"

#include <QObject>
#include <QList>

class QMenu;

namespace Tellico {
  class MainWindow;
  namespace Data {
    class Collection;
  }
  class Observer;

/**
 * @author Robby Stephenson
 */
class Controller : public QObject {
Q_OBJECT

public:
  static Controller* self() { return s_self; }
  /**
   * Initializes the singleton. Should just be called once, from Tellico::MainWindow
   */
  static void init(MainWindow* parent) {
    if(!s_self) s_self = new Controller(parent);
  }

  const Data::EntryList& selectedEntries() const { return m_selectedEntries; }
  Data::EntryList visibleEntries();

  void editEntry(Data::EntryPtr) const;
  void hideTabs() const;
  /**
   * Plug the default collection actions into a widget
   */
  void plugCollectionActions(QMenu* popup);
  /**
   * Plug the default entry actions into a widget
   */
  void plugEntryActions(QMenu* popup);
  QMenu* plugSortActions(QMenu* popup);
  void updateActions() const;

  /**
   * Returns the name of the field being used to group the entries.
   * That field name may not be an actual field in the collection, since
   * pseudo-groups like _people exist.
   */
  QString groupBy() const;
  /**
   * Returns a list of the fields being used to group the entries.
   * For ordinary fields, the list has a single item, the field name.
   * For the pseudo-group _people, all the people fields are included.
   */
  QStringList expandedGroupBy() const;
  /**
   * Returns a list of the titles of the fields being used to sort the entries in the detailed column view.
   */
  QStringList sortTitles() const;
  /**
   * Returns the title of the fields currently visible in the detailed column view.
   */
  QStringList visibleColumns() const;

  void    addObserver(Observer* obs);
  void removeObserver(Observer* obs);

  void addedField(Data::CollPtr coll, Data::FieldPtr field);
  void modifiedField(Data::CollPtr coll, Data::FieldPtr oldField, Data::FieldPtr newField);
  void removedField(Data::CollPtr coll, Data::FieldPtr field);

  void addedEntries(Data::EntryList entries);
  void modifiedEntries(Data::EntryList entries);
  void removedEntries(Data::EntryList entries);

  void addedBorrower(Data::BorrowerPtr borrower);
  void modifiedBorrower(Data::BorrowerPtr borrower);

  void addedFilter(FilterPtr filter);
  void removedFilter(FilterPtr filter);

  void reorderedFields(Data::CollPtr coll);
  void updatedFetchers();

  void clearFilter();

public Q_SLOTS:
  /**
   * When a collection is added to the document, certain actions need to be taken
   * by the parent app. The collection toolbar is updated, the entry count is set, and
   * the collection's modified signal is connected to the @ref GroupView widget.
   *
   * @param coll A pointer to the collection being added
   */
  void slotCollectionAdded(Tellico::Data::CollPtr coll);
  void slotCollectionModified(Tellico::Data::CollPtr coll, bool structuralChange);
  /**
   * Removes a collection from all the widgets
   *
   * @param coll A pointer to the collection being added
   */
  void slotCollectionDeleted(Tellico::Data::CollPtr coll);
  void slotFieldAdded(Tellico::Data::CollPtr coll, Tellico::Data::FieldPtr field);
  void slotRefreshField(Tellico::Data::FieldPtr field);

  void slotClearSelection();
  /**
   * Updates the widgets when entries are selected.
   *
   * @param entries The list of selected entries
   */
  void slotUpdateSelection(const Tellico::Data::EntryList& entries);
  void slotCopySelectedEntries();
  void slotUpdateSelectedEntries(const QString& source);
  void slotDeleteSelectedEntries();
  void slotMergeSelectedEntries();
  void slotUpdateFilter(Tellico::FilterPtr filter);
  void slotCheckOut();
  void slotCheckIn();
  void slotCheckIn(const Tellico::Data::EntryList& entries);

Q_SIGNALS:
  void collectionAdded(int collType);

private:
  static Controller* s_self;
  Controller(MainWindow* parent);
  ~Controller();

  void blockAllSignals(bool block) const;
  bool canCheckIn() const;
  void plugUpdateMenu(QMenu* popup);

  MainWindow* m_mainWindow;

  bool m_working;

  typedef QList<Tellico::Observer*> ObserverList;
  ObserverList m_observers;

  /**
   * Keep track of the selected entries so that a top-level delete has something for reference
   */
  Data::EntryList m_selectedEntries;
};

} // end namespace
#endif
