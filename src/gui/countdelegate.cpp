/***************************************************************************
    Copyright (C) 2008-2020 Robby Stephenson <robby@periapsis.org>
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
    QStyleOptionViewItem* noTextOption = qstyleoption_cast<QStyleOptionViewItem*>(option_);
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

  // only paint for first column and no parent
  m_showCount = index_.column() == 0 && !index_.parent().isValid();

  // First, paint the basic, but without the text. We remove the text
  // in initStyleOption(), which gets called by QStyledItemDelegate::paint().
  QStyledItemDelegate::paint(painter_, option_, index_);

  if(!m_showCount) {
    return;
  }
  m_showCount = false;

  // Now, we retrieve the correct style option by calling initStyleOption from
  // the superclass.
  QStyleOptionViewItem option(option_);
  QStyledItemDelegate::initStyleOption(&option, index_);
  QString text = option.text;

  QVariant countValue = index_.data(RowCountRole);
  QString countString = QStringLiteral(" (%1)").arg(countValue.toInt());

  // Now calculate the rectangle for the text
  QStyle* s = parent()->style();
  const QWidget* widget = option.widget;
  const QRect itemRect = s->subElementRect(QStyle::SE_ItemViewItemText, &option, widget);

  // Squeeze the folder text if it is to big and calculate the rectangles
  // where the text and the count will be drawn to
  QFontMetrics fm(painter_->fontMetrics());
  const int countWidth = fm.horizontalAdvance(countString);
  int textWidth = fm.horizontalAdvance(text);
  if(textWidth + countWidth > itemRect.width()) {
    text = fm.elidedText(text, Qt::ElideRight, itemRect.width() - countWidth);
    textWidth = fm.horizontalAdvance(text);
  }

  const int top = itemRect.top() + (itemRect.height() - fm.height()) / 2;
  QRect textRect = itemRect;
  textRect.setRight(textRect.left() + textWidth);
  textRect.setTop(top);
  textRect.setHeight(fm.height());
  QRect countRect = itemRect;
  countRect.setLeft(textRect.right());
  countRect.setTop(top);
  countRect.setHeight(fm.height());

  QColor textColor = index_.data(Qt::ForegroundRole).value<QColor>();
  if(!textColor.isValid()) {
    if(option.state & QStyle::State_Selected) {
      textColor = option.palette.highlightedText().color();
    } else {
      textColor = option.palette.text().color();
    }
  }
  KColorScheme::ColorSet colorSet = (option.state & QStyle::State_Selected) ?
                                     KColorScheme::Selection : KColorScheme::View;
  KColorScheme cs(QPalette::Active, colorSet);
  QColor countColor = cs.foreground(KColorScheme::LinkText).color();

  painter_->save();
  painter_->setPen(textColor);
  painter_->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
  painter_->setPen(countColor);
  painter_->drawText(countRect, Qt::AlignLeft | Qt::AlignVCenter, countString);
  painter_->restore();
}
