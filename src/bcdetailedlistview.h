/***************************************************************************
                                bcdetailedlistview.h
                             -------------------
    begin                : Tue Sep 4 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BCDETAILEDLISTVIEW_H
#define BCDETAILEDLISTVIEW_H

class BCUnitItem;
class BCCollection;
class BCAttribute;
class BookcaseDoc;
class BCFilter;

// needed for BCUnitList definition
#include <bcunit.h>

#include <klistview.h>
#include <kpopupmenu.h>

#include <qpoint.h>
#include <qstringlist.h>
#include <qpixmap.h>
#include <qvaluevector.h>
#include <qguardedptr.h>

/**
 * The BCDetailedListView class shows detailed information about units in the
 * collection.
 *
 * @author Robby Stephenson
 * @version $Id: bcdetailedlistview.h,v 1.3 2003/05/02 06:04:21 robby Exp $
 */
class BCDetailedListView : public KListView {
Q_OBJECT

public:
  /**
   * The constructor initializes the popup menu, but no columns are inserted.
   *
   * @param doc A pointer to the BookcaseDoc, used for filling out headers
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  BCDetailedListView(BookcaseDoc* doc, QWidget* parent, const char* name=0);
  ~BCDetailedListView();
  
  bool eventFilter(QObject* obj, QEvent* ev);
  /**
   * Rearranges the columns, given the names of the attributes.
   *
   * @param colNames A list of the attribute names in the order to be shown
   */
  void setColumns(BCCollection* coll, const QValueList<int>& colNames);
  QStringList visibleColumns() const;
  void setFilter(const BCFilter* filter);
  const BCFilter* filter() const;
   
public slots:
  /**
   */
  void slotAddCollection(BCCollection* coll);
  /**
   * Resets the list view, clearing and deleting all items.
   */
  void slotReset();
  /**
   * Adds a new list item showing the details for a unit.
   *
   * @param unit A pointer to the unit
   */
  void slotAddItem(BCUnit* unit);
  /**
   * Modifies any item which refers to a unit, resetting the column contents.
   *
   * @param unit A pointer to the unit
   */
  void slotModifyItem(BCUnit* unit);
  /**
   * Removes any item which refers to a certain unit.
   *
   * @param unit A pointer to the unit
   */
  void slotRemoveItem(BCUnit* unit);
  /**
   * Removes all item which refers to a unit within a collection.
   *
   * @param coll A pointer to the collection
   */
  void slotRemoveItem(BCCollection* coll);
  /**
   * Selects the item which refers to a certain unit.
   *
   * @param unit A pointer to the unit
   */
  void slotSetSelected(BCUnit* unit);
  /**
   * Handles emitting the signal indicating a desire to delete the unit attached
   * to the current list item.
   */
  void slotHandleDelete();
  void slotRefresh();
  void slotAddColumn(BCCollection*, BCAttribute* att);
  void slotModifyColumn(BCCollection*, BCAttribute* newAtt, BCAttribute* oldAtt);
  void slotRemoveColumn(BCCollection*, BCAttribute* att);

protected:
  /**
   * A helper method to populate an item. The column text is initialized by pulling
   * the contents from the unit pointer of the item, so it should properly be set
   * before this method is called.
   *
   * @param item A pointer to the item
   */
  void populateItem(BCUnitItem* item);
  void setPixmapAndText(BCUnitItem* item, int col, BCAttribute* att);
  
  /**
   * A helper method to locate any item which refers to a certain unit. If none
   * is found, a NULL pointer is returned.
   *
   * @param unit A pointer to the unit
   * @return A pointer to the item
   */
  BCUnitItem* const locateItem(const BCUnit* unit);
  void showColumn(int col);
  void hideColumn(int col);

protected slots:
  /**
   * Handles the appearance of the popup menu.
   *
   * @param item A pointer to the list item underneath the mouse
   * @param point The location point
   * @param col The column number, not currently used
   */
  void slotRMB(QListViewItem* item, const QPoint& point, int col);
  /**
   * Handles everything when an item is selected. The proper signal is emitted.
   */
  void slotSelectionChanged();
  /**
   * Clears the selection.
   */
  void slotClearSelection();
  void slotHeaderMenuActivated(int id);
  void slotCacheColumnWidth(int section, int oldSize, int newSize);

signals:
  /**
   * Signals that the selected units have changed. Zero, one or more may be selected.
   *
   * @param list A list of the selected items, may be empty.
   */
  void signalUnitSelected(const BCUnitList& list);
  /**
   * Signals a desire to delete a unit.
   *
   * @param unit A pointer to the unit
   */
  void signalDeleteUnit(BCUnit* unit);

private:
  BookcaseDoc* m_doc;
  KPopupMenu* m_itemMenu;
  KPopupMenu* m_headerMenu;
  QValueVector<bool> m_visibleColumns;
  QValueVector<int> m_columnWidths;
  QPixmap m_bookPix;
  QPixmap m_checkPix;
  BCUnitList m_selectedUnits;
  const BCFilter* m_filter;
};

#endif
