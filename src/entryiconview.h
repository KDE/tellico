/***************************************************************************
    copyright            : (C) 2002-2006 by Robby Stephenson
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

#include "observer.h"
#include "entry.h"

#include <kiconview.h>

#include <qintdict.h>

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

  EntryIconViewItem* firstItem() const;

  virtual void clear();
  void refresh();
  void showEntries(const Data::EntryVec& entries);
  /**
   * Adds a new list item showing the details for a entry.
   *
   * @param entry A pointer to the entry
   */
  virtual void    addEntries(Data::EntryVec entries);
  virtual void modifyEntries(Data::EntryVec entries);
  virtual void removeEntries(Data::EntryVec entries);

  const QString& imageField();
  void setMaxAllowedIconWidth(int width);
  int maxAllowedIconWidth() const { return m_maxAllowedIconWidth; }

  const QPixmap& defaultPixmap();
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

  Data::CollPtr m_coll;
  Data::EntryVec m_entries;
  QString m_imageField;
  QIntDict<QPixmap> m_defaultPixmaps;
  int m_maxAllowedIconWidth;
  int m_maxIconWidth;
  int m_maxIconHeight;
};

class EntryIconViewItem : public KIconViewItem {
public:
  EntryIconViewItem(EntryIconView* parent, Data::EntryPtr entry);
  ~EntryIconViewItem();

  EntryIconView* iconView() const { return static_cast<EntryIconView*>(KIconViewItem::iconView()); }
  EntryIconViewItem* nextItem() const { return static_cast<EntryIconViewItem*>(KIconViewItem::nextItem()); }

  Data::EntryPtr entry() const { return m_entry; }
  virtual void setSelected(bool s, bool cb);
  virtual void setSelected(bool s);

  bool usesImage() const { return m_usesImage; }
  void updatePixmap();

  void update();

protected:
  virtual void calcRect(const QString& text = QString::null);
  virtual void paintItem(QPainter* p, const QColorGroup& cg);
  virtual void paintFocus(QPainter* p, const QColorGroup& cg);
  void paintPixmap(QPainter* p, const QColorGroup& cg);
  void paintText(QPainter* p, const QColorGroup& cg);

private:
  Data::EntryPtr m_entry;
  bool m_usesImage : 1;
};

} // end namespace
#endif
