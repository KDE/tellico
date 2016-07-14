/***************************************************************************
    Copyright (C) 2008-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

// a good portion of this was borrowed from akonadi/collectionstatisticsdelegate.cpp, which is
// Copyright (c) 2008 Thomas McGuire <thomas.mcguire@gmx.net>
// used under the terms of the GNU LGPL v2

#include "countdelegate.h"
#include "../models/models.h"

#include <QTreeView>
#include <QPainter>

#include <KColorScheme>

using Tellico::GUI::CountDelegate;

CountDelegate::CountDelegate(QTreeView* parent_) : QStyledItemDelegate(parent_), m_showCount(false) {
}

CountDelegate::~CountDelegate() {
}

QTreeView* CountDelegate::parent() const {
  return static_cast<QTreeView*>(QObject::parent());
}

void CountDelegate::initStyleOption(QStyleOptionViewItem* option_,
                                    const QModelIndex& index_) const {
  if(m_showCount) {
    QStyleOptionViewItemV4* noTextOption = qstyleoption_cast<QStyleOptionViewItemV4*>(option_);
    QStyledItemDelegate::initStyleOption(noTextOption, index_);
    noTextOption->text.clear();
  } else {
    QStyledItemDelegate::initStyleOption(option_, index_);
  }
}

void CountDelegate::paint(QPainter* painter_,
                          const QStyleOptionViewItem& option_,
                          const QModelIndex& index_) const {
  Q_ASSERT(index_.isValid());

  // only paint for first column and for
  // items having children
  m_showCount = index_.column() == 0 && index_.model()->hasChildren(index_);

  // First, paint the basic, but without the text. We remove the text
  // in initStyleOption(), which gets called by QStyledItemDelegate::paint().
  QStyledItemDelegate::paint(painter_, option_, index_);

  if(!m_showCount) {
    return;
  }
  m_showCount = false;

  // Now, we retrieve the correct style option by calling initStyleOption from
  // the superclass.
  QStyleOptionViewItemV4 option4 = option_;
  QStyledItemDelegate::initStyleOption(&option4, index_);
  QString text = option4.text;

  QVariant countValue = index_.data(RowCountRole);
  QString countString = QString::fromLatin1(" (%1)").arg(countValue.toInt());

  // Now calculate the rectangle for the text
  QStyle* s = parent()->style();
  const QWidget* widget = option4.widget;
  const QRect itemRect = s->subElementRect(QStyle::SE_ItemViewItemText, &option4, widget);

  // Squeeze the folder text if it is to big and calculate the rectangles
  // where the text and the count will be drawn to
  QFontMetrics fm = painter_->fontMetrics();
  const int countWidth = fm.width(countString);
  if(fm.width(text) + countWidth > itemRect.width()) {
    text = fm.elidedText(text, Qt::ElideRight, itemRect.width() - countWidth);
  }

  int textWidth = fm.width(text);
  QRect textRect = itemRect;
  textRect.setRight(textRect.left() + textWidth);
  QRect countRect = itemRect;
  countRect.setLeft(textRect.right());

  KColorScheme::ColorSet cs = (option4.state & QStyle::State_Selected) ?
                               KColorScheme::Selection : KColorScheme::View;
  QColor countColor = KColorScheme(QPalette::Active, cs).foreground(KColorScheme::LinkText).color();

  painter_->save();
  painter_->drawText(textRect, Qt::AlignLeft, text);
  painter_->setPen(countColor);
  painter_->drawText(countRect, Qt::AlignLeft, countString);
  painter_->restore();
}
