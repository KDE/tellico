/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BOOKCASECONTROLLER_H
#define BOOKCASECONTROLLER_H

#include "entry.h" // needed for EntryList

#include <qobject.h>

namespace Bookcase {
  class MainWindow;
  class EntryView;
  class DetailedListView;
  class GroupView;
  class EntryEditDialog;
  class Filter;
  namespace Data {
    class Collection;
    class Field;
  }

/**
 * @author Robby Stephenson
 * @version $Id: controller.h 573 2004-03-25 02:08:30Z robby $
 */
class Controller : public QObject {
Q_OBJECT

public:
  Controller(MainWindow* parent, const char* name);

  void setWidgets(GroupView* groupView, DetailedListView* detailedView,
                  EntryEditDialog* editDialog, EntryView* entryView);
  const Data::EntryList& selectedEntries() const { return m_selectedEntries; }

public slots:
  /**
   * When a collection is added to the document, certain actions need to be taken
   * by the parent app. The collection toolbar is updated, the unit count is set, and
   * the collection's modified signal is connected to the @ref GroupView widget.
   *
   * @param coll A pointer to the collection being added
   */
  void slotCollectionAdded(Bookcase::Data::Collection* coll);
  /**
   * Removes a collection from all the widgets
   *
   * @param coll A pointer to the collection being added
   */
  void slotCollectionDeleted(Bookcase::Data::Collection* coll);
  void slotCollectionRenamed(const QString& name);
  void slotEntryAdded(Bookcase::Data::Entry* unit);
  void slotEntryModified(Bookcase::Data::Entry* unit);
  void slotEntryDeleted(Bookcase::Data::Entry* unit);
  void slotFieldAdded(Bookcase::Data::Collection* coll, Bookcase::Data::Field* field);
  void slotFieldDeleted(Bookcase::Data::Collection* coll, Bookcase::Data::Field* field);
  void slotFieldModified(Bookcase::Data::Collection* coll, Bookcase::Data::Field* oldField, Bookcase::Data::Field* newField);
  void slotFieldsReordered(Bookcase::Data::Collection* coll);
  void slotRefreshField(Bookcase::Data::Field* field);
  /**
   * Updates the widgets when an entry is selected.
   *
   * param widget A pointer to the widget where the units were selected
   * @param list The list of selected entries
   */
  void slotUpdateSelection(QWidget* widget, const Bookcase::Data::EntryList& list=Bookcase::Data::EntryList());
  void slotUpdateSelection(Bookcase::Data::Entry* entry, const QString& highlight);
  void slotDeleteSelectedEntries();
  void slotCopySelectedEntries();
  void slotUpdateFilter(Bookcase::Filter* filter);

private:
  void blockAllSignals(bool block);

  MainWindow* m_mainWindow;
  GroupView* m_groupView;
  DetailedListView* m_detailedView;
  EntryEditDialog* m_editDialog;
  EntryView* m_entryView;

  /**
   * Keep track of the selected entries so that a top-level delete has something for reference
   */
  Data::EntryList m_selectedEntries;
};

} // end namespace
#endif
