/***************************************************************************
    copyright            : (C) 2001-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef DETAILEDLISTVIEW_H
#define DETAILEDLISTVIEW_H

#include "gui/listview.h"
#include "observer.h"
#include "filter.h"

#include <qstringlist.h>
#include <qpixmap.h>
#include <qvaluevector.h>

class KPopupMenu;

namespace Tellico {
  class DetailedEntryItem;

/**
 * The DetailedListView class shows detailed information about entries in the
 * collection.
 *
 * @author Robby Stephenson
 */
class DetailedListView : public GUI::ListView, public Observer {
Q_OBJECT

public:
  /**
   * The constructor initializes the popup menu, but no columns are inserted.
   *
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  DetailedListView(QWidget* parent, const char* name=0);
  ~DetailedListView();

  /**
   * Event filter used to popup the menu
   */
  bool eventFilter(QObject* obj, QEvent* ev);
  /**
   * Selects the item which refers to a certain entry.
   *
   * @param entry A pointer to the entry
   */
  void setEntrySelected(Data::EntryPtr entry);
  void setFilter(FilterPtr filter);
  FilterPtr filter() { return m_filter; }

  int prevSortedColumn() const;
  int prev2SortedColumn() const;
  virtual void setSorting(int column, bool ascending = true);
  void setPrevSortedColumn(int prev1, int prev2);
  QString sortColumnTitle1() const;
  QString sortColumnTitle2() const;
  QString sortColumnTitle3() const;
  QStringList visibleColumns() const;
  Data::EntryVec visibleEntries();

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
  virtual void addEntries(Data::EntryVec entries);
  /**
   * Modifies any item which refers to a entry, resetting the column contents.
   *
   * @param entry A pointer to the entry
   */
  virtual void modifyEntries(Data::EntryVec entries);
  /**
   * Removes any item which refers to a certain entry.
   *
   * @param entry A pointer to the entry
   */
  virtual void removeEntries(Data::EntryVec entries);

  virtual void addField(Data::CollPtr, Data::FieldPtr field);
  void addField(Data::FieldPtr field, int width);
  virtual void modifyField(Data::CollPtr, Data::FieldPtr oldField, Data::FieldPtr newField);
  virtual void removeField(Data::CollPtr, Data::FieldPtr field);

  void reorderFields(const Data::FieldVec& fields);
  /**
   * saveConfig is only needed for custom collections */
  void saveConfig(Data::CollPtr coll, int saveConfig);
  /**
   * Select all visible items.
   */
  void selectAllVisible();
  int visibleItems() const { return m_filter ? m_visibleItems : childCount(); }
  /**
   * Set max size of pixmaps.
   *
   * @param width Width
   * @param height Height
   */
  void setPixmapSize(int width, int height) { m_pixWidth = width; m_pixHeight = height; }
  void resetEntryStatus();

  virtual int compare(int col, const GUI::ListViewItem* item1, GUI::ListViewItem* item2, bool asc);

public slots:
  /**
   * Resets the list view, clearing and deleting all items.
   */
  void slotReset();
  /**
   * Refreshes the view, repopulating all items.
   */
  void slotRefresh();
  /**
   * Refreshes all images only.
   */
  void slotRefreshImages();

private:
  DetailedEntryItem* addEntryInternal(Data::EntryPtr entry);
  int compareColumn(int col, const GUI::ListViewItem* item1, GUI::ListViewItem* item2, bool asc);

  /**
   * A helper method to populate an item. The column text is initialized by pulling
   * the contents from the entry pointer of the item, so it should properly be set
   * before this method is called.
   *
   * @param item A pointer to the item
   */
  void populateItem(DetailedEntryItem* item);
  void populateColumn(int col);
  void setPixmapAndText(DetailedEntryItem* item, int col, Data::FieldPtr field);

  /**
   * A helper method to locate any item which refers to a certain entry. If none
   * is found, a NULL pointer is returned.
   *
   * @param entry A pointer to the entry
   * @return A pointer to the item
   */
  DetailedEntryItem* const locateItem(Data::EntryPtr entry);
  void showColumn(int col);
  void hideColumn(int col);
  void updateFirstSection();
  void resetComparisons();

private slots:
  /**
   * Handles the appearance of the popup menu.
   *
   * @param item A pointer to the list item underneath the mouse
   * @param point The location point
   * @param col The column number, not currently used
   */
  void contextMenuRequested(QListViewItem* item, const QPoint& point, int col);
  void slotHeaderMenuActivated(int id);
  void slotCacheColumnWidth(int section, int oldSize, int newSize);
  /**
   * Slot to update the position of the pixmap
   */
  void slotUpdatePixmap();

private:
  struct ConfigInfo {
    QStringList cols;
    QValueList<int> widths;
    QValueList<int> order;
    int colSorted;
    bool ascSort : 1;
    int prevSort;
    int prev2Sort;
  };

  KPopupMenu* m_headerMenu;
  QValueVector<int> m_columnWidths;
  QValueVector<bool> m_isDirty;
  QValueVector<int> m_imageColumns;
  bool m_loadingCollection;
  QPixmap m_entryPix;
  QPixmap m_checkPix;

  FilterPtr m_filter;
  int m_prevSortColumn;
  int m_prev2SortColumn;
  int m_firstSection;
  int m_visibleItems;
  int m_pixWidth;
  int m_pixHeight;
};

} // end namespace;
#endif
