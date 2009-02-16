/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_GUI_TABLEFIELDWIDGET_H
#define TELLICO_GUI_TABLEFIELDWIDGET_H

#include "fieldwidget.h"
#include "../datavectors.h"

class QTableWidget;
class QTableWidgetItem;
class QEvent;

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class TableFieldWidget : public FieldWidget {
Q_OBJECT

public:
  TableFieldWidget(Data::FieldPtr field, QWidget* parent);
  virtual ~TableFieldWidget() {}

  virtual QString text() const;
  virtual void setText(const QString& text);

public slots:
  virtual void clear();

protected:
  virtual QWidget* widget();
  virtual void updateFieldHook(Data::FieldPtr oldField, Data::FieldPtr newField);

private slots:
  void tableContextMenu(const QPoint& point);
  void horizontalHeaderContextMenu(const QPoint& point);
  void verticalHeaderContextMenu(const QPoint& point);
  void slotCheckRows(int row, int col);
  void slotResizeColumn(QTableWidgetItem* item);
  void slotRenameColumn();
  void slotInsertRow();
  void slotRemoveRow();
  void slotMoveRowUp();
  void slotMoveRowDown();

private:
  void makeRowContextMenu(const QPoint& point);
  bool emptyRow(int row) const;
  void labelColumns(Data::FieldPtr field);

  QTableWidget* m_table;
  int m_columns;
  Data::FieldPtr m_field;
  int m_row;
  int m_col;
};

  } // end GUI namespace
} // end namespace
#endif
