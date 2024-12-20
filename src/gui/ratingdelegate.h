/*
 * Copyright (C) 2010 Stefan Burnicki (stefan.burnicki@gmx.de)
 * <https://bangarangkde.wordpress.com> BANGARANG MEDIA PLAYER
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

#ifndef RATINGDELEGATE_H
#define RATINGDELEGATE_H

#include <QStyledItemDelegate>

/**
  * @brief Displays the usual rating stars in the tablewidget. Not editable
  */
class RatingDelegate : public QStyledItemDelegate {
Q_OBJECT

public:
  RatingDelegate(QObject* parent = nullptr);

  void setMaxRating(uint rating_) { m_maxRating = rating_; }
  void paint(QPainter* painter, const QStyleOptionViewItem& option,const QModelIndex& index) const override;

  QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
  uint m_maxRating;
};

#endif // RATINGDELEGATE_H
