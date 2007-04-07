/***************************************************************************
    copyright            : (C) 2005-2007 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_DETAILEDENTRYITEM_H
#define TELLICO_DETAILEDENTRYITEM_H

class QTimer;

#include "entryitem.h"

#include <qdatetime.h>

namespace Tellico {

class DetailedListView;

/**
 * @author Robby Stephenson
 */
class DetailedEntryItem : public EntryItem {
public:
  enum State { Normal, New, Modified };

  DetailedEntryItem(DetailedListView* parent, Data::EntryPtr entry);
  ~DetailedEntryItem();

  void setState(State state);

  virtual QColor backgroundColor(int column);
  virtual void paintCell(QPainter* p, const QColorGroup& cg,
                         int column, int width, int align);

  virtual int compare(QListViewItem* item, int col, bool ascending) const;

  DetailedListView* listView() const;

private:
  /**
   * Paints a focus indicator on the rectangle (current item). Disable for current items.
   */
  void paintFocus(QPainter*, const QColorGroup&, const QRect&);

  State m_state;
  QTime* m_time;
  QTimer* m_timer;
};

}

#endif
