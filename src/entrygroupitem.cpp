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

#include "entrygroupitem.h"
#include "entry.h"
#include "collection.h"
#include "gui/ratingwidget.h"
#include "tellico_debug.h"
#include "latin1literal.h"
#include "core/tellico_config.h"

#include <kiconloader.h>
#include <klocale.h>

#include <qpixmap.h>
#include <qpainter.h>

using Tellico::EntryGroupItem;

EntryGroupItem::EntryGroupItem(GUI::ListView* parent_, Data::EntryGroup* group_, int fieldType_)
    : GUI::CountedItem(parent_), m_group(group_), m_fieldType(fieldType_) {
  setText(0, group_->groupName());
  m_emptyGroup = group_->groupName() == i18n(Data::Collection::s_emptyGroupTitle);
}

QPixmap EntryGroupItem::ratingPixmap() {
  if(!m_group) {
    return QPixmap();
  }
  QPixmap stars = GUI::RatingWidget::pixmap(m_group->groupName());
  if(m_pix.isNull() && stars.isNull()) {
    m_emptyGroup = true;
    return QPixmap();
  }

  QPixmap newPix(m_pix.width() + 4 + stars.width(), QMAX(m_pix.height(), stars.height()));
  newPix.fill(isSelected() ? listView()->colorGroup().highlight() : backgroundColor(0));
  QPainter p(&newPix);
  if(!m_pix.isNull()) {
    p.drawPixmap(0, 0, m_pix);
  }
  if(!stars.isNull()) {
    p.drawPixmap(m_pix.width() + 4, 0, stars);
  }
  p.end();
  return newPix;
}

void EntryGroupItem::setPixmap(int col, const QPixmap& pix) {
  m_pix = pix;
  GUI::CountedItem::setPixmap(col, m_pix);
}

void EntryGroupItem::paintCell(QPainter* p_, const QColorGroup& cg_,
                               int column_, int width_, int align_) {
  if(column_> 0 || m_fieldType != Data::Field::Rating || m_emptyGroup) {
    return GUI::CountedItem::paintCell(p_, cg_, column_, width_, align_);
  }

  QString oldText = text(column_);
  // "\t\t" is the flag to not paint the item
  // CountedItem already uses "\t"
  if(oldText == Latin1Literal("\t\t")) {
    return;
  }

  setText(column_, QString::fromLatin1("\t\t"));
  GUI::CountedItem::setPixmap(column_, ratingPixmap());
  GUI::CountedItem::paintCell(p_, cg_, column_, width_, align_);
//  GUI::CountedItem::setPixmap(column_, m_pix);
  setText(column_, oldText);
}

// prepend a tab character to always sort the empty group name first
// also check for surname prefixes
QString EntryGroupItem::key(int col_, bool) const {
  if(col_ > 0) {
    return text(col_);
  }

  if((m_fieldType == Data::Field::Rating || text(col_).isEmpty())
     && pixmap(col_) && !pixmap(col_)->isNull()) {
    // a little weird, sort for width, too, in case of rating widget
    // but sort reverse by width
    return QChar('\t') + QString::number(1000-pixmap(col_)->width());
  } else if(text(col_) == i18n(Data::Collection::s_emptyGroupTitle)) {
    return QChar('\t');
  }

  if(m_text.isEmpty() || m_text != text(col_)) {
    m_text = text(col_);
    if(Config::autoFormat()) {
      const QStringList prefixes = Config::surnamePrefixList();
      // build a regexp to match surname prefixes
      // complicated by fact that prefix could have an apostrophe
      QString tokens;
      for(QStringList::ConstIterator it = prefixes.begin();
                                     it != prefixes.end();
                                     ++it) {
        tokens += (*it);
        if(!(*it).endsWith(QChar('\''))) {
          tokens += QString::fromLatin1("\\s");
        }
        // if it's not the last item, add a pipe
        if((*it) != prefixes.last()) {
          tokens += QChar('|');
        }
      }
      QRegExp rx(QString::fromLatin1("^(") + tokens + QChar(')'), false);
      // expensive
      if(rx.search(m_text) > -1) {
        m_key = m_text.mid(rx.matchedLength());
      } else {
        m_key = m_text;
      }
    } else {
      m_key = m_text;
    }
  }

  return m_key;
}

size_t EntryGroupItem::count() const {
  return m_group ? m_group->count() : GUI::CountedItem::count();
}

Tellico::Data::EntryVec EntryGroupItem::entries() const {
  return m_group ? *m_group : Data::EntryVec();
}
