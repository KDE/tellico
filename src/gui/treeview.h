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

#ifndef TELLICO_GUI_TREEVIEW_H
#define TELLICO_GUI_TREEVIEW_H

#include <QTreeView>

namespace Tellico {

  class EntrySortModel;

  namespace GUI {

/**
 * @author Robby Stephenson
 */
class TreeView : public QTreeView {
Q_OBJECT

public:
  TreeView(QWidget* parent);
  virtual ~TreeView();

  virtual void setModel(QAbstractItemModel* model);
  EntrySortModel* sortModel() const;

  bool isEmpty() const;

  void setSorting(Qt::SortOrder order, int role);
  Qt::SortOrder sortOrder() const;
  int sortRole() const;
};

  } // end namespace
} // end namespace
#endif
