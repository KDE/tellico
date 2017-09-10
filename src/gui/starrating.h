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

#ifndef STARRATING_H
#define STARRATING_H

#include <QMetaType>
#include <QPainter>

/**
  * @brief This file provides the StarRating for painting rating stars.
  * It supports ratings from 1 to 10 in different sizes.
  */
class StarRating {

public:
  enum Size { Small = 8, Medium = 12, Big = 16, Huge = 22 };
  enum MinMax{ InvalidRating = -1, MinRating = 0, MaxRating = 10 };

  static const int Margin = 1;

  explicit StarRating(int rating = 0, int size = Small, QPoint point = QPoint(0, 0));

  void setRating(int rating);
  void setMaxRating(int maxRating);
  void setSize(int size);
  void setPoint(QPoint point) { m_point = point; }

  bool valid(int rating) const;
  int rating() const { return m_rating; }
  void paint(QPainter *painter);

  QSize sizeHint() const { return sizeHint(m_starSize); }
  static QSize sizeHint(int size);

protected:
  QPixmap m_starNormal;
  QPixmap m_starInactive;

  int m_rating;
  int m_maxRating;
  QPoint m_point;
  int m_starSize;
};

Q_DECLARE_METATYPE(StarRating)

#endif
