/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_FILTERVIEW_H
#define TELLICO_FILTERVIEW_H

class KPopupMenu;

#include "gui/listview.h"
#include "observer.h"
#include "filteritem.h"

#include <qdict.h>

namespace Tellico {
  class FilterItem;

/**
 * @author Robby Stephenson
 */
class FilterView : public GUI::ListView, public Observer {
Q_OBJECT

public:
  FilterView(QWidget* parent, const char* name=0);

  virtual bool isSelectable(GUI::ListViewItem*) const;

  void addCollection(Data::Collection* coll);

  virtual void    addEntry(Data::Entry* entry);
  virtual void modifyEntry(Data::Entry* entry);
  virtual void removeEntry(Data::Entry* entry);

  virtual void    addFilter(Filter* filter);
  virtual void modifyFilter(Filter*) {}
  virtual void removeFilter(Filter* filter);

private slots:
  /**
   * Handles the appearance of the popup menu.
   *
   * @param item A pointer to the item underneath the mouse
   * @param point The location point
   * @param col The column number, not currently used
   */
  void contextMenuRequested(QListViewItem* item, const QPoint& point, int col);

  /**
   * Modify a saved filter
   */
  void slotModifyFilter();
  /**
   * Delete a saved filter
   */
  void slotDeleteFilter();

private:
  virtual void setSorting(int column, bool ascending = true);

  bool m_notSortedYet;
  KPopupMenu* m_filterMenu;
  QDict<FilterItem> m_itemDict;
};

} // end namespace

#endif
