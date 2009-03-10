/***************************************************************************
    copyright            : (C) 2001-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_DETAILEDLISTVIEW_H
#define TELLICO_DETAILEDLISTVIEW_H

#include "gui/treeview.h"
#include "observer.h"
#include "filter.h"

#include <QStringList>
#include <QPixmap>
#include <QEvent>
#include <QVector>

class KMenu;

namespace Tellico {
  class DetailedEntryItem;
  class EntryModel;

/**
 * The DetailedListView class shows detailed information about entries in the
 * collection.
 *
 * @author Robby Stephenson
 */
class DetailedListView : public GUI::TreeView, public Observer {
Q_OBJECT

public:
  /**
   * The constructor initializes the popup menu, but no columns are inserted.
   *
   * @param parent A pointer to the parent widget
   */
  DetailedListView(QWidget* parent);
  virtual ~DetailedListView();

  EntryModel* sourceModel() const;

  /**
   * Event filter used to popup the menu
   */
  bool eventFilter(QObject* obj, QEvent* ev);
  /**
   * Selects the item which refers to a certain entry.
   *
   * @param entry A pointer to the entry
   */
  void setEntriesSelected(Data::EntryList entries);
  void setFilter(FilterPtr filter);
  FilterPtr filter() const;

  QString sortColumnTitle1() const;
  QString sortColumnTitle2() const;
  QString sortColumnTitle3() const;
  QStringList visibleColumns() const;
  Data::EntryList visibleEntries();

  /**
   * @param coll A pointer to the collection
   */
  void addCollection(Data::CollPtr coll);
  /**
   * Removes all items which refers to a entry within a collection.
   *
   * @param coll A pointer to the collection
   */
  void removeCollection(Data::CollPtr coll);

  /**
   * Adds a new list item showing the details for a entry.
   *
   * @param entry A pointer to the entry
   */
  virtual void addEntries(Data::EntryList entries);
  /**
   * Modifies any item which refers to a entry, resetting the column contents.
   *
   * @param entry A pointer to the entry
   */
  virtual void modifyEntries(Data::EntryList entries);
  /**
   * Removes any item which refers to a certain entry.
   *
   * @param entry A pointer to the entry
   */
  virtual void removeEntries(Data::EntryList entries);

  virtual void addField(Data::CollPtr, Data::FieldPtr field);
  void addField(Data::FieldPtr field, int width);
  virtual void modifyField(Data::CollPtr, Data::FieldPtr oldField, Data::FieldPtr newField);
  virtual void removeField(Data::CollPtr, Data::FieldPtr field);

  void reorderFields(const Data::FieldList& fields);
  /**
   * saveConfig is only needed for custom collections */
  void saveConfig(Data::CollPtr coll, int saveConfig);
  /**
   * Select all visible items.
   */
  void selectAllVisible();
  int visibleItems() const;
  /**
   * Set max size of pixmaps.
   *
   * @param width Width
   * @param height Height
   */
  void setPixmapSize(int width, int height) { Q_UNUSED(width) Q_UNUSED(height) }
  void resetEntryStatus();

public slots:
  /**
   * Resets the list view, clearing and deleting all items.
   */
  void slotReset();
  /**
   * Refreshes the view, repopulating all items.
   */
  void slotRefresh();

private slots:
  void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
  void slotDoubleClicked(const QModelIndex& index);
  void slotHeaderMenuActivated(QAction* action);
//  void slotCacheColumnWidth(int section, int oldSize, int newSize);

private:
  void contextMenuEvent(QContextMenuEvent* event);
  void updateHeaderMenu();
  void setState(Tellico::Data::EntryList entries_, int state);

  struct ConfigInfo {
    QStringList cols;
    QList<int> widths;
    QList<int> order;
    int colSorted;
    bool ascSort : 1;
    int prevSort;
    int prev2Sort;
    QString state;
  };

  KMenu* m_headerMenu;
  bool m_loadingCollection;
};

} // end namespace;
#endif
