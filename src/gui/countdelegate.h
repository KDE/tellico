/***************************************************************************
    copyright            : (C) 2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
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
