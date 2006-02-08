/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "detailedentryitem.h"
#include "entry.h"
#include "tellico_utils.h"

#include <klocale.h>
#include <kstringhandler.h>

#include <qpainter.h>
#include <qheader.h>
#include <qtimer.h>

namespace {
  static const short ENTRY_MAX_MINUTES_MESSAGE = 5;
}

using Tellico::DetailedEntryItem;

DetailedEntryItem::DetailedEntryItem(DetailedListView* parent_, Data::EntryPtr entry_)
    : EntryItem(parent_, entry_), m_state(Normal), m_time(0), m_timer(0) {
}

DetailedEntryItem::~DetailedEntryItem() {
  delete m_time;
  m_time = 0;
  delete m_timer;
  m_timer = 0;
}

void DetailedEntryItem::setState(State state_) {
  if(m_state == state_) {
    return;
  }
  m_state = state_;

  if(m_state == Normal) {
    delete m_time;
    m_time = 0;
    delete m_timer;
    m_timer = 0;
  } else {
    if(!m_time) {
      m_time = new QTime;
    }
    m_time->start();

    if(!m_timer) {
      m_timer = new QTimer();
      m_timer->connect(m_timer, SIGNAL(timeout()), listView(), SLOT(triggerUpdate()));
    }
    m_timer->start(30 * 1000); // every 30 seconds
  }

  // have to put this in a timer, or it doesn't update properly
  QTimer::singleShot(500, listView(), SLOT(triggerUpdate()));
}

void DetailedEntryItem::paintCell(QPainter* p_, const QColorGroup& cg_,
                                  int column_, int width_, int align_) {
  if(m_state == Normal) {
    EntryItem::paintCell(p_, cg_, column_, width_, align_);
    return;
  }

  int t = m_time->elapsed()/(60 * 1000);
  if(t > ENTRY_MAX_MINUTES_MESSAGE) {
    setState(Normal);
    t = 0;
  }

  QFont f = p_->font();
  f.setBold(true);
  if(m_state == New) {
    f.setItalic(true);
  }
  p_->setFont(f);

  // taken from ListViewItem, but without line drawn to right of cell
  QColorGroup cg = cg_;
  const QPixmap* pm = listView()->viewport()->backgroundPixmap();
  if(pm && !pm->isNull()) {
    cg.setBrush(QColorGroup::Base, QBrush(backgroundColor(column_), *pm));
    QPoint o = p_->brushOrigin();
    p_->setBrushOrigin(o.x()-listView()->contentsX(), o.y()-listView()->contentsY());
  } else {
    cg.setColor(listView()->viewport()->backgroundMode() == Qt::FixedColor ?
                QColorGroup::Background : QColorGroup::Base,
                backgroundColor(column_));
  }
  // don't call KListViewItem::paintCell() since that also does alternate painting, etc...
  QListViewItem::paintCell(p_, cg, column_, width_, align_);
}

QColor DetailedEntryItem::backgroundColor(int column_) {
  GUI::ListView* lv = listView();
  if(!lv || m_state == Normal || isSelected()) {
    return EntryItem::backgroundColor(column_);
  }
  int t = m_time->elapsed()/(60 * 1000);
  if(t > ENTRY_MAX_MINUTES_MESSAGE) {
    return EntryItem::backgroundColor(column_);
  }
  return blendColors(lv->colorGroup().highlight(),
                     lv->colorGroup().base(),
                     80 + 20*t/ENTRY_MAX_MINUTES_MESSAGE /* percent */);
                     // no more than 20% of highlight color
}

void DetailedEntryItem::paintFocus(QPainter*, const QColorGroup&, const QRect&) {
// don't paint anything
}
