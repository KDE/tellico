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

#ifndef TELLICO_COUNTDELEGATE_H
#define TELLICO_COUNTDELEGATE_H

#include <QStyledItemDelegate>

class QTreeView;

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class CountDelegate : public QStyledItemDelegate {
Q_OBJECT

public:
  CountDelegate(QTreeView* parent);
  virtual ~CountDelegate();

protected:
  virtual void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const;
  virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

private:
  QTreeView* parent() const;

  mutable bool m_showCount;
};

  } // end GUI namespace
} // end namespace
#endif
