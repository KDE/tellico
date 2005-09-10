/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICOCONTROLLER_H
#define TELLICOCONTROLLER_H

namespace Tellico {
  class MainWindow;
  class GroupView;
  class GroupIterator;
  namespace Data {
    class Collection;
  }
  namespace GUI {
    class WidgetUpdateBlocker;
  }
}

#include "observer.h"
#include "entry.h"
#include "datavectors.h"

#include <qobject.h>

namespace Tellico {

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
  static void init(MainWindow* parent, const char* name=0) {
    if(!s_self) s_self = new Controller(parent, name);
  }

  const Data::EntryVec& selectedEntries() const { return m_selectedEntries; }
  Data::EntryVec visibleEntries();

  void editEntry(const Data::Entry& entry) const;
  void hideTabs() const;
  /**
   * Plug the default collection actions into a widget
   */
  void plugCollectionActions(QWidget* widget);
  /**
   * Plug the default entry actions into a widget
   */
  void plugEntryActions(QWidget* widget);

  GroupIterator groupIterator() const;
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

  void addedField(Data::Collection* coll, Data::Field* field);
  void modifiedField(Data::Collection* coll, Data::Field* oldField, Data::Field* newField);
  void removedField(Data::Collection* coll, Data::Field* field);

  void addedEntries(Data::EntryVec entries);
  void modifiedEntry(Data::Entry* entry);
  void removedEntries(Data::EntryVec entries);

  void addedBorrower(Data::Borrower* borrower);
  void modifiedBorrower(Data::Borrower* borrower);

  void addedFilter(Filter* filter);
  void removedFilter(Filter* filter);

  void reorderedFields(Data::Collection* coll);

public slots:
  /**
   * When a collection is added to the document, certain actions need to be taken
   * by the parent app. The collection toolbar is updated, the entry count is set, and
   * the collection's modified signal is connected to the @ref GroupView widget.
   *
   * @param coll A pointer to the collection being added
   */
  void slotCollectionAdded(Tellico::Data::Collection* coll);
  void slotCollectionModified(Tellico::Data::Collection* coll);
  /**
   * Removes a collection from all the widgets
   *
   * @param coll A pointer to the collection being added
   */
  void slotCollectionDeleted(Tellico::Data::Collection* coll);
  void slotRefreshField(Tellico::Data::Field* field);

  void slotClearSelection();
  /**
   * Updates the widgets when entries are selected.
   *
   * param widget A pointer to the widget where the entries were selected
   * @param list The list of selected entries
   */
  void slotUpdateSelection(QWidget* widget, const Tellico::Data::EntryVec& entries);
  void slotUpdateCurrent(const Tellico::Data::EntryVec& entries);
  void slotDeleteSelectedEntries();
  void slotCopySelectedEntries();
  void slotUpdateFilter(Tellico::Filter* filter);
  void slotCheckOut();
  void slotCheckIn();
  void slotCheckIn(const Data::EntryVec& entries);

private:
  static Controller* s_self;
  Controller(MainWindow* parent, const char* name);

  void blockAllSignals(bool block) const;
  void blockAllUpdates(bool block);
  void updateActions() const;
  bool canCheckIn() const;

  MainWindow* m_mainWindow;

  bool m_working;

  typedef PtrVector<Tellico::Observer> ObserverVec;
  ObserverVec m_observers;

  QPtrList<GUI::WidgetUpdateBlocker> m_widgetBlocks;

  /**
   * Keep track of the selected entries so that a top-level delete has something for reference
   */
  Data::EntryVec m_selectedEntries;
  Data::EntryVec m_currentEntries;
};

} // end namespace
#endif
