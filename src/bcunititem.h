/* *************************************************************************
                          bcunititem.h  -  description
                             -------------------
    begin                : Sat Oct 20 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@radiojodi.com
 * *************************************************************************/

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 * *************************************************************************/

#ifndef BCUNITITEM_H
#define BCUNITITEM_H

#include <klistview.h>

class BCUnit;

/**
 * The BCUnitItem is a subclass of KListViewItem containing a pointer to a BCUnit.
 *
 * The unit pointer allows easy access to listview items which refer to a certain unit.
 *
 * @see BCUnit
 *
 * @author Robby Stephenson
 * @version $Id: bcunititem.h,v 1.7 2001/11/03 18:15:05 robby Exp $
 */
class BCUnitItem : public KListViewItem {
public:
  /**
   * This constructor is for items which are direct children of a KListView object.
   *
   * @param parent A pointer to the parent
   * @param unit A pointer to the unit to which the item refers
   */
  BCUnitItem(KListView* parent, BCUnit* unit)
   : KListViewItem(parent), m_unit(unit) {}
  /**
   * This constructor is for items which have other KListViewItems as parents. It
   * initializes the text in the first column, as well.
   *
   * @param parent A pointer to the parent
   * @param text The text in the first column
   * @param unit A pointer to the unit to which the item refers
   */
  BCUnitItem(KListViewItem* parent, const QString& text, BCUnit* unit)
   : KListViewItem(parent, text), m_unit(unit) {}
  /**
   */
  ~BCUnitItem() {};

  /**
   * Returns a pointer to the unit to which the item refers
   *
   * @return The unit pointer
   */
  BCUnit* unit() { return m_unit; }

private:
  BCUnit* m_unit;
};

/**
 * The ParentItem is a subclass of KListViewItem which includes an id reference number.
 *
 * The id allows for matching on collections or whatever. The id is not a unique identifier
 * of the item itself.
 *
 *
 * @author Robby Stephenson
 * @version $Id: bcunititem.h,v 1.7 2001/11/03 18:15:05 robby Exp $
 */
class ParentItem : public KListViewItem {
public:
  /**
   * This constructor is for items which are direct children of a KListView object.
   *
   * @param parent A pointer to the parent
   * @param text The text in the first column
   * @param id The id number
   */
  ParentItem(KListView* parent, const QString& text, int id)
   : KListViewItem(parent, text), m_id(id) {}
  /**
   * This constructor is for items which are children of another ParentItem and do not
   * have an id reference number. It is primarily used for grouping of the BCUnitItems.
   * The id is set to -1.
   *
   * @see BCUnitItem
   *
   * @param parent A pointer to the parent
   * @param text The text in the first column
   */
  ParentItem(ParentItem* parent, const QString& text)
   : KListViewItem(parent, text), m_id(-1) {}
  /**
   */
  ~ParentItem() {};

  /**
   * Returns the id reference number of the ParentItem.
   *
   * @return id The id number
   */
  int id() { return m_id; }

private:
  int m_id;
};

#endif
