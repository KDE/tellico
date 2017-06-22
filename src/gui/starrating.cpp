/*
 * Copyright (C) 2010 Stefan Burnicki (stefan.burnicki@gmx.de)
 * <http://gitorious.org/bangarang> BANGARANG MEDIA PLAYER
 * Copyright 2011 Jörg Ehrichs <joerg.ehrichs@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "starrating.h"

#include <QPainter>
#include <QSize>
#include <QRect>
#include <QIcon>

StarRating::StarRating(int rating, int size, QPoint point) : m_rating(rating), m_maxRating(MaxRating), m_point(point) {
  setSize(size);
}

bool StarRating::valid(int rating) const {
  return (rating >= MinRating && rating <= MaxRating);
}

void StarRating::paint(QPainter* painter) {
  painter->save();

  painter->translate(m_point + QPoint(StarRating::Margin, StarRating::Margin));
  for(int i = 1; i <= m_rating; ++i) {
    painter->drawPixmap(((i - 1) * (m_starSize + 2*Margin)), 0, m_starNormal);
  }

  painter->restore();
}

void StarRating::setRating(int rating_) {
  m_rating = qMin(rating_, m_maxRating);
}

void StarRating::setMaxRating(int maxRating_) {
  m_maxRating = qMin(int(MaxRating), maxRating_);
}

void StarRating::setSize(int size) {
  int px = (size < Small) ? Small : size;
  QSize sz = QSize(px, px);
  QIcon icon = QIcon(QLatin1String(":/icons/star_on"));
  m_starNormal = icon.pixmap(sz);
  m_starSize = m_starNormal.size().width(); //maybe we didn't get the full size
  sz = QSize(m_starSize, m_starSize);
  m_starInactive = icon.pixmap(sz, QIcon::Disabled);
}

//static
QSize StarRating::sizeHint(int size) {
  // *__*__*__*__*   +  StarRating::Margin around it
  const int bothMargin = (StarRating::Margin * 2);
  return QSize( 5 * (size + 2) - 2, size) + QSize(bothMargin, bothMargin);
}
