/***************************************************************************
    copyright            : (C) 2002-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICOENTRYICONVIEW_H
#define TELLICOENTRYICONVIEW_H

class KPopupMenu;

#include "observer.h"
#include "entry.h"
#include "datavectors.h"

#include <kiconview.h>

namespace Tellico {
  class EntryIconViewItem;
  namespace Data {
    class Collection;
  }

/**
 * @author Robby Stephenson
 */
class EntryIconView : public KIconView, public Observer {
Q_OBJECT

friend class EntryIconViewItem;

public:
  EntryIconView(QWidget* parent, const char* name = 0);

  virtual void clear();
  void refresh();
  void showEntries(const Data::EntryVec& entries);
  /**
   * Adds a new list item showing the details for a entry.
   *
   * @param entry A pointer to the entry
   */
  virtual void    addEntry(Data::Entry* entry);
  virtual void modifyEntry(Data::Entry*) { refresh(); }
  virtual void removeEntry(Data::Entry* entry);

  const QString& imageField();
  void setMaxIconWidth(uint width);
  uint maxIconWidth() const { return m_maxIconWidth; }

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
  void fillView();

  KSharedPtr<Data::Collection> m_coll;
  Data::EntryVec m_entries;
  QString m_imageField;
  QPixmap m_defaultPixmap;
  KPopupMenu* m_itemMenu;
  uint m_maxIconWidth;
};

class EntryIconViewItem : public KIconViewItem {
public:
  EntryIconViewItem(EntryIconView* parent, Data::Entry* entry);
  ~EntryIconViewItem();

  Data::Entry* entry() const { return m_entry; }
  virtual void setSelected(bool s, bool cb);
  virtual void setSelected(bool s);

private:
  Data::EntryPtr m_entry;
};

} // end namespace
#endif
