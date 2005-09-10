/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
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

#include "fieldwidget.h"

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class TableFieldWidget : public FieldWidget {
Q_OBJECT

public:
  TableFieldWidget(const Data::Field* field, QWidget* parent, const char* name=0);
  virtual ~TableFieldWidget() {}

  virtual QString text() const;
  virtual void setText(const QString& text);

public slots:
  virtual void clear();

protected:
  virtual QWidget* widget();
  virtual void updateFieldHook(Data::Field* oldField, Data::Field* newField);

private slots:
  void slotCheckRows(int row, int col);
  void slotResizeColumn(int row, int col);

private:
  QTable* m_table;
  int m_columns;
};

  } // end GUI namespace
} // end namespace
#endif
