/***************************************************************************
    copyright            : (C) 2002-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BOOKCASEENTRYICONVIEW_H
#define BOOKCASEENTRYICONVIEW_H

class KPopupMenu;

#include "entry.h"

#include <kiconview.h>

namespace Bookcase {
/**
 * @author Robby Stephenson
 * @version $Id: entryiconview.h 760 2004-08-12 00:29:24Z robby $
 */
class EntryIconView : public KIconView {
Q_OBJECT

friend class EntryIconViewItem;

public:
  EntryIconView(QWidget* parent, const char* name = 0);

  virtual void clear();
  void refresh();
  void showCollection(const Data::Collection* collection);
  void showEntryGroup(const Data::EntryGroup* group);
  /**
   * Adds a new list item showing the details for a entry.
   *
   * @param entry A pointer to the entry
   */
  void addEntry(Data::Entry* entry);
  void removeEntry(Data::Entry* entry);
  const QString& imageField();
  void setMaxIconWidth(const unsigned& width);
  const unsigned& maxIconWidth() const { return m_maxIconWidth; };

  const QPixmap& defaultPixmap() { return m_defaultPixmap; }
  /**
   * Returns a list of the currently selected items;
   *
   * @return The list of selected items
   */
  const QPtrList<EntryIconViewItem>& selectedItems() const { return m_selectedItems; }

private slots:
  void slotSelectionChanged();
  void slotDoubleClicked(QIconViewItem* item);
  void slotShowContextMenu(QIconViewItem* item, const QPoint& point);

private:
  /**
   * Updates the pointer list.
   *
   * @param item The item being selected or deselected
   * @param s Selected or not
   */
  void updateSelected(EntryIconViewItem* item, bool s) const;
  mutable QPtrList<EntryIconViewItem> m_selectedItems;

  void findImageField();
  void fillView(const Data::EntryList& list);

  const Data::Collection* m_coll;
  const Data::EntryGroup* m_group;
  QString m_imageField;
  QPixmap m_defaultPixmap;
  KPopupMenu* m_itemMenu;
  unsigned m_maxIconWidth;
};

class EntryIconViewItem : public KIconViewItem {
public:
  EntryIconViewItem(EntryIconView* parent, Data::Entry* entry);
  ~EntryIconViewItem();

  Data::Entry* entry() const { return m_entry; }
  virtual void setSelected(bool s, bool cb);
  virtual void setSelected(bool s);

private:
  Data::Entry* m_entry;
};

} // end namespace
#endif
