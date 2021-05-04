/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_GUI_TABLEFIELDWIDGET_H
#define TELLICO_GUI_TABLEFIELDWIDGET_H

#include "fieldwidget.h"
#include "../datavectors.h"

class QTableWidget;
class QTableWidgetItem;
class QEvent;
class FieldWidgetTest;

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class TableFieldWidget : public FieldWidget {
Q_OBJECT

friend class ::FieldWidgetTest;

public:
  TableFieldWidget(Data::FieldPtr field, QWidget* parent);
  virtual ~TableFieldWidget() {}

  virtual QString text() const Q_DECL_OVERRIDE;
  virtual void setTextImpl(const QString& text) Q_DECL_OVERRIDE;

public Q_SLOTS:
  virtual void clearImpl() Q_DECL_OVERRIDE;

protected:
  virtual QWidget* widget() Q_DECL_OVERRIDE;
  virtual void updateFieldHook(Data::FieldPtr oldField, Data::FieldPtr newField) Q_DECL_OVERRIDE;

private Q_SLOTS:
  void tableContextMenu(QPoint point);
  void horizontalHeaderContextMenu(QPoint point);
  void verticalHeaderContextMenu(QPoint point);
  void slotCheckRows(int row, int col);
  void slotResizeColumn(QTableWidgetItem* item);
  void slotRenameColumn();
  void slotInsertRow();
  void slotRemoveRow();
  void slotMoveRowUp();
  void slotMoveRowDown();
  void slotClear();

private:
  void makeRowContextMenu(QPoint point);
  bool emptyRow(int row) const;
  void labelColumns(Data::FieldPtr field);
  void renameColumn(const QString& newName);

  QTableWidget* m_table;
  int m_columns;
  int m_row;
  int m_col;
};

  } // end GUI namespace
} // end namespace
#endif
