/***************************************************************************
    copyright            : (C) 2001-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef MULTISELECTIONLISTVIEW_H
#define MULTISELECTIONLISTVIEW_H

#include "tellico_utils.h" // for KDE_IS_VERSION macro

#include <klistview.h>

namespace Tellico {

/**
 * A MultiSelectionListView keeps track of the selected items and allows subclasses to determine
 * whether child items may be selected or not.
 *
 * @author Robby Stephenson
 * @version $Id: multiselectionlistview.h 1079 2005-02-05 23:50:11Z robby $
 */
class MultiSelectionListView : public KListView {
Q_OBJECT

friend class MultiSelectionListViewItem;

public:
  MultiSelectionListView(QWidget* parent, const char* name = 0);
  ~MultiSelectionListView() {};

  /**
   * Returns a list of the currently selected items;
   *
   * @return The list of selected items
   */
  const QPtrList<MultiSelectionListViewItem>& selectedItems() const { return m_selectedItems; }
  /**
   * Used to determine whether an item may be selected without having to setSelectable on
   * every child item. The default implementation always returns @p true;
   *
   * @return Whether the item may be selected
   */
  virtual bool isSelectable(MultiSelectionListViewItem*) const { return true; }
  // taken from KDE bug 59791
#if !KDE_IS_VERSION(3,3,90)
  void setShadeSortColumn(bool shade_);
  bool shadeSortColumn() const { return m_shadeSortColumn; }
  const QColor& background2() const { return m_backColor2; }
  const QColor& alternateBackground2() const { return m_altColor2; }
#endif

private slots:
  void slotUpdateColors();

private:
  /**
   * Updates the pointer list.
   *
   * @param item The item being selected or deselected
   * @param s Selected or not
   */
  void updateSelected(MultiSelectionListViewItem* item, bool s) const;

  mutable QPtrList<MultiSelectionListViewItem> m_selectedItems;
#if !KDE_IS_VERSION(3,3,90)
  bool m_shadeSortColumn;
  QColor m_backColor2;
  QColor m_altColor2;
#endif
};

/**
 * @author Robby Stephenson
 * @version $Id: multiselectionlistview.h 1079 2005-02-05 23:50:11Z robby $
 */
class MultiSelectionListViewItem : public KListViewItem {
public:
  virtual ~MultiSelectionListViewItem();

  MultiSelectionListViewItem(MultiSelectionListView* parent) : KListViewItem(parent) {}
  MultiSelectionListViewItem(MultiSelectionListViewItem* parent) : KListViewItem(parent) {}
  MultiSelectionListViewItem(MultiSelectionListView* parent, const QString& text) : KListViewItem(parent, text) {}
  MultiSelectionListViewItem(MultiSelectionListViewItem* parent, const QString& text) : KListViewItem(parent, text) {}

  virtual void setSelected(bool selected);
#if !KDE_IS_VERSION(3,3,90)
  const QColor& backgroundColor(int column);
  virtual void paintCell(QPainter* painter, const QColorGroup& colorGroup,
                         int column, int width, int alignment);
#endif
};

} // end namespace

#endif
