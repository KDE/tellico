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

namespace Bookcase {
  class MainWindow;
  class ViewStack;
  class DetailedListView;
  class GroupView;
  class EntryEditDialog;
  class Filter;
  namespace Data {
    class Collection;
    class Field;
  }
}

#include "entry.h" // needed for EntryList

#include <qobject.h>

namespace Bookcase {

/**
 * @author Robby Stephenson
 * @version $Id: controller.h 739 2004-08-05 05:37:17Z robby $
 */
class Controller : public QObject {
Q_OBJECT

public:
  static Controller* self() { return s_self; }
  /**
   * Initializes the singleton. Should just be called once, from Bookcase::MainWindow
   */
  static void init(MainWindow* parent, const char* name=0) { if(!s_self) s_self = new Controller(parent, name); }

  /**
   * Initialize pointers to all the main widgets.
   */
  void initWidgets();
  const Data::EntryList& selectedEntries() const { return m_selectedEntries; }

  void editEntry(const Data::Entry& entry) const;
  /**
   * Plug the default collection actions into a widget
   */
  void plugCollectionActions(QWidget* widget);
  /**
   * Plug the default entry actions into a widget
   */
  void plugEntryActions(QWidget* widget);

public slots:
  /**
   * When a collection is added to the document, certain actions need to be taken
   * by the parent app. The collection toolbar is updated, the entry count is set, and
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
  void slotEntryAdded(Bookcase::Data::Entry* entry);
  void slotEntryModified(Bookcase::Data::Entry* entry);
  void slotEntryDeleted(Bookcase::Data::Entry* entry);
  void slotFieldAdded(Bookcase::Data::Collection* coll, Bookcase::Data::Field* field);
  void slotFieldDeleted(Bookcase::Data::Collection* coll, Bookcase::Data::Field* field);
  void slotFieldModified(Bookcase::Data::Collection* coll, Bookcase::Data::Field* oldField, Bookcase::Data::Field* newField);
  void slotFieldsReordered(Bookcase::Data::Collection* coll);
  void slotRefreshField(Bookcase::Data::Field* field);

  void slotUpdateSelection(QWidget* widget, const Bookcase::Data::Collection* coll);
  void slotUpdateSelection(QWidget* widget, const Bookcase::Data::EntryGroup* group);
  /**
   * Updates the widgets when an entry is selected.
   *
   * param widget A pointer to the widget where the entries were selected
   * @param list The list of selected entries
   */
  void slotUpdateSelection(QWidget* widget, const Bookcase::Data::EntryList& list=Bookcase::Data::EntryList());
  void slotUpdateSelection(Bookcase::Data::Entry* entry, const QString& highlight);
  void slotDeleteSelectedEntries();
  void slotCopySelectedEntries();
  void slotUpdateFilter(Bookcase::Filter* filter);

private:
  static Controller* s_self;
  Controller(MainWindow* parent, const char* name);

  void blockAllSignals(bool block) const;
  void updateActions() const;

  MainWindow* m_mainWindow;
  GroupView* m_groupView;
  DetailedListView* m_detailedView;
  EntryEditDialog* m_editDialog;
  ViewStack* m_viewStack;

  bool m_working;

  /**
   * Keep track of the selected entries so that a top-level delete has something for reference
   */
  Data::EntryList m_selectedEntries;
};

} // end namespace
#endif
