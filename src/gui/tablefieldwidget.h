/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TABLEFIELDWIDGET_H
#define TABLEFIELDWIDGET_H

class QTable;
class QEvent;

#include "fieldwidget.h"
#include "../datavectors.h"

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class TableFieldWidget : public FieldWidget {
Q_OBJECT

public:
  TableFieldWidget(Data::FieldPtr field, QWidget* parent, const char* name=0);
  virtual ~TableFieldWidget() {}

  virtual QString text() const;
  virtual void setText(const QString& text);

  /**
   * Event filter used to popup the menu
   */
  bool eventFilter(QObject* obj, QEvent* ev);

public slots:
  virtual void clear();

protected:
  virtual QWidget* widget();
  virtual void updateFieldHook(Data::FieldPtr oldField, Data::FieldPtr newField);

private slots:
  void contextMenu(int row, int col, const QPoint& p);
  void slotCheckRows(int row, int col);
  void slotResizeColumn(int row, int col);
  void slotRenameColumn();
  void slotInsertRow();
  void slotRemoveRow();
  void slotMoveRowUp();
  void slotMoveRowDown();

private:
  bool emptyRow(int row) const;
  void labelColumns(Data::FieldPtr field);

  QTable* m_table;
  int m_columns;
  Data::FieldPtr m_field;
  int m_row;
  int m_col;
};

  } // end GUI namespace
} // end namespace
#endif
